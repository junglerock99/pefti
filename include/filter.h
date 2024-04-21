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

// Filters playlists and EPGs.
class Filter {
 public:
  Filter(ConfigType& config, Playlist& playlist,
         ChannelsMapper& channels_mapper)
      : config_(config),
        playlist_(playlist),
        channels_mapper_(channels_mapper) {}
  Filter(Filter&) = delete;
  Filter(Filter&&) = delete;
  Filter& operator=(Filter&) = delete;
  Filter& operator=(Filter&&) = delete;
  void filter(std::vector<std::string>&& epgs,
              std::string_view new_epg_filename);
  cppcoro::task<> filter(cppcoro::static_thread_pool& tp,
                         PlaylistParserFilterBuffer& pf_buffer,
                         PlaylistFilterTransformerBuffer& ft_buffer);

 private:
  void copy_xml_nodes(const std::string& epg, std::string_view node_name,
                      std::ofstream& new_epg_stream);
  cppcoro::task<> publish_sentinel(cppcoro::static_thread_pool& tp,
                                   PlaylistFilterTransformerBuffer& ft_buffer);

 private:
  ConfigType& config_;
  Playlist& playlist_;
  ChannelsMapper& channels_mapper_;
};

}  // namespace pefti
