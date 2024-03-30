#pragma once

#include <cppcoro/single_consumer_async_auto_reset_event.hpp>
#include <cppcoro/single_producer_sequencer.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>

#include "config.h"
#include "filter.h"
#include "mapper.h"
#include "parser.h"
#include "playlist.h"
#include "transformer.h"

namespace pefti {

class Application {
 public:
  Application(std::string&& config_filename);
  Application(const Application&) = delete;
  Application(const Application&&) = delete;
  void operator=(Application&) = delete;
  void operator=(Application&&) = delete;
  void run();

 private:
  cppcoro::task<> process_playlists(cppcoro::static_thread_pool& tp);
  cppcoro::task<> process_epgs(cppcoro::static_thread_pool& tp);

 private:
  ConfigType m_config;
  Playlist m_playlist;
  Parser m_parser;
  Filter m_filter;
  Transformer m_transformer;
  ChannelsMapper m_channels_mapper;
  cppcoro::single_consumer_async_auto_reset_event m_have_iptv_channels;
};

}  // namespace pefti
