#include "config.h"

#include <string_view>
#include <vector>

#include "iptv_channel.h"

using namespace std::literals;

namespace pefti {

template class Config<TomlConfigReader>;

template <typename ConfigReader>
Config<ConfigReader>::Config(std::string& config_filename)
    : ConfigReader(config_filename) {
  ConfigReader::check_required_element("resources");
  ConfigReader::check_required_element("resources.playlists");
  ConfigReader::check_required_element("resources.new_playlist");
  ConfigReader::get_data("resources.playlists", m_config.playlists_urls);
  ConfigReader::get_data("resources.new_playlist",
                         m_config.new_playlist_filename);
  ConfigReader::get_data("resources.epgs", m_config.epgs_urls);
  ConfigReader::get_data("resources.new_epg", m_config.new_epg_filename);
  ConfigReader::get_data("groups.allow", m_config.allowed_groups);
  ConfigReader::get_data("groups.block", m_config.blocked_groups);
  ConfigReader::get_data("urls.block", m_config.blocked_urls);
  ConfigReader::get_data("channels.copy_group_title",
                         m_config.copy_group_title);
  ConfigReader::get_data("channels.number_of_duplicates",
                         m_config.num_duplicates);
  ConfigReader::get_data("channels.duplicates_location",
                         m_config.duplicates_location);
  ConfigReader::get_data("channels.sort_qualities", m_config.sort_qualities);
  ConfigReader::get_data("channels.tags_block", m_config.blocked_tags);
  ConfigReader::get_data("channels.block", m_config.blocked_channels);
  ConfigReader::get_data("channels.allow", m_config.channels_templates);
  if (m_config.duplicates_location == "inline")
    m_duplicates_location = DuplicatesLocation::kInline;
  else if (m_config.duplicates_location == "append")
    m_duplicates_location = DuplicatesLocation::kAppend;
  else {
    m_duplicates_location = DuplicatesLocation::kNone;
    m_config.num_duplicates = 0;
  }
}

template <typename ConfigReader>
const Config<ConfigReader>::DuplicatesLocation&
Config<ConfigReader>::get_duplicates_location() noexcept {
  return m_duplicates_location;
}

template <typename ConfigReader>
const std::vector<std::string>& Config<ConfigReader>::get_epgs_urls() noexcept {
  return m_config.epgs_urls;
}

template <typename ConfigReader>
const std::vector<std::string>& Config<ConfigReader>::get_playlists_urls() {
  return m_config.playlists_urls;
}

template <typename ConfigReader>
int Config<ConfigReader>::get_num_duplicates() noexcept {
  return m_config.num_duplicates;
}

template <typename ConfigReader>
std::string_view Config<ConfigReader>::get_new_epg_filename() noexcept {
  return m_config.new_epg_filename;
}

template <typename ConfigReader>
const std::string& Config<ConfigReader>::get_new_playlist_filename() noexcept {
  return m_config.new_playlist_filename;
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_allowed_group(std::string_view group_name) {
  return m_config.allowed_groups.contains(std::string{group_name});
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_channel(
    std::string_view original_channel_name) {
  bool is_blocked_channel{false};
  for (auto& substring : m_config.blocked_channels) {
    if (original_channel_name.find(substring) != std::string::npos) {
      is_blocked_channel = true;
      break;
    }
  }
  return is_blocked_channel;
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_group(std::string_view group_name) {
  return m_config.blocked_groups.contains(std::string{group_name});
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_url(std::string_view url) {
  return m_config.blocked_urls.contains(std::string{url});
}

}  // namespace pefti
