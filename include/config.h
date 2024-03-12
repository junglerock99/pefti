#pragma once

#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config_reader.h"
#include "iptv_channel.h"

using namespace std::literals;

namespace pefti {

// Holds the data for one channel template in `channels.allow`
// in the configuration file
struct ChannelTemplate {
  std::vector<std::string> include;
  std::vector<std::string> exclude;
  std::string new_name;
  std::vector<std::pair<std::string, std::string>> tags;
};

class ChannelsMap;

template <typename ConfigReader>
class Config : public ConfigReader {
 public:
  enum class DuplicatesLocation { kNone, kInline, kAppend };

 public:
  explicit Config(std::string& config_filename);
  Config(Config&) = delete;
  Config(Config&&) = delete;
  Config& operator=(Config&) = delete;
  Config& operator=(Config&&) = delete;

  decltype(auto) get_allowed_groups() {
    return m_config.allowed_groups | std::ranges::views::all;
  }

  decltype(auto) get_blocked_tags() {
    return m_config.blocked_tags | std::ranges::views::all;
  }

  decltype(auto) get_channels_templates() {
    return m_config.channels_templates | std::ranges::views::all;
  }

  bool get_copy_group_title_flag() { return m_config.copy_group_title; }

  // Returns an enum representing the value of [channels].duplicates_location
  const DuplicatesLocation& get_duplicates_location() noexcept;

  const std::vector<std::string>& get_epgs_urls() noexcept;

  // Returns number of channels in [channels].allow in configuration file
  int get_num_channels_templates() {
    return m_config.channels_templates.size();
  }

  // Returns the number of groups in [groups].allow in the configuration file
  int get_num_allowed_groups() { return m_config.allowed_groups.size(); }

  // Returns the value of [channels].number_of_duplicates in the configuration
  // file
  int get_num_duplicates() noexcept;

  // Returns the value of [files].new_epg from the configuration file
  std::string_view get_new_epg_filename() noexcept;

  // Returns the value of [files].new_playlist from the configuration file
  const std::string& get_new_playlist_filename() noexcept;

  // Returns the filenames specified in [files].playlists
  const std::vector<std::string>& get_playlists_urls();

  decltype(auto) get_sort_qualities() {
    return m_config.sort_qualities | std::ranges::views::all;
  }

  bool is_allowed_group(std::string_view group_name);
  bool is_blocked_channel(std::string_view original_channel_name);
  bool is_blocked_group(std::string_view group_name);
  bool is_blocked_url(std::string_view url);

 private:
  struct PeftiConfig {
    std::vector<std::string> playlists_urls;
    std::string new_playlist_filename;
    std::vector<std::string> epgs_urls;
    std::string new_epg_filename;
    std::unordered_set<std::string> blocked_groups;
    std::unordered_set<std::string> allowed_groups;
    std::unordered_set<std::string> blocked_urls;
    bool copy_group_title;
    int num_duplicates;
    std::string duplicates_location;
    std::vector<std::string> sort_qualities;
    std::vector<std::string> blocked_tags;
    std::unordered_set<std::string> blocked_channels;
    std::vector<ChannelTemplate> channels_templates;
  };

 private:
  PeftiConfig m_config;
  DuplicatesLocation m_duplicates_location;

  friend class ChannelsMapper;
};

using ConfigType = Config<TomlConfigReader>;

}  // namespace pefti
