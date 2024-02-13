#include "sorter.h"

#include <algorithm>
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

Sorter::Sorter(std::shared_ptr<Config> config) : m_config(config) {
  // Create lookup table for qualities
  int i{};
  for (auto quality : m_config->get_qualities())
    m_qualities_lookup[quality] = i++;
  m_qualities_lookup[""s] = i;
}

void Sorter::sort(Playlist& playlist) {
  sort_groups(playlist);
  sort_channels();
  sort_duplicates();
  remove_unwanted_duplicate_channels(playlist);
  remove_quality_tags(playlist);
}

// When there are multiple instances of the same channel, the
// "number_of_duplicates" configuration setting determines how many instances
// are copied to the new playlist. The remaining instances have a tag added
// to the channel so that they can be deleted later.
void Sorter::mark_duplicates_for_deletion() {
  const auto max_num_duplicates = m_config->get_max_num_duplicates();
  for (std::size_t i{}; i < m_groups_spans.size(); ++i) {
    auto rotate_span = m_groups_spans[i];
    for (const auto& channel_span : m_channels_spans[i]) {
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
// channels appear in the new playlist. For the "append_to_group" setting, the
// duplicates are moved to the end of the group by rotating the group.
void Sorter::move_duplicates() {
  const auto& duplicates_location = m_config->get_duplicates_location();
  for (std::size_t i{}; i < m_groups_spans.size(); ++i) {
    auto rotate_span = m_groups_spans[i];
    for (const auto& channel_span : m_channels_spans[i]) {
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

void Sorter::remove_quality_tags(Playlist& playlist) {
  std::ranges::for_each(playlist, [](auto&& channel) {
    channel.delete_tag(IptvChannel::kTagQuality);
  });
}

void Sorter::remove_unwanted_duplicate_channels(Playlist& playlist) {
  playlist.erase(
      std::remove_if(playlist.begin(), playlist.end(),
                     [](auto&& channel) {
                       return channel.contains_tag(IptvChannel::kTagDelete);
                     }),
      playlist.end());
}

// Partitions channels within groups. This must be done after partitioning
// groups. Saves channels subranges data for future sorting.
void Sorter::partition_channels() {
  auto group_names = m_config->get_channels_group_names();
  m_channels_spans.reserve(group_names.size());
  for (std::size_t i{}; i < m_groups_spans.size(); ++i) {
    std::vector<std::span<IptvChannel>> spans;
    auto channels_subrange = m_groups_spans[i];
    for (const auto& channel_name :
         m_config->get_channel_names_in_group(group_names[i])) {
      auto channel_start = channels_subrange.begin();
      channels_subrange = std::ranges::partition(
          channels_subrange, [&channel_name](auto&& channel) {
            return channel.get_new_name() == channel_name;
          });
      spans.push_back(
          std::span<IptvChannel>{channel_start, channels_subrange.begin()});
    }
    m_channels_spans.push_back(std::move(spans));
  }
}

void Sorter::sort_channels() {
  partition_channels();
  sort_channels_by_quality();
}

void Sorter::sort_channels_by_quality() {
  for (const auto& group : m_channels_spans) {
    for (const auto& channel_span : group) {
      std::ranges::sort(channel_span, [this](auto&& lhs, auto&& rhs) {
        auto quality = IptvChannel::kTagQuality;
        return m_qualities_lookup[*(lhs.get_tag_value(quality))] <
               m_qualities_lookup[*(rhs.get_tag_value(quality))];
      });
    }
  }
}

void Sorter::sort_duplicates() {
  mark_duplicates_for_deletion();
  move_duplicates();
}

// Partitions groups, this must be done before partitioning channels. Saves
// group subranges data for future sorting.
void Sorter::sort_groups(Playlist& playlist) {
  auto groups_subrange = std::ranges::subrange(playlist);
  auto group_names = m_config->get_channels_group_names();
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
    m_groups_spans.push_back(group_subrange);
    group_begin = groups_subrange.begin();
  }
}

}  // namespace pefti
