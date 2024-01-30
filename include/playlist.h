#ifndef PEFTI_PLAYLIST_H_
#define PEFTI_PLAYLIST_H_

#include <vector>

#include "iptv_channel.h"

namespace pefti {

class Playlist {
 private:
  using vi = std::vector<IptvChannel>::iterator;

 private:
  std::vector<IptvChannel> m_playlist;

 public:
  Playlist() = default;
  Playlist(int size) { m_playlist.reserve(size); }
  decltype(m_playlist.begin()) begin() { return m_playlist.begin(); }
  decltype(m_playlist.cbegin()) cbegin() { return m_playlist.cbegin(); }
  decltype(m_playlist.cend()) cend() { return m_playlist.cend(); }
  decltype(m_playlist.end()) end() { return m_playlist.end(); }
  vi erase(vi begin, vi end) { return m_playlist.erase(begin, end); }
  void push_back(IptvChannel channel) { m_playlist.push_back(channel); }
};

std::vector<std::string> load_playlists(
  const std::vector<std::string>& filenames);
void store_playlist(std::string_view filename, Playlist& playlist);

}  // namespace pefti

#endif  // PEFTI_PLAYLIST_H_
