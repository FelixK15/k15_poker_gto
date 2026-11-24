// gto-cli: runs the solver engine without a server.
// Solving from a JSON config arrives in M1; for now it only reports its version.

#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "gto/version.h"

int main(int argc, char** argv) {
  const std::string_view arg = argc > 1 ? argv[1] : "--help";
  const std::string_view version = gto::engine_version();

  if (arg == "--version") {
    std::printf("gto-cli (engine %.*s)\n", static_cast<int>(version.size()), version.data());
    return EXIT_SUCCESS;
  }
  if (arg == "--help" || arg == "-h") {
    std::puts("Usage: gto-cli --version\n\nSolving from a config file is not implemented yet.");
    return EXIT_SUCCESS;
  }
  std::fprintf(stderr, "error: unknown argument '%s'\n", argv[1]);
  return EXIT_FAILURE;
}
