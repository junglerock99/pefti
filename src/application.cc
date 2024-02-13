#include "application.h"

#include <cxxopts.hpp>
#include <memory>

#include "config.h"
#include "epg.h"
#include "filter.h"
#include "playlist.h"
#include "sorter.h"
#include "transformer.h"

namespace pefti {

Application::Application(int argc, char* argv[]) {
  cxxopts::Options options("pefti",
                           "Playlist and EPG Filter/Transformer for IPTV");
  options.add_options()("config", "The configuration file",
                        cxxopts::value<std::string>());
  options.parse_positional({"config"});
  auto result = options.parse(argc, argv);
  auto config_filename = result["config"].as<std::string>();
  m_config = std::make_shared<Config>(std::move(config_filename));
  m_filter = std::make_unique<Filter>(m_config);
  m_transformer = std::make_unique<Transformer>(m_config);
  m_sorter = std::make_unique<Sorter>(m_config);
}

// Fiters, transforms and sorts IPTV playlists according to the user
// configuration, then filters EPGs. The playlists must be processed first
// because filtering the EPGs depends on channels in the new playlist,
// the new EPG will only contain data for channels in the new playlist.
// Output is one new playlist file and one new EPG file.
void Application::run() {
  //
  // Process playlists
  Playlist new_playlist;
  const auto& playlist_urls = m_config->get_playlists_urls();
  auto playlists = load_playlists(playlist_urls);
  m_filter->filter(std::move(playlists), new_playlist);
  m_transformer->transform(new_playlist);
  m_sorter->sort(new_playlist);
  store_playlist(m_config->get_new_playlist_filename(), new_playlist);
  //
  // Process EPGs
  const auto& epg_urls = m_config->get_epgs_urls();
  auto epgs = load_epgs(epg_urls);
  m_filter->filter(std::move(epgs), m_config->get_new_epg_filename(),
                   new_playlist);
}

}  // namespace pefti
