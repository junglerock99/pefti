#include "playlist.h"

#include <algorithm>
#include <fstream>
#include <string_view>

#include "iptv_channel.h"

namespace pefti {

std::vector<std::ifstream> load_playlists(
    const std::vector<std::string>& filenames) {
  std::vector<std::ifstream> streams;
  std::ranges::for_each(filenames, [&streams](auto&& filename) {
    streams.emplace_back(std::ifstream{filename});
  });
  return streams;
}

void store_playlist(std::string_view filename, Playlist& playlist) {
  std::ofstream file{std::string(filename)};
  file << "#EXTM3U\n";
  for (auto& channel : playlist) file << channel;
  file.close();
}

}  // namespace pefti
