#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include "gto/version.h"
#include "server_service.h"

namespace {

struct Options {
  // Loopback by default: a server only becomes reachable from other machines
  // when the operator asks for it. Authentication and TLS arrive in M3.
  std::string host = "127.0.0.1";
  int port = 50051;  // 0 = let the OS pick a free port.
};

void print_usage() {
  std::puts(
      "gto-server " GTO_SERVER_VERSION
      "\n\n"
      "Usage: gto-server [--host <address>] [--port <port>]\n\n"
      "  --host <address>  Address to listen on (default 127.0.0.1).\n"
      "                    Use 0.0.0.0 to accept remote clients.\n"
      "  --port <port>     TCP port (default 50051). 0 picks a free port.\n"
      "  --version         Print version and exit.\n"
      "  --help            Print this help and exit.\n\n"
      "After startup the server prints one line 'LISTENING <port>' on stdout.");
}

// Returns true to continue, false to exit with `exit_code`.
bool parse_args(int argc, char** argv, Options& options, int& exit_code) {
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
      if (value == nullptr || *value == '\0') {
        std::fputs("error: --host needs an address\n", stderr);
        exit_code = EXIT_FAILURE;
        return false;
      }
      options.host = value;
      continue;
    }
    if (arg == "--port") {
      const char* value = next();
      const std::string_view text = value != nullptr ? value : "";
      int port = -1;
      const auto result = std::from_chars(text.data(), text.data() + text.size(), port);
      if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
          port < 0 || port > 65535) {
        std::fputs("error: --port needs a number between 0 and 65535\n", stderr);
        exit_code = EXIT_FAILURE;
        return false;
      }
      options.port = port;
      continue;
    }
    std::fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
    exit_code = EXIT_FAILURE;
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  Options options;
  int exit_code = EXIT_SUCCESS;
  if (!parse_args(argc, argv, options, exit_code)) return exit_code;

  gto::server::ServerServiceImpl server_service;

  int bound_port = 0;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(options.host + ":" + std::to_string(options.port),
                           grpc::InsecureServerCredentials(), &bound_port);
  builder.RegisterService(&server_service);

  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  if (!server || bound_port == 0) {
    spdlog::error("could not listen on {}:{}", options.host, options.port);
    return EXIT_FAILURE;
  }

  spdlog::info("gto-server {} listening on {}:{}", GTO_SERVER_VERSION, options.host, bound_port);
  // Machine-readable line for a client that started us with --port 0.
  std::printf("LISTENING %d\n", bound_port);
  std::fflush(stdout);

  server->Wait();
  return EXIT_SUCCESS;
}
