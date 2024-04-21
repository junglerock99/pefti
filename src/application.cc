#include "application.h"

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
#include "loader.h"
#include "mapper.h"
#include "playlist.h"
#include "transformer.h"

// For each input playlist, there is a pipeline of coroutines consisting of
// Loader, Parser, Filter and Transformer stages. There is a ring buffer
// between each stage. Loader, Parser and Filter are producers. Parser,
// Filter and Transformer are consumers.
/*
--------------   ----------   ----------   ----------   ----------   ---------    ----------   ---------------
| Playlist 1 |-->| Loader |-->| Buffer |-->| Parser |-->| Buffer |-->| Filter |-->| Buffer |-->| Transformer |\
--------------   ----------   ----------   ----------   ----------   ----------   ----------   --------------- \
                                                                                                                ------------
     ...            ...           ...          ...          ...           ...         ...              ...      | Playlist |
                                                                                                                ------------
--------------   ----------   ----------   ----------   ----------   ----------   ----------   --------------- /
| Playlist n |-->| Loader |-->| Buffer |-->| Parser |-->| Buffer |-->| Filter |-->| Buffer |-->| Transformer |/
--------------   ----------   ----------   ----------   ----------   ----------   ----------   ---------------
*/

namespace pefti {

Application::Application(std::string&& config_filename)
    : config_(config_filename),
      playlist_(config_),
      filter_(config_, playlist_, channels_mapper_),
      transformer_(config_, playlist_, channels_mapper_) {
  channels_mapper_.set_config(config_);
  channels_mapper_.set_playlist(playlist_);
}

// Filters EPGs and creates a new EPG which will only contain data for channels
// that are in the new playlist. Output is one new EPG file.
[[nodiscard]] cppcoro::task<> Application::process_epgs(
    cppcoro::static_thread_pool& tp) {
  co_await tp.schedule();
  const auto& epg_urls = config_.get_epgs_urls();
  auto epgs = load_epgs(epg_urls);
  co_await have_iptv_channels_;
  filter_.filter(std::move(epgs), config_.get_new_epg_filename());
}

// Fiters and transforms IPTV playlists and creates a new playlist according
// to the user configuration. Output is one new playlist file.
[[nodiscard]] cppcoro::task<> Application::process_playlists(
    cppcoro::static_thread_pool& tp) {
  co_await tp.schedule();
  const auto& playlist_urls = config_.get_playlists_urls();
  std::vector<PlaylistLoaderParserBuffer> lp_buffers(playlist_urls.size());
  std::vector<PlaylistParserFilterBuffer> pf_buffers(playlist_urls.size());
  std::vector<PlaylistFilterTransformerBuffer> ft_buffers(playlist_urls.size());
  std::vector<cppcoro::task<>> tasks;
  for (size_t i{0}; i < playlist_urls.size(); ++i) {
    tasks.push_back(
        std::move(loader_.load(tp, lp_buffers[i], playlist_urls[i])));
    tasks.push_back(std::move(parser_.parse(tp, lp_buffers[i], pf_buffers[i])));
    tasks.push_back(
        std::move(filter_.filter(tp, pf_buffers[i], ft_buffers[i])));
    tasks.push_back(std::move(transformer_.transform(tp, ft_buffers[i])));
  }
  co_await cppcoro::when_all(std::move(tasks));
  channels_mapper_.populate_maps();
  have_iptv_channels_.set();
  transformer_.transform();
  store_playlist(config_.get_new_playlist_filename(), playlist_, config_,
                 channels_mapper_);
}

void Application::run() {
  cppcoro::static_thread_pool thread_pool;
  cppcoro::sync_wait(cppcoro::when_all(process_playlists(thread_pool),
                                       process_epgs(thread_pool)));
}

}  // namespace pefti
