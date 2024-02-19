// Configure toml++ before toml.hpp is included in config.h
#define TOML_IMPLEMENTATION
#define TOML_ENABLE_FORMATTERS 0
#include "config.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <gsl/gsl>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "iptv_channel.h"

using namespace std::literals;
namespace fs = std::filesystem;

namespace pefti {

Config::Config(std::string&& filename) {
  try {
    auto status = fs::status(filename);
    if (!fs::exists(status)) {
      throw std::runtime_error("Configuration file does not exist"s);
    } else if (fs::is_directory(status)) {
      std::ostringstream stream;
      stream << filename << " is a directory"s;
      throw std::runtime_error(stream.str());
    } else if (!fs::is_regular_file(status) && !fs::is_symlink(status)) {
      std::ostringstream stream;
      stream << filename << " is not a valid file"s;
      throw std::runtime_error(stream.str());
    }
    m_config = toml::parse_file(filename);
  } catch (const toml::parse_error& e) {
    std::ostringstream stream;
    stream << e.description() << '(' << e.source().begin << ')';
    throw std::runtime_error(stream.str());
  }
  convert_from_toml();
}

// Returns a vector containing the names of channels in group `group_name`
// as specified in [channels]allow in the configuration file
std::vector<std::string> Config::get_channel_names_in_group(
    std::string_view group_name) {
  std::vector<std::string> channel_names;
  int start_index{};
  int i{};
  auto list = m_allowed_channels;  // alias
  while (m_group_names[i] != group_name)
    start_index += m_channels_groups_size[i++];
  int end_index = start_index + m_channels_groups_size[i];
  std::for_each(m_allowed_channels.cbegin() + start_index,
                m_allowed_channels.cbegin() + end_index,
                [&channel_names](auto&& channel) {
                  channel_names.push_back(channel.new_name);
                });
  return channel_names;
}

const std::string& Config::get_channel_new_name(
    std::string_view channel_original_name) {
  return get_allowed_channel(channel_original_name).new_name;
}

const std::vector<IptvChannel::Tag>& Config::get_channel_tags(
    std::string_view channel_original_name) {
  return get_allowed_channel(channel_original_name).tags;
}

std::span<std::string> Config::get_channels_group_names() {
  return std::span(m_group_names);
}

const Config::DuplicatesLocation& Config::get_duplicates_location() noexcept {
  return m_duplicates_location;
}

const std::vector<std::string>& Config::get_epgs_urls() noexcept {
  return m_epgs_urls;
}

const std::vector<std::string>& Config::get_playlists_urls() {
  return m_playlists_urls;
}

int Config::get_max_num_duplicates() noexcept { return m_max_num_duplicates; }

std::string_view Config::get_new_epg_filename() noexcept {
  return m_new_epg_filename;
}

const std::string& Config::get_new_playlist_filename() noexcept {
  return m_new_playlist_filename;
}

void to_lower(std::string_view original, std::string& lowercase) {
  std::transform(original.cbegin(), original.cend(),
                 std::back_inserter(lowercase),
                 [](unsigned char c) { return std::tolower(c); });
}

// Check if a channel is an allowed channel by comparing with all channels
// specified in [channels]allow in the configuration. Case insensitive
// comparisons are performed by converting both text strings to lowercase.
bool Config::is_allowed_channel(std::string_view original_name) {
  //
  // Comparing a channel name to all of the allowed channel names in the
  // configuration is an expensive operation so the results are cached in
  // m_allowed_channels_lookup.
  if (m_allowed_channels_lookup.contains(std::string{original_name}))
    return true;
  //
  // Convert channel_name to lowercase
  std::string original_name_lower{};
  original_name_lower.reserve(original_name.size());
  to_lower(original_name, original_name_lower);
  //
  // Checks if `search_text` is contained within the channel name. Converts
  // `search_text` to lowercase
  auto channel_name_contains = [&original_name_lower](const auto& search_text) {
    std::string search_text_lower{};
    search_text_lower.reserve(search_text.size());
    to_lower(search_text, search_text_lower);
    auto pos = original_name_lower.find(search_text_lower);
    if (pos != std::string::npos) {
      bool char_before_is_alphanumeric{
          (pos != 0) && (std::isalnum(original_name_lower[pos - 1]) != 0)};
      bool char_after_is_alphanumeric{
          std::isalnum(original_name_lower[pos + search_text_lower.length()]) !=
          0};
      if (!char_before_is_alphanumeric && !char_after_is_alphanumeric)
        return true;
    }
    return false;
  };
  for (auto& channel : m_allowed_channels) {
    bool is_allowed = std::all_of(channel.allow.cbegin(), channel.allow.cend(),
                                  channel_name_contains);
    bool is_blocked = std::any_of(channel.block.cbegin(), channel.block.cend(),
                                  channel_name_contains);
    if (is_allowed && !is_blocked) {
      // Add this result to the cache
      m_allowed_channels_lookup[std::string{original_name}] = &channel;
      return true;
    }
  }
  return false;
}

bool Config::is_allowed_group(std::string_view group_name) {
  return m_allowed_groups.contains(std::string{group_name});
}

bool Config::is_blocked_channel(std::string_view original_channel_name) {
  bool is_blocked_channel{false};
  for (auto& substring : m_blocked_channels) {
    if (original_channel_name.find(substring) != std::string::npos) {
      is_blocked_channel = true;
      break;
    }
  }
  return is_blocked_channel;
}

bool Config::is_blocked_group(std::string_view group_name) {
  return m_blocked_groups.contains(std::string{group_name});
}

bool Config::is_blocked_url(std::string_view url) {
  return m_blocked_urls.contains(std::string{url});
}

void Config::convert_allowed_channel_tags(toml::v3::table& tags,
                                          AllowedChannel& channel) {
  tags.for_each([this, &channel](const toml::key& toml_key, auto&& toml_value) {
    std::string key = std::string{toml_key};
    if (key == "allow") {
      if (!(toml_value.is_array()))
        throw std::runtime_error("[channels]allow: expected array"s);
      auto array = toml_value.as_array();
      for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
        if (!((*iter).is_string()))
          throw std::runtime_error("[channels]allow: expected text string"s);
        channel.allow.push_back(std::string{**((*iter).as_string())});
      }
    } else if (key == "block") {
      auto array{toml_value.as_array()};
      for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
        if (!((*iter).is_string()))
          throw std::runtime_error("[channels]allow: expected text string"s);
        channel.block.push_back(std::string{**((*iter).as_string())});
      }
    } else if (key == "rename") {
      if (!(toml_value.is_string()))
        throw std::runtime_error("[channels]allow: expected text string"s);
      channel.new_name = std::string{*(toml_value.as_string())};
    } else if (key == "tags") {
      if (!(toml_value.is_table()))
        throw std::runtime_error("[channels]allow: expected table"s);
      auto table(toml_value.as_table());
      table->for_each(
          [this, &channel](const toml::key& toml_key, auto&& toml_value) {
            channel.tags.push_back(std::make_pair(
                std::string{toml_key}, std::string{*(toml_value.as_string())}));
          });
    } else
      throw std::runtime_error("[channels]allow: invalid key"s);
  });
}

