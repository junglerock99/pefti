#ifndef PEFTI_CONFIG_H_
#define PEFTI_CONFIG_H_

#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "iptv_channel.h"

#define TOML_HEADER_ONLY 0
#include "toml++/toml.hpp"

namespace pefti {

// Class Config converts the configuration data from TOML format to a generic
// format. By encapsulating TOML within this class we can support other
// formats (JSON, YAML, etc) without impacting other modules.
class Config {
 public:
  enum class DuplicatesLocation { kNone, kInline, kAppendToGroup };

 public:
  Config();
  Config(Config&) = delete;
  Config(Config&&) = delete;
  Config& operator=(Config&) = delete;
  Config& operator=(Config&&) = delete;

  decltype(auto) get_blocked_tags() {
    return m_blocked_tags | std::ranges::views::all;
  }

  // Returns the names of all channels in group `group_name` in [channels].allow
  std::vector<std::string> get_channel_names_in_group(
      std::string_view group_name);

  // Returns the channel name specified by the user in the configuration file.
  // `channel_original_name` is the name of this channel in the
  // input playlist file.
  const std::string& get_channel_new_name(
      std::string_view channel_original_name);

  // Returns the tags for this channel. They may not be the same as the tags in
  // the input playlist file if they have been modified.
  const std::vector<IptvChannel::Tag>& get_channel_tags(
      std::string_view channel_original_name);

  // Returns an enum representing the value of [channels].duplicates_location
  const DuplicatesLocation& get_duplicates_location() noexcept;

  // Returns the group names from [channels].allow
  std::span<std::string> get_channels_group_names();

  // Returns the filenames specified in [files].input_playlists
  const std::vector<std::string>& get_input_playlists_filenames();

  // Returns the number of channels in [channels].allow in the configuration
  // file
  int get_num_allowed_channels() { return m_allowed_channels.size(); }

  // Returns the number of groups in [groups].allow in the configuration file
  int get_num_allowed_groups() { return m_allowed_groups.size(); }

  // Returns the value of [channels].number_of_duplicates in the configuration
  // file
  int get_max_num_duplicates() noexcept;

  // Returns the value of [files].new_playlist from the configuration file
  const std::string& get_new_playlist_filename() noexcept;

  decltype(auto) get_qualities() {
    return m_qualities | std::ranges::views::all;
  }
  bool is_allowed_channel(std::string_view channel_original_name);
  bool is_allowed_group(std::string_view group_name);
  bool is_blocked_channel(std::string_view original_channel_name);
  bool is_blocked_group(std::string_view group_name);
  bool is_blocked_url(std::string_view url);

 private:
  // Holds the data specified for each channel in [channels].allow
  // in the configuration file
  struct AllowedChannel {
    std::vector<std::string> allow;
    std::vector<std::string> block;
    std::string new_name;
    std::vector<IptvChannel::Tag> tags;
  };

 private:
  void convert_allowed_channels();
  void convert_allowed_channel_tags(toml::v3::table& table,
                                    AllowedChannel& channel);
  void convert_blocked_tags();
  void convert_collections();
  void convert_duplicates();
  void convert_from_toml();
  void convert_input_playlist_filenames();
  void convert_new_playlist_filename();
  void convert_qualities();

  // Returns the data for a channel as specified in [channels].allow.
  // `channel_original_name` is the channel name from the input playlist file.
  const AllowedChannel& get_allowed_channel(
      std::string_view channel_original_name);

 private:
  toml::table m_config;
  DuplicatesLocation m_duplicates_location;
  int m_max_num_duplicates;
  std::vector<std::string> m_input_playlists_filenames;
  std::string m_new_playlist_filename;
  std::unordered_set<std::string> m_blocked_groups;
  std::unordered_set<std::string> m_blocked_channels;
  std::unordered_set<std::string> m_blocked_urls;
  std::unordered_set<std::string> m_allowed_groups;
  std::vector<std::string> m_qualities;
  std::vector<std::string> m_blocked_tags;
  std::vector<std::string> m_group_names;
  std::vector<signed> m_channels_groups_size;
  std::vector<AllowedChannel> m_allowed_channels;
  std::unordered_map<std::string, AllowedChannel*> m_allowed_channels_lookup;
};

}  // namespace pefti

#endif  // PEFTI_CONFIG_H_
