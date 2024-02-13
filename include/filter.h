#pragma once

#include <fstream>
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
  Playlist& filter(std::vector<std::string>&& playlists,
                   Playlist& new_playlist);
  void filter(std::vector<std::string>&& epgs,
              std::string_view new_epg_filename, Playlist& playlist);

 private:
  void copy_xml_nodes(const std::string& epg, std::string_view node_name,
                      Playlist& playlist, std::ofstream& new_epg_stream);

 private:
  std::shared_ptr<Config> m_config;
};

}  // namespace pefti
