#pragma once

#include <cppcoro/task.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "mapper.h"

namespace pefti {

class Playlist {
 private:
  using vi = std::vector<IptvChannel>::iterator;

 private:
  ConfigType& config_;
  std::vector<IptvChannel> playlist_;
  std::optional<std::unordered_set<std::string>> tvg_id_lookup_{};

 public:
  Playlist(ConfigType& config) : config_(config) {}
  decltype(playlist_.begin()) begin() { return playlist_.begin(); }
  decltype(playlist_.cbegin()) cbegin() { return playlist_.cbegin(); }
  decltype(playlist_.cend()) cend() { return playlist_.cend(); }
  decltype(playlist_.empty()) empty() { return playlist_.empty(); }
  decltype(playlist_.end()) end() { return playlist_.end(); }
  vi erase(vi begin, vi end) { return playlist_.erase(begin, end); }
  bool is_tvg_id_in_playlist(std::string_view tvg_id);
  cppcoro::task<> push_back(IptvChannel channel);
  decltype(playlist_.size()) size() { return playlist_.size(); }
};

std::vector<std::string> load_playlists(const std::vector<std::string>& urls);
void store_playlist(std::string_view filename, Playlist& playlist,
                    ConfigType& config, ChannelsMapper& channels_mapperper);

}  // namespace pefti
