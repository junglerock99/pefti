#include "playlist.h"

#include <curl/curl.h>
#include <curl/mprintf.h>

#include <algorithm>
#include <exception>
#include <fstream>
#include <gsl/gsl>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "resource.h"

using namespace std::string_view_literals;

namespace pefti {

// Creates the lookup table on first invocation
bool Playlist::is_tvg_id_in_playlist(std::string_view tvg_id) {
  if (tvg_id == ""sv) return false;
  if (!m_tvg_id_lookup) {
    m_tvg_id_lookup = std::make_optional<std::unordered_set<std::string>>();
    for (auto& channel : m_playlist) {
      const auto value = channel.get_tag_value(IptvChannel::kTagTvgId);
      if (value && (*value != "")) m_tvg_id_lookup->insert(*value);
    }
  }
  Ensures(m_tvg_id_lookup.has_value());
  Ensures(!m_tvg_id_lookup->empty());
  return m_tvg_id_lookup->contains(std::string{tvg_id});
}

std::vector<std::string> load_playlists(const std::vector<std::string>& urls) {
  return load_resources(urls);
}

void store_playlist(std::string_view filename, Playlist& playlist,
                    ConfigType& config, ChannelsMapper& channels_mapper) {
  Expects(filename != ""sv);
  std::ofstream file(std::string{filename});
  if (!file)
    throw std::runtime_error("Failed to create/open new playlist file"s);
  file << "#EXTM3U\n";
  const auto num_duplicates =
      static_cast<std::size_t>(config.get_num_duplicates());
  const auto duplicates_location = config.get_duplicates_location();
  auto write_duplicates = [&file, &num_duplicates](auto& channels) {
    const auto num_to_write =
        (channels.size() > 0) ? std::min(num_duplicates, channels.size() - 1)
                              : 0;
    if (num_to_write > 0) {
      for (std::size_t i{1}; i <= num_to_write; ++i) {
        (*(channels[i])).delete_tag(IptvChannel::kTagTvgId);
        file << *(channels[i]);
      }
    }
  };
  //
  // Write highest priority instance of each channel, plus inline duplicates
  auto channels_templates = config.get_channels_templates();
  for (auto& ct : channels_templates) {
    auto& channels = channels_mapper.map_template_to_channel(ct);
    if (channels.size() > 0) file << *(channels[0]);
    if (duplicates_location == ConfigType::DuplicatesLocation::kInline)
      write_duplicates(channels);
  }
  if (duplicates_location == ConfigType::DuplicatesLocation::kAppend) {
    // Append duplicates
    for (auto& ct : channels_templates) {
      auto& channels = channels_mapper.map_template_to_channel(ct);
      write_duplicates(channels);
    }
  }
  //
  // Write allowed groups
  auto allowed_groups = config.get_allowed_groups();
  for (auto& group : allowed_groups) {
    for (auto& channel : playlist) {
      auto value = channel.get_tag_value(IptvChannel::kTagGroupTitle);
      if (value && value == group) {
        if (!channels_mapper.is_allowed_channel(channel)) file << channel;
      }
    }
  }
  file.close();
}

}  // namespace pefti
