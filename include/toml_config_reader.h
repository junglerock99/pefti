#pragma once

#define TOML_HEADER_ONLY 0
#include <exception>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <gsl/gsl>

#include "toml++/toml.hpp"

using namespace std::literals;

namespace pefti {

class TomlConfigReader {
 public:
  explicit TomlConfigReader(std::string& filename);
  void check_required_element(std::string_view path);

  // Get collections
  template <template <typename...> typename T, typename U>
  void get_data(std::string_view path, T<U>& destination) {
    Expects(destination.empty());
    auto node = m_toml_config[toml::path{path}];
    if (!node) return;
    if (!node.is_array()) throw std::runtime_error("Expected array"s);
    auto array = node.as_array();
    for (auto iter = array->cbegin(); iter != array->cend(); iter++) {
      //
      // Check if this is a collection of strings
      if constexpr (std::is_same_v<U, std::string>) {
        if (!((*iter).is_string()))
          throw std::runtime_error("Expected array of text strings"s);
        auto string = **((*iter).as_string());
        if constexpr (std::is_same_v<T<std::string>,
                                     std::vector<std::string>>) {
          destination.push_back(string);
        } else if constexpr (std::is_same_v<T<std::string>,
                                            std::unordered_set<std::string>>) {
          destination.insert(string);
        } else {
          throw std::runtime_error(
              "Unhandled type in TomlConfigReader::get_array()"s);
        }
      } else {
        if (!((*iter).is_table())) throw std::runtime_error("Expected table"s);
        auto table = *((*iter).as_table());
        U channel{};
        get_key_value_pairs(table, channel);
        if (channel.new_name.empty()) channel.new_name = channel.include[0];
        destination.push_back(channel);
      }
    }
  }

  // Get strings
  template <typename T>
  void get_data(std::string_view path, std::basic_string<T>& destination) {
    auto node = m_toml_config[toml::path{path}];
    if (!node) return;
    if (!node.is_string()) throw std::runtime_error("Expected text string"s);
    destination = **(node.as_string());
  }

  // Get integers and booleans
  template <typename T>
  void get_data(std::string_view path, T& destination) {
    auto node = m_toml_config[toml::path{path}];
    if (!node) return;
    if constexpr (std::is_integral_v<T>) {
      if (node.is_boolean()) {
        destination = **(node.as_boolean());
      } else if (node.is_integer()) {
        destination = **(node.as_integer());
      } else {
        throw std::runtime_error("Expected integer or boolean"s);
      }
    } else {
      throw std::runtime_error(
          "Unhandled type in TomlConfigReader::get_data()"s);
    }
  }

  template <typename T>
  void get_key_value_pairs(toml::v3::table& tags, T& channel) {
    tags.for_each([this, &channel](const toml::key& toml_key,
                                   auto&& toml_value) {
      std::string key = std::string{toml_key};
      if (key == "i") {
        if (!(toml_value.is_array()))
          throw std::runtime_error("channels.allow: expected array"s);
        auto array = toml_value.as_array();
        for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
          if (!((*iter).is_string()))
            throw std::runtime_error("channels.allow: expected text string"s);
          channel.include.push_back(std::string{**((*iter).as_string())});
        }
      } else if (key == "e") {
        auto array{toml_value.as_array()};
        for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
          if (!((*iter).is_string()))
            throw std::runtime_error("channels.allow: expected text string"s);
          channel.exclude.push_back(std::string{**((*iter).as_string())});
        }
      } else if (key == "n") {
        if (!(toml_value.is_string()))
          throw std::runtime_error("channels.allow: expected text string"s);
        channel.new_name = std::string{*(toml_value.as_string())};
      } else if (key == "t") {
        if (!(toml_value.is_table()))
          throw std::runtime_error("channels.allow: expected table"s);
        auto table(toml_value.as_table());
        table->for_each([this, &channel](const toml::key& toml_key,
                                         auto&& toml_value) {
          channel.tags.push_back(std::make_pair(
              std::string{toml_key}, std::string{*(toml_value.as_string())}));
        });
      } else
        throw std::runtime_error("channels.allow: invalid key"s);
    });
  }

 private:
  toml::table m_toml_config;
};

}  // namespace pefti
