#include <cxxopts.hpp>
#include <exception>
#include <iostream>

#include "application.h"
#include "version.h"

enum class AppStatus { kOk, kError, kFinished };

static void print_usage();
static void print_version();
static AppStatus process_arguments(int argc, char* argv[]);

// Creates and runs the application.
// All exceptions are handled here.
// Returns 0 on normal exit, or 1 on abnormal exit.
int main(int argc, char* argv[]) {
  int result = 0;
  try {
    AppStatus status = process_arguments(argc, argv);
    if (status == AppStatus::kOk) {
      pefti::Application app(argc, argv);
      app.run();
    } else if (status == AppStatus::kError) {
      result = 1;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    result = 1;
  }
  return result;
}

// Handles command-line arguments
static AppStatus process_arguments(int argc, char* argv[]) {
  AppStatus status = AppStatus::kOk;
  static constexpr auto kExpectedNumArgs{2};
  if (argc < kExpectedNumArgs) {
    print_usage();
    status = AppStatus::kError;
  } else {
    cxxopts::Options options("pefti",
                             "Playlist and EPG Filter/Transformer for IPTV");
    options.add_options()("h,help", "Print usage")
                         ("v,version", "Print version");
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      print_usage();
      status = AppStatus::kFinished;
    } else if (result.count("version")) {
      print_version();
      status = AppStatus::kFinished;
    }
  }
  return status;
}

static void print_usage() {
  std::cout << "Usage: pefti [OPTION]... [--] config-file\n";
  std::cout << "  -h, --help              display this help text and exit\n";
  std::cout
      << "  -v, --version           display version information and exit\n";
  std::cout << "Full documentation <https://github.com/junglerock99/pefti>\n";
}

static void print_version() {
  std::cout << "pefti " << kVersionMajor << '.' << kVersionMinor << '.'
            << kVersionPatch << '\n';
}
