#include "config.h"

#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;

namespace pefti {

template class Config<TomlConfigReader>;

template <typename ConfigReader>
Config<ConfigReader>::Config(std::string& config_filename)
    : ConfigReader(config_filename) {
  ConfigReader::check_required_element("resources");
  ConfigReader::check_required_element("resources.playlists");
  ConfigReader::check_required_element("resources.new_playlist");
  ConfigReader::get_data("resources.playlists", config_.playlists_urls);
  ConfigReader::get_data("resources.new_playlist",
                         config_.new_playlist_filename);
  ConfigReader::get_data("resources.epgs", config_.epgs_urls);
  ConfigReader::get_data("resources.new_epg", config_.new_epg_filename);
  ConfigReader::get_data("groups.allow", config_.allowed_groups);
  ConfigReader::get_data("groups.block", config_.blocked_groups);
  ConfigReader::get_data("urls.block", config_.blocked_urls);
  ConfigReader::get_data("channels.copy_group_title", config_.copy_group_title);
  ConfigReader::get_data("channels.number_of_duplicates",
                         config_.num_duplicates);
  ConfigReader::get_data("channels.duplicates_location",
                         config_.duplicates_location);
  ConfigReader::get_data("channels.sort_qualities", config_.sort_qualities);
  ConfigReader::get_data("channels.tags_block", config_.blocked_tags);
  ConfigReader::get_data("channels.block", config_.blocked_channels);
  ConfigReader::get_data("channels.allow", config_.channels_templates);
  if (config_.duplicates_location == "inline")
    duplicates_location_ = DuplicatesLocation::kInline;
  else if (config_.duplicates_location == "append")
    duplicates_location_ = DuplicatesLocation::kAppend;
  else {
    duplicates_location_ = DuplicatesLocation::kNone;
    config_.num_duplicates = 0;
  }
}

template <typename ConfigReader>
const Config<ConfigReader>::DuplicatesLocation&
Config<ConfigReader>::get_duplicates_location() noexcept {
  return duplicates_location_;
}

template <typename ConfigReader>
const std::vector<std::string>& Config<ConfigReader>::get_epgs_urls() noexcept {
  return config_.epgs_urls;
}

template <typename ConfigReader>
const std::vector<std::string>& Config<ConfigReader>::get_playlists_urls() {
  return config_.playlists_urls;
}

template <typename ConfigReader>
int Config<ConfigReader>::get_num_duplicates() noexcept {
  return config_.num_duplicates;
}

template <typename ConfigReader>
std::string_view Config<ConfigReader>::get_new_epg_filename() noexcept {
  return config_.new_epg_filename;
}

template <typename ConfigReader>
const std::string& Config<ConfigReader>::get_new_playlist_filename() noexcept {
  return config_.new_playlist_filename;
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_allowed_group(std::string_view group_name) {
  return config_.allowed_groups.contains(std::string{group_name});
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_channel(
    std::string_view original_channel_name) {
  bool is_blocked_channel{false};
  for (auto& substring : config_.blocked_channels) {
    if (original_channel_name.find(substring) != std::string::npos) {
      is_blocked_channel = true;
      break;
    }
  }
  return is_blocked_channel;
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_group(std::string_view group_name) {
  return config_.blocked_groups.contains(std::string{group_name});
}

template <typename ConfigReader>
bool Config<ConfigReader>::is_blocked_url(std::string_view url) {
  return config_.blocked_urls.contains(std::string{url});
}

}  // namespace pefti
