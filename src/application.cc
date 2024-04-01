#include "application.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cppcoro/single_consumer_async_auto_reset_event.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/when_all.hpp>
#include <cxxopts.hpp>
#include <string>
#include <vector>

#include "buffers.h"
#include "config.h"
#include "epg.h"
#include "filter.h"
#include "iptv_channel.h"
#include "mapper.h"
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

cppcoro::task<> Application::process_epgs(
    cppcoro::static_thread_pool& thread_pool) {
  co_await thread_pool.schedule();
  const auto& epg_urls = m_config.get_epgs_urls();
  auto epgs = load_epgs(epg_urls);
  co_await m_have_iptv_channels;
  m_filter.filter(std::move(epgs), m_config.get_new_epg_filename());
}

cppcoro::task<> Application::process_playlists(
    cppcoro::static_thread_pool& thread_pool) {
  co_await thread_pool.schedule();
  const auto& playlist_urls = m_config.get_playlists_urls();
  auto playlists = load_playlists(playlist_urls);
  PlaylistParserFilterBuffer pf_buffer;
  co_await cppcoro::when_all(
      m_parser.parse_iptv_channels(thread_pool, pf_buffer, playlists),
      m_filter.filter_iptv_channels(thread_pool, pf_buffer));
  m_channels_mapper.populate_maps();
  m_transformer.transform();
  m_have_iptv_channels.set();
  store_playlist(m_config.get_new_playlist_filename(), m_playlist, m_config,
                 m_channels_mapper);
}

// Fiters and transforms IPTV playlists and creates a new playlist according
// to the user configuration. Also filters EPGs and creates a new EPG which
// will only contain data for channels in the new playlist.
// Output is one new playlist file and one new EPG file.
void Application::run() {
  cppcoro::static_thread_pool thread_pool;
  cppcoro::sync_wait(cppcoro::when_all(process_playlists(thread_pool),
                                       process_epgs(thread_pool)));
}

}  // namespace pefti
