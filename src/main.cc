#include <exception>
#include <iostream>

#include <cxxopts.hpp>

#include "application.h"
#include "version.h"

static void print_help();
static void print_version();

// Creates and runs the application.
// All exceptions are handled here.
// Returns 0 on normal exit, or 1 on abnormal exit.
int main(int argc, char* argv[]) {
  static constexpr auto kExpectedNumArgs{2};
  if (argc != kExpectedNumArgs) {
    print_help();
    return 0;
  }
  try {
    cxxopts::Options options("pefti", "Playlist and EPG Filter/Transformer for IPTV");
    options.add_options()
      ("h,help", "Print usage")
      ("v,version", "Print version");
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      print_help();
    } else if (result.count("version")) {
      print_version();
    } else {
      pefti::Application app(argc, argv);
      app.run();
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}

static void print_help() {
  std::cout << "Usage: pefti [OPTION]... [--] config-file\n";
  std::cout << "  -h, --help              display this help text and exit\n";
  std::cout << 
      "  -v, --version           display version information and exit\n";
  std::cout << "Full documentation <https://github.com/junglerock99/pefti>\n";
}

static void print_version() {
  std::cout << "pefti " << kVersionMajor << '.' << 
                           kVersionMinor << '.' << 
                           kVersionPatch << '\n';
}

