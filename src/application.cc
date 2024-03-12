#include "application.h"

#include <cxxopts.hpp>

#include "channels_mapper.h"
#include "config.h"
#include "epg.h"
#include "filter.h"
#include "playlist.h"
#include "transformer.h"

namespace pefti {

Application::Application(std::string&& config_filename)
    : m_config(config_filename),
      m_playlist(m_config),
      m_filter(m_config, m_playlist, m_channels_mapper),
      m_transformer(m_config, m_playlist, m_channels_mapper) {
  m_channels_mapper.set_config(m_config);
  m_channels_mapper.set_playlist(m_playlist);
}

// Fiters and transforms IPTV playlists according to the user configuration,
// then filters EPGs. The playlists must be processed first because filtering
// the EPGs depends on channels in the new playlist, the new EPG will only
// contain data for channels in the new playlist. Output is one new playlist
// file and one new EPG file.
void Application::run() {
  //
  // Process playlists
  const auto& playlist_urls = m_config.get_playlists_urls();
  auto playlists = load_playlists(playlist_urls);
  m_filter.filter(std::move(playlists));
  m_channels_mapper.populate_maps();
  m_transformer.transform();
  store_playlist(m_config.get_new_playlist_filename(), 
                 m_playlist, m_config,
                 m_channels_mapper);
  //
  // Process EPGs
  const auto& epg_urls = m_config.get_epgs_urls();
  auto epgs = load_epgs(epg_urls);
  m_filter.filter(std::move(epgs), m_config.get_new_epg_filename());
}

}  // namespace pefti
