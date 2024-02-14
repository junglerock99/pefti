#pragma once

#include <memory>
#include <span>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"

namespace pefti {

// Contains the application logic for sorting a playlist.
class Sorter {
 public:
  Sorter(std::shared_ptr<Config> config);
  Sorter(Sorter&) = delete;
  Sorter(Sorter&&) = delete;
  Sorter& operator=(Sorter&) = delete;
  Sorter& operator=(Sorter&&) = delete;
  void sort(Playlist& playlist);

 private:
  void mark_duplicates_for_deletion();
  void move_duplicates();
  void partition_channels();
  void remove_quality_tags(Playlist& playlist);
  void remove_tvgid_from_duplicates();
  void remove_unwanted_duplicate_channels(Playlist& playlist);
  void sort_channels();
  void sort_channels_by_quality();
  void sort_duplicates();
  void sort_groups(Playlist& playlist);

 private:
  std::shared_ptr<Config> m_config;
  std::unordered_map<std::string, int> m_qualities_lookup;
  std::vector<std::span<IptvChannel>> m_groups_spans;
  std::vector<std::vector<std::span<IptvChannel>>> m_channels_spans{};
};

}  // namespace pefti
