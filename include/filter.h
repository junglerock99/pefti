#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"
#include "epg.h"
#include "playlist.h"

namespace pefti {

// Application logic for filtering playlists and EPGs
class Filter {

 public:
  Filter(std::shared_ptr<Config> config) : m_config(config) {}
  Filter(Filter&) = delete;
  Filter(Filter&&) = delete;
  Filter& operator=(Filter&) = delete;
  Filter& operator=(Filter&&) = delete;
  void filter(std::vector<std::string>&& playlists, 
              Playlist& new_playlist);
  void filter(std::vector<std::string>&& epgs, 
              std::string_view new_epg_filename, 
              Playlist& playlist);

 private:
  std::shared_ptr<Config> m_config;
};

}  // namespace pefti
