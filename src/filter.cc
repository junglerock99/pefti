#include "filter.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <ranges>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

using namespace std::literals;

namespace pefti {

// Filters the input playlists and creates a new playlist
void Filter::filter(std::vector<std::ifstream> in_playlists,
                    Playlist& new_playlist) {
  //
  // Filters out a channel if it is a member of a blocked group
  auto block_group = [this](auto&& channel) {
    return !(m_config->is_blocked_group(channel.get_tag("group-title"sv)));
  };
  //
  // Filters out a channel if its name contains any of the text strings
  // that identify blocked channels
  auto block_channel = [this](auto&& channel) {
    return !(m_config->is_blocked_channel(channel.get_original_name()));
  };
  //
  // Filters out a channel if it has a blocked URL
  auto block_url = [this](auto&& channel) {
    return !(m_config->is_blocked_url(channel.get_url()));
  };
  //
  // Filters out a channel if it is not an allowed channel or in an allowed
  // group. A special case is that if no allowed groups or allowed channels are
  // specified in the configuration then all channels are unfiltered.
  auto allow_groups_and_channels = [this](auto&& channel) {
    const bool are_all_channels_allowed =
        (m_config->get_num_allowed_channels() +
         m_config->get_num_allowed_groups()) == 0;
    bool is_allowed{false};
    if (are_all_channels_allowed) {
      is_allowed = true;
    } else if (m_config->is_allowed_group(channel.get_tag("group-title"sv))) {
      is_allowed = true;
    } else if (m_config->is_allowed_channel(channel.get_original_name())) {
      is_allowed = true;
    } else
      is_allowed = false;
    return is_allowed;
  };
  //
  // Removes tags from a channel if the tags are blocked in the configuration
  auto block_tags = [this](auto&& channel) {
    for (auto tag : m_config->get_blocked_tags()) channel.delete_tag(tag);
    return channel;
  };
  //
  // Filter all channels from the input playlists to create the new playlist
  for (auto& playlist : in_playlists) {
    auto view = std::views::istream<IptvChannel>(playlist) |
                std::views::filter(block_group) |
                std::views::filter(block_channel) |
                std::views::filter(block_url) |
                std::views::filter(allow_groups_and_channels) |
                std::views::transform(block_tags);
    for (auto channel : view) {
      new_playlist.push_back(std::move(channel));
    }
  }
}

}  // namespace pefti