void Config::convert_allowed_channels() {
  Expects(m_group_names.empty());
  if (!m_config["channels"]["allow"]) return;
  auto node = m_config["channels"]["allow"];
  if (!node.is_array())
    throw std::runtime_error("[channels]allow: expected array"s);
  auto array = node.as_array();
  enum class ElementType { kGroupName, kChannels };
  ElementType element_type{ElementType::kGroupName};
  std::string group_name;
  for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
    switch (element_type) {
      case ElementType::kGroupName: {
        if (!((*iter).is_string()))
          throw std::runtime_error("[channels]allow: expected text string"s);
        group_name = **((*iter).as_string());
        m_group_names.push_back(group_name);
        break;
      }
      case ElementType::kChannels: {
        auto size_before = m_allowed_channels.size();
        if (!((*iter).is_array_of_tables()))
          throw std::runtime_error(
              "[channels]allow: expected array of tables"s);
        const auto channels = (*iter).as_array();
        channels->for_each([this, &group_name](auto&& toml_channel) {
          if (!(toml_channel.is_table()))
            throw std::runtime_error("[channels]allow: expected table"s);
          auto table = *(toml_channel.as_table());
          AllowedChannel channel{};
          convert_allowed_channel_tags(table, channel);
          channel.tags.push_back(
              std::make_pair(std::string{"group-title"}, group_name));
          //
          // If there is no "rename" tag for this channel,
          // then use the first string from the "allow" array as the new name
          if (channel.new_name.empty()) channel.new_name = channel.allow[0];
          m_allowed_channels.push_back(channel);
        });
        m_channels_groups_size.push_back(m_allowed_channels.size() -
                                         size_before);
        break;
      }
    }
    // Toggle
    element_type = (element_type == ElementType::kGroupName)
                       ? ElementType::kChannels
                       : ElementType::kGroupName;
  }
}

