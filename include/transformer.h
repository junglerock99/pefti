#pragma once

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <vector>

#include "buffers.h"
#include "config.h"
#include "iptv_channel.h"
#include "mapper.h"
#include "playlist.h"

namespace pefti {

// Transforms playlists.
class Transformer {
 public:
  explicit Transformer(ConfigType& config, Playlist& playlist,
                       ChannelsMapper& channels_mapper)
      : config_(config),
        playlist_(playlist),
        channels_mapper_(channels_mapper) {}
  ~Transformer() = default;
  Transformer(Transformer&) = delete;
  Transformer(Transformer&&) = delete;
  Transformer& operator=(Transformer&) = delete;
  Transformer& operator=(Transformer&&) = delete;
  void transform();
  cppcoro::task<> transform(cppcoro::static_thread_pool& tp,
                            PlaylistFilterTransformerBuffer& buffer);

 private:
  void block_tags(IptvChannel& channel);
  void copy_tags(IptvChannel& channel);
  void copy_group_title_tag();
  void order_by_sort_criteria();
  void set_name(IptvChannel& channel);

 private:
  ConfigType& config_;
  Playlist& playlist_;
  ChannelsMapper& channels_mapper_;
};

}  // namespace pefti
