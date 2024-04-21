// Configure toml++ before toml.hpp is included in toml_config_reader.h
#define TOML_IMPLEMENTATION
#define TOML_ENABLE_FORMATTERS 0

#include "toml_config_reader.h"

#include <exception>
#include <gsl/gsl>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

using namespace std::literals;

namespace pefti {

TomlConfigReader::TomlConfigReader(std::string& filename) {
  toml_config_ = toml::parse_file(filename);
}

void TomlConfigReader::check_required_element(std::string_view path) {
  auto node = toml_config_[toml::path{path}];
  if (!node) {
    std::ostringstream stream;
    stream << path << ": missing from config"s;
    throw std::runtime_error(stream.str());
  }
}

}  // namespace pefti
