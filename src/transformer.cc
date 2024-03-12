#include "transformer.h"

#include <algorithm>
#include <execution>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

using namespace std::literals;

namespace pefti {

void Transformer::transform() {
  if (m_config.get_num_channels_templates() > 0) {
    copy_tags();
    copy_group_title_tag();
    block_tags();
    set_name();
    order_by_sort_criteria();
  }
}

void Transformer::block_tags() {
  auto block_tags = [this](auto&& channel) {
    for (auto tag : m_config.get_blocked_tags()) channel.delete_tag(tag);
  };
  std::for_each(std::execution::par, m_playlist.begin(), m_playlist.end(),
                block_tags);
}

// If enabled in the configuration, channels without a group-title tag entry
// in the channel template will use the group-title from the previous template
// in the channel templates list.
void Transformer::copy_group_title_tag() {
  if (!m_config.get_copy_group_title_flag()) return;
  std::string_view previous_group_title = ""sv;
  auto channels_templates = m_config.get_channels_templates();
  for (auto& ct : channels_templates) {
    auto group_title = previous_group_title;
    for (auto& tag : ct.tags) {
      if (tag.first == IptvChannel::kTagGroupTitle) {
        group_title = tag.second;
        break;
      }
    }
    previous_group_title = group_title;
    auto& channels = m_channels_mapper.map_template_to_channel(ct);
    for (auto channel : channels)
      channel->set_tag(IptvChannel::kTagGroupTitle, group_title);
  }
}

// Copies the tags for each channel from the configuration to the new playlist
void Transformer::copy_tags() {
  auto set_tags = [this](auto&& channel) {
    if (m_channels_mapper.is_allowed_channel(channel))
      channel.set_tags(m_channels_mapper.get_channel_template(channel).tags);
  };
  std::for_each(std::execution::par, 
                m_playlist.begin(), 
                m_playlist.end(),
                set_tags);
}

// The channels in the playlist are not moved, pointers to the channels in
// channels_mapper are moved.
void Transformer::order_by_sort_criteria() {
  auto channels_templates = m_config.get_channels_templates();
  const auto sort_qualities = m_config.get_sort_qualities();
  for (auto& ct : channels_templates) {
    auto& channels = m_channels_mapper.map_template_to_channel(ct);
    // Construct map
    std::unordered_map<IptvChannel*, int> channel_to_priority_map;
    for (auto channel : channels) {
      auto& name = channel->get_original_name();
      int i{};
      for (auto quality : sort_qualities) {
        if (name.find(quality) != std::string::npos) {
          break;
        }
        ++i;
      }
      channel_to_priority_map[channel] = i;
    }
    std::ranges::sort(channels, [&channel_to_priority_map](IptvChannel* lhs,
                                                           IptvChannel* rhs) {
      return channel_to_priority_map[lhs] < channel_to_priority_map[rhs];
    });
  }
}

// Sets each channel's name so that it appears in the new playlist with the
// name specified in the configuration instead of the original name from the
// input playlist.
void Transformer::set_name() {
  auto set_name = [this](auto&& channel) {
    if (m_channels_mapper.is_allowed_channel(channel))
      channel.set_new_name(
          m_channels_mapper.get_channel_template(channel).new_name);
  };
  std::for_each(std::execution::par, 
                m_playlist.begin(), 
                m_playlist.end(),
                set_name);
}

}  // namespace pefti
