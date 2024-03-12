#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "channels_mapper.h"
#include "config.h"
#include "iptv_channel.h"

namespace pefti {

class Playlist {
 private:
  using vi = std::vector<IptvChannel>::iterator;

 private:
  std::vector<IptvChannel> m_playlist;
  std::optional<std::unordered_set<std::string>> m_tvg_id_lookup{};

 public:
  Playlist(ConfigType& config) : m_config(config) {}
  decltype(m_playlist.begin()) begin() { return m_playlist.begin(); }
  decltype(m_playlist.cbegin()) cbegin() { return m_playlist.cbegin(); }
  decltype(m_playlist.cend()) cend() { return m_playlist.cend(); }
  decltype(m_playlist.empty()) empty() { return m_playlist.empty(); }
  decltype(m_playlist.end()) end() { return m_playlist.end(); }
  vi erase(vi begin, vi end) { return m_playlist.erase(begin, end); }
  bool is_tvg_id_in_playlist(std::string_view tvg_id);
  void push_back(IptvChannel channel) { m_playlist.push_back(channel); }
  decltype(m_playlist.size()) size() { return m_playlist.size(); }

 private:
  ConfigType& m_config;
};

std::vector<std::string> load_playlists(const std::vector<std::string>& urls);
void store_playlist(std::string_view filename, Playlist& playlist,
                    ConfigType& config, ChannelsMapper& channels_mapperper);

}  // namespace pefti
