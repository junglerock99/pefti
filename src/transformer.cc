#include "transformer.h"

#include <algorithm>
#include <execution>
#include <memory>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

using namespace std::literals;

namespace pefti {

void Transformer::transform(Playlist& playlist) {
  if (m_config->get_num_allowed_channels() > 0) {
    set_tags(playlist);
    set_quality(playlist);
    set_name(playlist);
  }
}

// Sets a channel's name so that it appears in the new playlist with the name
// specified in the configuration instead of the original name from the
// old playlist.
void Transformer::set_name(Playlist& playlist) {
  auto set_name = [this](auto&& channel) {
    if (m_config->is_allowed_channel(channel.get_original_name()))
      channel.set_new_name(
          m_config->get_channel_new_name(channel.get_original_name()));
  };
  std::for_each(std::execution::par, playlist.begin(), playlist.end(),
                set_name);
}

// Determines the quality for a channel by comparing the channel's name with
// the quality text strings specified in the configuration. Creates a new tag
// containing the quality which will be used for sorting and then deleted after
// sorting.
void Transformer::set_quality(Playlist& playlist) {
  auto set_quality = [this](auto&& channel) {
    if (m_config->is_allowed_channel(channel.get_original_name())) {
      auto& channel_name{channel.get_original_name()};
      channel.set_tag("quality"sv, ""sv);
      for (const auto& quality : m_config->get_qualities()) {
        if (channel_name.find(quality) != std::string::npos) {
          channel.set_tag("quality"sv, quality);
          break;
        }
      }
    }
  };
  std::for_each(std::execution::par, playlist.begin(), playlist.end(),
                set_quality);
}

// Copies the tags for a channel from the configuration to the new playlist
void Transformer::set_tags(Playlist& playlist) {
  auto set_tags = [this](auto&& channel) {
    if (m_config->is_allowed_channel(channel.get_original_name()))
      channel.set_tags(m_config->get_channel_tags(channel.get_original_name()));
  };
  std::for_each(std::execution::par, playlist.begin(), playlist.end(),
                set_tags);
}

}  // namespace pefti
