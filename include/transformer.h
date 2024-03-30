#pragma once

#include <vector>

#include "buffers.h"
#include "config.h"
#include "mapper.h"
#include "playlist.h"

namespace pefti {

// Contains the application logic for transforming a playlist
class Transformer {
 public:
  explicit Transformer(ConfigType& config, Playlist& playlist,
                       ChannelsMapper& channels_mapper)
      : m_config(config),
        m_playlist(playlist),
        m_channels_mapper(channels_mapper) {}
  ~Transformer() = default;
  Transformer(Transformer&) = delete;
  Transformer(Transformer&&) = delete;
  Transformer& operator=(Transformer&) = delete;
  Transformer& operator=(Transformer&&) = delete;
  void transform();

 private:
  void block_tags();
  void copy_tags();
  void copy_group_title_tag();
  void order_by_sort_criteria();
  void set_name();

 private:
  ConfigType& m_config;
  Playlist& m_playlist;
  ChannelsMapper& m_channels_mapper;
};

}  // namespace pefti
