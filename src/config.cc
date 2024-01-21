// Configure toml++ before toml.hpp is included in config.h
#define TOML_IMPLEMENTATION
#define TOML_ENABLE_FORMATTERS 0
#include "config.h"

#include <algorithm>
#include <filesystem>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "iptv_channel.h"

namespace pefti {

Config::Config() {
  try {
    m_config = toml::parse_file("config.toml");
  } catch (const toml::parse_error& e) {
    std::ostringstream stream;
    stream << e.description() << "(" << e.source().begin << ")";
    throw stream.str();
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

std::span<std::string> Config::get_channels_group_names() {
  return std::span(m_group_names);
}

const std::vector<IptvChannel::Tag>& Config::get_channel_tags(
    std::string_view channel_original_name) {
  return get_allowed_channel(channel_original_name).tags;
}

const Config::DuplicatesLocation& Config::get_duplicates_location() noexcept {
  return m_duplicates_location;
}

const std::vector<std::string>& Config::get_input_playlists_filenames() {
  return m_input_playlists_filenames;
}

int Config::get_max_num_duplicates() noexcept { return m_max_num_duplicates; }

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
      if (!(toml_value.is_array())) throw "[channels]allow: expected array";
      auto array = toml_value.as_array();
      for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
        if (!((*iter).is_string()))
          throw "[channels]allow: expected text string";
        channel.allow.push_back(std::string{**((*iter).as_string())});
      }
    } else if (key == "block") {
      auto array{toml_value.as_array()};
      for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
        if (!((*iter).is_string()))
          throw "[channels]allow: expected text string";
        channel.block.push_back(std::string{**((*iter).as_string())});
      }
    } else if (key == "rename") {
      if (!(toml_value.is_string()))
        throw "[channels]allow: expected text string";
      channel.new_name = std::string{*(toml_value.as_string())};
    } else if (key == "tags") {
      if (!(toml_value.is_table())) throw "[channels]allow: expected table";
      auto table(toml_value.as_table());
      table->for_each(
          [this, &channel](const toml::key& toml_key, auto&& toml_value) {
            channel.tags.push_back(std::make_pair(
                std::string{toml_key}, std::string{*(toml_value.as_string())}));
          });
    } else
      throw "[channels]allow: invalid key";
  });
}

void Config::convert_allowed_channels() {
  if (!m_config["channels"]["allow"]) return;
  auto node = m_config["channels"]["allow"];
  if (!node.is_array()) throw "[channels]allow: expected array";
  auto array = node.as_array();
  enum class ElementType { kGroupName, kChannels };
  ElementType element_type{ElementType::kGroupName};
  std::string group_name;
  for (auto iter = array->cbegin(); iter != array->cend(); ++iter) {
    switch (element_type) {
      case ElementType::kGroupName: {
        if (!((*iter).is_string()))
          throw "[channels]allow: expected text string";
        group_name = **((*iter).as_string());
        m_group_names.push_back(group_name);
        break;
      }
      case ElementType::kChannels: {
        auto size_before = m_allowed_channels.size();
        if (!((*iter).is_array_of_tables()))
          throw "[channels]allow: expected array of tables";
        const auto channels = (*iter).as_array();
        channels->for_each([this, &group_name](auto&& toml_channel) {
          if (!(toml_channel.is_table()))
            throw "[channels]allow: expected table";
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
      if (!((*iter).is_string())) throw "Invalid configuration";
      if constexpr (std::is_same_v<decltype(collection),
                                   std::vector<std::string>&>)
        collection.push_back(**((*iter).as_string()));
      else if constexpr (std::is_same_v<decltype(collection),
                                        std::unordered_set<std::string>&>)
        collection.insert(**((*iter).as_string()));
      else
        throw "Type missing from convert_collection()";
    }
  } else
    throw "Invalid configuration";
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
    if (m_duplicates_location == kNone)
      m_max_num_duplicates = 0;
    else
      m_max_num_duplicates =
          m_config["channels"]["number_of_duplicates"].as_integer()->get();
  } else {
    m_duplicates_location = kNone;
    m_max_num_duplicates = 0;
  }
}

// Converts all configuration settings from TOML to a generic format
void Config::convert_from_toml() {
  convert_collections();
  convert_allowed_channels();
  convert_duplicates();
  convert_input_playlist_filenames();
  convert_new_playlist_filename();
}

void Config::convert_input_playlist_filenames() {
  if (!m_config["files"]["input_playlists"])
    throw "[files]input_playlists: missing";
  auto input_playlists = m_config["files"]["input_playlists"];
  if (!input_playlists.is_array())
    throw "[files]input_playlists: expected an array";
  auto array = input_playlists.as_array();
  for (auto iter = array->cbegin(); iter != array->cend(); iter++) {
    if (!((*iter).is_string()))
      throw "[files]input_playlists: array should contain text strings";
    m_input_playlists_filenames.push_back(**((*iter).as_string()));
    for (const auto& filename : m_input_playlists_filenames) {
      if (!std::filesystem::exists(filename)) {
        std::string error;
        error = "[files]input_playlists: " + filename + " does not exist";
        throw error;
      }
    }
  }
}

void Config::convert_new_playlist_filename() {
  if (!m_config["files"]["new_playlist"]) throw "[files]new_playlist: missing";
  auto new_playlist{m_config["files"]["new_playlist"]};
  if (!new_playlist.is_string())
    throw "[files]new_playlist: expected a text string";
  m_new_playlist_filename =
      m_config["files"]["new_playlist"].as_string()->get();
}

const Config::AllowedChannel& Config::get_allowed_channel(
    std::string_view channel_original_name) {
  return *(m_allowed_channels_lookup[std::string{channel_original_name}]);
}

}  // namespace pefti