template <template <class...> class T>
void convert_collection(toml::v3::node_view<toml::v3::node> node,
                        T<std::string>& collection) {
  auto items{node};
  if (items.is_array()) {
    auto array = items.as_array();
    for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
      if (!((*iter).is_string()))
        throw std::runtime_error("Invalid configuration"s);
      if constexpr (std::is_same_v<decltype(collection),
                                   std::vector<std::string>&>)
        collection.push_back(**((*iter).as_string()));
      else if constexpr (std::is_same_v<decltype(collection),
                                        std::unordered_set<std::string>&>)
        collection.insert(**((*iter).as_string()));
      else
        throw std::runtime_error("Type missing from convert_collection()"s);
    }
  } else
    throw std::runtime_error("Invalid configuration"s);
}

void Config::convert_collections() {
  if (m_config["groups"]["block"])
    convert_collection(m_config["groups"]["block"], m_blocked_groups);
  if (m_config["channels"]["block"])
    convert_collection(m_config["channels"]["block"], m_blocked_channels);
  if (m_config["urls"]["block"])
    convert_collection(m_config["urls"]["block"], m_blocked_urls);
  if (m_config["groups"]["allow"])
    convert_collection(m_config["groups"]["allow"], m_allowed_groups);
  if (m_config["channels"]["qualities"])
    convert_collection(m_config["channels"]["qualities"], m_qualities);
  if (m_config["channels"]["tags_block"])
    convert_collection(m_config["channels"]["tags_block"], m_blocked_tags);
}

void Config::convert_duplicates() {
  using enum DuplicatesLocation;
  if (m_config["channels"]["duplicates_location"]) {
    std::string location{
        m_config["channels"]["duplicates_location"].as_string()->get()};
    if (location == "inline")
      m_duplicates_location = kInline;
    else if (location == "append_to_group")
      m_duplicates_location = kAppendToGroup;
    else
      m_duplicates_location = kNone;
    m_max_num_duplicates =
        (m_duplicates_location == kNone)
            ? 0
            : m_config["channels"]["number_of_duplicates"].as_integer()->get();
  } else {
    m_duplicates_location = kNone;
    m_max_num_duplicates = 0;
  }
}

void Config::convert_epgs_urls() {
  Expects(m_epgs_urls.empty());
  if (!m_config["resources"]["epgs"]) return;
  auto epgs = m_config["resources"]["epgs"];
  if (!epgs.is_array())
    throw std::runtime_error("[files]epgs: expected an array"s);
  auto array = epgs.as_array();
  for (auto iter = array->cbegin(); iter != array->cend(); iter++) {
    if (!((*iter).is_string()))
      throw std::runtime_error(
          "[files]epgs: array should contain text strings"s);
    m_epgs_urls.push_back(**((*iter).as_string()));
  }
  Expects(!m_epgs_urls.empty());
}

// Converts all configuration settings from TOML to a generic format
void Config::convert_from_toml() {
  convert_collections();
  convert_allowed_channels();
  convert_duplicates();
  convert_playlists_urls();
  convert_new_playlist_filename();
  convert_epgs_urls();
  convert_new_epg_filename();
}

void Config::convert_new_epg_filename() {
  Expects(m_new_epg_filename.empty());
  if (!m_config["resources"]["new_epg"]) return;
  auto new_epg{m_config["resources"]["new_epg"]};
  if (!new_epg.is_string())
    throw std::runtime_error("[files]new_epg: expected a text string"s);
  m_new_epg_filename = m_config["resources"]["new_epg"].as_string()->get();
  Ensures(!m_new_epg_filename.empty());
}

void Config::convert_new_playlist_filename() {
  Expects(m_new_playlist_filename.empty());
  if (!m_config["resources"]["new_playlist"])
    throw std::runtime_error("[files]new_playlist: missing");
  auto new_playlist{m_config["resources"]["new_playlist"]};
  if (!new_playlist.is_string())
    throw std::runtime_error("[files]new_playlist: expected a text string"s);
  m_new_playlist_filename =
      m_config["resources"]["new_playlist"].as_string()->get();
  Ensures(!m_new_playlist_filename.empty());
}

void Config::convert_playlists_urls() {
  Expects(m_playlists_urls.empty());
  if (!m_config["resources"]["playlists"])
    throw std::runtime_error("[files]playlists: missing"s);
  auto playlists = m_config["resources"]["playlists"];
  if (!playlists.is_array())
    throw std::runtime_error("[files]playlists: expected an array"s);
  auto array = playlists.as_array();
  for (auto iter = array->cbegin(); iter != array->cend(); iter++) {
    if (!((*iter).is_string()))
      throw std::runtime_error(
          "[files]playlists: array should contain text strings"s);
    m_playlists_urls.push_back(**((*iter).as_string()));
  }
  Ensures(!m_playlists_urls.empty());
}

const Config::AllowedChannel& Config::get_allowed_channel(
    std::string_view channel_original_name) {
  return *(m_allowed_channels_lookup[std::string{channel_original_name}]);
}

}  // namespace pefti
