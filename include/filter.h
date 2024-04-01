#pragma once

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "buffers.h"
#include "config.h"
#include "epg.h"
#include "iptv_channel.h"
#include "mapper.h"
#include "playlist.h"

namespace pefti {

// Application logic for filtering playlists and EPGs
class Filter {
 public:
  Filter(ConfigType& config, Playlist& playlist,
         ChannelsMapper& channels_mapper)
      : m_config(config),
        m_playlist(playlist),
        m_channels_mapper(channels_mapper) {}
  Filter(Filter&) = delete;
  Filter(Filter&&) = delete;
  Filter& operator=(Filter&) = delete;
  Filter& operator=(Filter&&) = delete;
  void filter(std::vector<std::string>&& epgs,
              std::string_view new_epg_filename);
  cppcoro::task<> filter_iptv_channels(cppcoro::static_thread_pool& tp,
                                       PlaylistParserFilterBuffer& buffer);

 private:
  void copy_xml_nodes(const std::string& epg, std::string_view node_name,
                      std::ofstream& new_epg_stream);

 private:
  ConfigType& m_config;
  Playlist& m_playlist;
  ChannelsMapper& m_channels_mapper;
};

}  // namespace pefti
