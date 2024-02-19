#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "iptv_channel.h"

namespace pefti {

class Playlist {
 private:
  using vi = std::vector<IptvChannel>::iterator;

 private:
  std::vector<IptvChannel> m_playlist;
  std::optional<std::unordered_set<std::string>> m_tvg_id_lookup{};

 public:
  Playlist() = default;
  explicit Playlist(int size) { m_playlist.reserve(size); }
  decltype(m_playlist.begin()) begin() { return m_playlist.begin(); }
  decltype(m_playlist.cbegin()) cbegin() { return m_playlist.cbegin(); }
  decltype(m_playlist.cend()) cend() { return m_playlist.cend(); }
  decltype(m_playlist.empty()) empty() { return m_playlist.empty(); }
  decltype(m_playlist.end()) end() { return m_playlist.end(); }
  vi erase(vi begin, vi end) { return m_playlist.erase(begin, end); }
  bool is_tvg_id_in_playlist(std::string_view tvg_id);
  void push_back(IptvChannel channel) { m_playlist.push_back(channel); }
  decltype(m_playlist.size()) size() { return m_playlist.size(); }
};

std::vector<std::string> load_playlists(const std::vector<std::string>& urls);
void store_playlist(std::string_view filename, Playlist& playlist);

}  // namespace pefti
