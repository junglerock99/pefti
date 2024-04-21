#include "transformer.h"

#include <algorithm>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <execution>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

using namespace std::literals;

namespace pefti {

void Transformer::transform() {
  if (config_.get_num_channels_templates() > 0) {
    copy_group_title_tag();
    order_by_sort_criteria();
  }
}

cppcoro::task<> Transformer::transform(
    cppcoro::static_thread_pool& tp, PlaylistFilterTransformerBuffer& buffer) {
  const auto kIndexMask = buffer.get_index_mask();
  size_t read_index{0};
  bool received_sentinel{false};
  while (!received_sentinel) {
    const size_t write_index =
        co_await buffer.sequencer.wait_until_published(read_index, tp);
    do {
      auto& channel = buffer.data[read_index & kIndexMask];
      const bool is_sentinel = channel.get_original_name() == kSentinel;
      if (!is_sentinel) {
        copy_tags(channel);
        block_tags(channel);
        set_name(channel);
        co_await playlist_.push_back(std::move(channel));
      } else {
        received_sentinel = true;
        break;
      }
    } while (read_index++ != write_index);
    buffer.barrier.publish(read_index);
  }
}

void Transformer::block_tags(IptvChannel& channel) {
  for (auto tag : config_.get_blocked_tags()) channel.delete_tag(tag);
}

// If enabled in the configuration, channels without a group-title tag entry
// in the channel template will use the group-title from the previous template
// in the channel templates list.
void Transformer::copy_group_title_tag() {
  if (!config_.get_copy_group_title_flag()) return;
  std::string_view previous_group_title = ""sv;
  auto channels_templates = config_.get_channels_templates();
  for (auto& ct : channels_templates) {
    auto group_title = previous_group_title;
    for (auto& tag : ct.tags) {
      if (tag.first == IptvChannel::kTagGroupTitle) {
        group_title = tag.second;
        break;
      }
    }
    previous_group_title = group_title;
    auto& channels = channels_mapper_.map_template_to_channel(ct);
    for (auto channel : channels)
      channel->set_tag(IptvChannel::kTagGroupTitle, group_title);
  }
}

void Transformer::copy_tags(IptvChannel& channel) {
  if (channels_mapper_.is_allowed_channel(channel))
    channel.set_tags(channels_mapper_.get_channel_template(channel).tags);
}

// The channels in the playlist are not moved, pointers to the channels in
// channels_mapper are moved.
void Transformer::order_by_sort_criteria() {
  auto channels_templates = config_.get_channels_templates();
  const auto sort_qualities = config_.get_sort_qualities();
  for (auto& ct : channels_templates) {
    auto& channels = channels_mapper_.map_template_to_channel(ct);
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

void Transformer::set_name(IptvChannel& channel) {
  if (channels_mapper_.is_allowed_channel(channel))
    channel.set_new_name(
        channels_mapper_.get_channel_template(channel).new_name);
}

}  // namespace pefti
