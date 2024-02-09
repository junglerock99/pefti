#include "playlist.h"

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <curl/curl.h>
#include <curl/mprintf.h>

#include "resource.h"

using namespace std::string_view_literals;

namespace pefti {

// Creates the lookup table on first invocation
bool Playlist::is_tvg_id_in_playlist(std::string_view tvg_id) {
  if (!m_tvg_id_lookup) {
    m_tvg_id_lookup = std::make_optional<std::unordered_set<std::string>>();
    for (auto& channel : m_playlist) {
      const auto value = channel.get_tag_value(IptvChannel::kTagTvgId);
      if (value && (*value != ""))
        m_tvg_id_lookup->insert(*value);
    }
  }
  return m_tvg_id_lookup->contains(std::string{tvg_id});
}

std::vector<std::string> load_playlists(const std::vector<std::string>& urls) {
  return load_resources(urls);
}

void store_playlist(std::string_view filename, Playlist& playlist) {
  std::ofstream file{std::string(filename)};
  file << "#EXTM3U\n";
  for (auto& channel : playlist) file << channel;
  file.close();
}

}  // namespace pefti
