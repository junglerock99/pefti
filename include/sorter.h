#pragma once

#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

namespace pefti {

// Contains the application logic for sorting a playlist
class Sorter {
 public:
  explicit Sorter(std::shared_ptr<Config> config);
  Sorter(Sorter&) = delete;
  Sorter(Sorter&&) = delete;
  Sorter& operator=(Sorter&) = delete;
  Sorter& operator=(Sorter&&) = delete;
  void sort(Playlist& playlist);

 private:
  using GroupSpan = std::span<IptvChannel>;
  using GroupSpans = std::vector<GroupSpan>;
  using ChannelSpan = std::span<IptvChannel>;
  using ChannelSpans = std::vector<ChannelSpan>;
  using GroupedChannelSpans = std::vector<ChannelSpans>;

 private:
  std::unordered_map<std::string, int> get_qualities_priorities_map();
  void mark_duplicates_for_deletion(GroupedChannelSpans& gc_spans);
  void move_duplicates(GroupSpans& g_spans, GroupedChannelSpans& gc_spans);
  void partition_channels(GroupSpans& g_spans, GroupedChannelSpans& gc_spans);
  void remove_quality_tags(Playlist& playlist) noexcept;
  void remove_tvgid_from_duplicates(GroupedChannelSpans& gc_span);
  void remove_unwanted_duplicate_channels(Playlist& playlist);
  void sort_channels(GroupSpans& g_spans, GroupedChannelSpans& gc_spans);
  void sort_channels_by_quality(GroupedChannelSpans& gc_spans);
  void sort_duplicates(GroupSpans& g_spans, GroupedChannelSpans&& gc_spans);
  void sort_groups(Playlist& playlist, GroupSpans& g_spans);

 private:
  std::shared_ptr<Config> m_config;
};

}  // namespace pefti
