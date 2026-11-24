#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "auth.h"
#include "gto/version.h"
#include "job_manager.h"
#include "services.h"
#include "system_info.h"

namespace {

struct Options {
  // Loopback by default: a server only becomes reachable from other machines
  // when the operator asks for it.
  std::string host = "127.0.0.1";
  int port = 50051;  // 0 = let the OS pick a free port.
  std::string token;
  std::string data_dir = "gto-data";
  unsigned threads = 0;
  std::uint64_t memory_limit_mb = 0;  // 0 = 80% of the machine's RAM
};

void print_usage() {
  std::puts(
      "gto-server " GTO_SERVER_VERSION
      "\n\n"
      "Usage: gto-server [options]\n\n"
      "  --host <address>        Address to listen on (default 127.0.0.1).\n"
      "                          Use 0.0.0.0 to accept remote clients; that\n"
      "                          requires an access token.\n"
      "  --port <port>           TCP port (default 50051). 0 picks a free port.\n"
      "  --token <token>         Access token clients must send. Prefer the\n"
      "                          environment variable GTO_SERVER_TOKEN, which\n"
      "                          does not show up in the process list.\n"
      "  --data-dir <path>       Where jobs and solutions are kept (default\n"
      "                          ./gto-data). Pass an empty string to keep\n"
      "                          everything in memory only.\n"
      "  --threads <n>           Threads for solving (default: all).\n"
      "  --memory-limit-mb <n>   Largest solve to accept (default: 80% of RAM).\n"
      "  --version               Print version and exit.\n"
      "  --help                  Print this help and exit.\n\n"
      "After startup the server prints one line 'LISTENING <port>' on stdout.\n"
      "Connections are not encrypted yet: on untrusted networks, put the server\n"
      "behind a VPN or an SSH tunnel.");
}

template <typename T>
bool parse_number(const char* text, T min, T max, T& out) {
  if (text == nullptr) return false;
  const std::string_view view = text;
  T value{};
  const auto result = std::from_chars(view.data(), view.data() + view.size(), value);
  if (view.empty() || result.ec != std::errc{} || result.ptr != view.data() + view.size() ||
      value < min || value > max) {
    return false;
  }
  out = value;
  return true;
}

// Returns true to continue, false to exit with `exit_code`.
bool parse_args(int argc, char** argv, Options& options, int& exit_code) {
  const auto fail = [&](const char* message) {
    std::fprintf(stderr, "error: %s\n", message);
    exit_code = EXIT_FAILURE;
    return false;
  };

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    const auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };

    if (arg == "--help" || arg == "-h") {
      print_usage();
      exit_code = EXIT_SUCCESS;
      return false;
    }
    if (arg == "--version") {
      std::printf("gto-server %s (engine %.*s)\n", GTO_SERVER_VERSION,
                  static_cast<int>(gto::engine_version().size()), gto::engine_version().data());
      exit_code = EXIT_SUCCESS;
      return false;
    }
    if (arg == "--host") {
      const char* value = next();
      if (value == nullptr || *value == '\0') return fail("--host needs an address");
      options.host = value;
    } else if (arg == "--port") {
      if (!parse_number(next(), 0, 65535, options.port)) {
        return fail("--port needs a number between 0 and 65535");
      }
    } else if (arg == "--token") {
      const char* value = next();
      if (value == nullptr || *value == '\0') return fail("--token needs a value");
      options.token = value;
    } else if (arg == "--data-dir") {
      const char* value = next();
      if (value == nullptr) return fail("--data-dir needs a path");
      options.data_dir = value;
    } else if (arg == "--threads") {
      if (!parse_number(next(), 1u, 4096u, options.threads)) {
        return fail("--threads needs a number from 1 to 4096");
      }
    } else if (arg == "--memory-limit-mb") {
      if (!parse_number<std::uint64_t>(next(), 1, 1ull << 40, options.memory_limit_mb)) {
        return fail("--memory-limit-mb needs a positive number");
      }
    } else {
      std::fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
      exit_code = EXIT_FAILURE;
      return false;
    }
  }
  return true;
}

bool is_loopback(const std::string& host) {
  return host == "127.0.0.1" || host == "localhost" || host == "::1" || host == "[::1]";
}

std::atomic<bool> g_stop{false};

void on_signal(int) { g_stop = true; }

}  // namespace

int main(int argc, char** argv) {
  Options options;
  int exit_code = EXIT_SUCCESS;
  if (!parse_args(argc, argv, options, exit_code)) return exit_code;

  if (options.token.empty()) {
    if (const char* env = std::getenv("GTO_SERVER_TOKEN"); env != nullptr) options.token = env;
  }
  if (options.token.empty() && !is_loopback(options.host)) {
    std::fputs("error: listening on a non-loopback address needs an access token.\n"
               "       Set GTO_SERVER_TOKEN or pass --token.\n",
               stderr);
    return EXIT_FAILURE;
  }

  gto::server::JobManagerConfig config;
  config.data_dir = options.data_dir;
  config.threads = options.threads;
  config.memory_budget_bytes = options.memory_limit_mb > 0
                                   ? options.memory_limit_mb << 20
                                   : gto::server::memory_info().total_bytes / 10 * 8;

  const gto::server::Auth auth(options.token);
  gto::server::JobManager jobs(config);
  gto::server::ServerServiceImpl server_service(auth, GTO_SERVER_VERSION);
  gto::server::TreeServiceImpl tree_service(auth, jobs);
  gto::server::JobServiceImpl job_service(auth, jobs);
  gto::server::SolutionServiceImpl solution_service(auth, jobs);

  int bound_port = 0;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(options.host + ":" + std::to_string(options.port),
                           grpc::InsecureServerCredentials(), &bound_port);
  builder.RegisterService(&server_service);
  builder.RegisterService(&tree_service);
  builder.RegisterService(&job_service);
  builder.RegisterService(&solution_service);

  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  if (!server || bound_port == 0) {
    spdlog::error("could not listen on {}:{}", options.host, options.port);
    return EXIT_FAILURE;
  }

  spdlog::info("gto-server {} listening on {}:{}", GTO_SERVER_VERSION, options.host, bound_port);
  spdlog::info("access token: {}", auth.required() ? "required" : "not required (loopback only)");
  spdlog::info("data directory: {}",
               options.data_dir.empty() ? "none, memory only" : options.data_dir);
  spdlog::info("memory limit per solve: {} MB", config.memory_budget_bytes >> 20);
  if (!is_loopback(options.host)) {
    spdlog::warn("connections are not encrypted; use a VPN or SSH tunnel on untrusted networks");
  }
  // Machine-readable line for a client that started us with --port 0.
  std::printf("LISTENING %d\n", bound_port);
  std::fflush(stdout);

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);
  while (!g_stop) std::this_thread::sleep_for(std::chrono::milliseconds(200));

  spdlog::info("shutting down");
  // Stop solving and save first, then end the open calls.
  jobs.shutdown();
  server->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(2));
  return EXIT_SUCCESS;
}
