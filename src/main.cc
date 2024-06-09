#include <cstdlib>
#include <cxxopts.hpp>
#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "application.h"
#include "version.h"

using namespace std::literals;
namespace fs = std::filesystem;

enum class AppStatus { kOk, kError, kFinished };

static constexpr auto kNumExpectedArgs{2};

static void print_usage();
static void print_version();
static AppStatus process_arguments(int argc, char* argv[],
                                   std::string& filename);
void verify_file(std::string& config_filename);

// Processes command-line arguments then creates and runs the application.
// All exceptions are handled here.
// Returns: EXIT_SUCCESS on successful execution
//          EXIT_FAILURE on unsuccessful execution
int main(int argc, char* argv[]) {
  try {
    std::string config_filename;
    AppStatus status = process_arguments(argc, argv, config_filename);
    if (status == AppStatus::kOk) {
      pefti::Application app(std::move(config_filename));
      app.run();
    } else if (status == AppStatus::kError) {
      return EXIT_FAILURE;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
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

static AppStatus process_arguments(int argc, char* argv[],
                                   std::string& config_filename) {
  if (argc < kNumExpectedArgs) {
    print_usage();
    return AppStatus::kError;
  }
  cxxopts::Options options("pefti",
                           "Playlist and EPG Filter/Transformer for IPTV");
  options.add_options()("h,help", "Print usage")("v,version", "Print version")(
      "config", "Configuration file", cxxopts::value<std::string>());
  options.parse_positional({"config"});
  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    print_usage();
    return AppStatus::kFinished;
  }
  if (result.count("version")) {
    print_version();
    return AppStatus::kFinished;
  }
  config_filename = result["config"].as<std::string>();
  verify_file(config_filename);
  return AppStatus::kOk;
}

void verify_file(std::string& config_filename) {
  auto status = fs::status(config_filename);
  if (!fs::exists(status)) {
    std::ostringstream stream;
    stream << config_filename << " does not exist"s;
    throw std::runtime_error(stream.str());
  } else if (fs::is_directory(status)) {
    std::ostringstream stream;
    stream << config_filename << " is a directory"s;
    throw std::runtime_error(stream.str());
  } else if (!fs::is_regular_file(status) && !fs::is_symlink(status)) {
    std::ostringstream stream;
    stream << config_filename << " is not a valid file"s;
    throw std::runtime_error(stream.str());
  }
}
