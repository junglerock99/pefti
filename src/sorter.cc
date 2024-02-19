#include "sorter.h"

#include <algorithm>
#include <gsl/gsl>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "iptv_channel.h"
#include "playlist.h"

using namespace std::literals;

namespace pefti {

Sorter::Sorter(std::shared_ptr<Config> config) : m_config(config) {}

void Sorter::sort(Playlist& playlist) {
  GroupSpans g_spans;
  sort_groups(playlist, g_spans);
  GroupedChannelSpans gc_spans;
  sort_channels(g_spans, gc_spans);
  sort_duplicates(g_spans, std::move(gc_spans));
  remove_unwanted_duplicate_channels(playlist);
  remove_quality_tags(playlist);
}

// Return a mapping of channel qualities to priorities to
// enable ordering channels by quality
std::unordered_map<std::string, int> Sorter::get_qualities_priorities_map() {
  std::unordered_map<std::string, int> map;
  int i{};
  const auto qualities = m_config->get_qualities();
  for (const auto& quality : qualities) map[quality] = i++;
  map[""s] = i;
  Ensures(map.size() == (qualities.size() + 1));
  return map;
}

// When there are multiple instances of the same channel, the
// `number_of_duplicates` configuration setting determines how many instances
// are copied to the new playlist. Add a kTagDelete tag to extra duplicate
// channels so that they can be found and deleted later.
void Sorter::mark_duplicates_for_deletion(GroupedChannelSpans& gc_spans) {
  const auto max_num_duplicates = m_config->get_max_num_duplicates();
  for (const auto& channel_spans : gc_spans) {
    for (const auto& channel_span : channel_spans) {
      const auto num_instances = std::ranges::distance(channel_span);
      if (num_instances > (1 + max_num_duplicates)) {
        std::ranges::for_each(channel_span.begin() + 1 + max_num_duplicates,
                              channel_span.end(), [](auto&& channel) {
                                channel.set_tag(IptvChannel::kTagDelete,
                                                "yes"s);
                              });
      }
    }
  }
}

// When there are multiple instances of the same channel,
// the "duplicates_location" configuration setting determines where the
// duplicate channels appear in the new playlist. For the "append_to_group"
// setting, the duplicates channels are moved to the end of the group by
// rotating the group.
void Sorter::move_duplicates(GroupSpans& g_spans,
                             GroupedChannelSpans& gc_spans) {
  const auto& duplicates_location = m_config->get_duplicates_location();
  for (std::size_t i{}; i < g_spans.size(); ++i) {
    auto rotate_span = g_spans[i];
    for (const auto& channel_span : gc_spans[i]) {
      const auto num_instances = std::ranges::distance(channel_span);
      if (duplicates_location == Config::DuplicatesLocation::kAppendToGroup) {
        if (num_instances > 0) {
          rotate_span = std::span(rotate_span.begin() + 1, rotate_span.end());
        }
        if (num_instances > 1) {
          std::ranges::rotate(rotate_span,
                              rotate_span.begin() + (num_instances - 1));
        }
      }
    }
  }
}

// Sorts channels within groups
void Sorter::partition_channels(GroupSpans& g_spans,
                                GroupedChannelSpans& gc_spans) {
  const auto group_names = m_config->get_channels_group_names();
  gc_spans.reserve(group_names.size());
  auto spans = g_spans.begin();
  for (auto names = group_names.begin(); names != group_names.end();
       names++, spans++) {
    auto channels_subrange = *spans;
    const auto channel_names = m_config->get_channel_names_in_group(*names);
    ChannelSpans channel_spans;
    for (const auto& channel_name : channel_names) {
      const auto channel_start = channels_subrange.begin();
      channels_subrange = std::ranges::partition(
          channels_subrange, [&channel_name](auto&& channel) {
            return channel.get_new_name() == channel_name;
          });
      channel_spans.push_back(
          ChannelSpan{channel_start, channels_subrange.begin()});
    }
    gc_spans.push_back(std::move(channel_spans));
  }
}

void Sorter::remove_quality_tags(Playlist& playlist) noexcept {
  std::ranges::for_each(playlist, [](auto&& channel) {
    channel.delete_tag(IptvChannel::kTagQuality);
  });
}

// When there are multiple instances of the same channel,
// remove the `tvg-id` tag from duplicate instances.
// This reduces the size of the EPG file and should prevent the
// media player from displaying the same EPG data multiple times.
void Sorter::remove_tvgid_from_duplicates(GroupedChannelSpans& gc_spans) {
  for (const auto& channel_spans : gc_spans) {
    for (const auto& channel_span : channel_spans) {
      const auto num_instances = std::ranges::distance(channel_span);
      if (num_instances > 1) {
        std::ranges::for_each(
            channel_span.begin() + 1, channel_span.end(),
            [](auto&& channel) { channel.delete_tag(IptvChannel::kTagTvgId); });
      }
    }
  }
}

void Sorter::remove_unwanted_duplicate_channels(Playlist& playlist) {
  playlist.erase(
      std::remove_if(playlist.begin(), playlist.end(),
                     [](auto&& channel) {
                       return channel.contains_tag(IptvChannel::kTagDelete);
                     }),
      playlist.end());
}

void Sorter::sort_channels(GroupSpans& g_spans, GroupedChannelSpans& gc_spans) {
  partition_channels(g_spans, gc_spans);
  sort_channels_by_quality(gc_spans);
}

void Sorter::sort_channels_by_quality(GroupedChannelSpans& gc_spans) {
  auto map = get_qualities_priorities_map();
  for (const auto& channel_spans : gc_spans) {
    for (const auto& channel_span : channel_spans) {
      std::ranges::sort(channel_span, [&map](auto&& lhs, auto&& rhs) {
        auto tag = IptvChannel::kTagQuality;
        return map[*(lhs.get_tag_value(tag))] < map[*(rhs.get_tag_value(tag))];
      });
    }
  }
}

void Sorter::sort_duplicates(GroupSpans& g_spans,
                             GroupedChannelSpans&& gc_spans) {
  mark_duplicates_for_deletion(gc_spans);
  remove_tvgid_from_duplicates(gc_spans);
  move_duplicates(g_spans, gc_spans);
}

void Sorter::sort_groups(Playlist& playlist, GroupSpans& g_spans) {
  const auto group_names = m_config->get_channels_group_names();
  auto groups_subrange = std::ranges::subrange(playlist);
  auto group_begin = groups_subrange.begin();
  for (std::string_view group_name : group_names) {
    groups_subrange = std::ranges::stable_partition(
        groups_subrange.begin(), groups_subrange.end(),
        [&group_name](auto&& channel) {
          auto group_title = IptvChannel::kTagGroupTitle;
          return *(channel.get_tag_value(group_title)) == group_name;
        });
    std::size_t group_size =
        std::distance(group_begin, groups_subrange.begin());
    std::span<IptvChannel> group_subrange{group_begin, group_size};
    g_spans.push_back(group_subrange);
    group_begin = groups_subrange.begin();
  }
}

}  // namespace pefti
