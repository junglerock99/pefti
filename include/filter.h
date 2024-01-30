#ifndef PEFTI_FILTER_H_
#define PEFTI_FILTER_H_

#include <memory>
#include <vector>

#include "config.h"
#include "playlist.h"

namespace pefti {

// Contains the application logic for filtering playlists.
class Filter {
 public:
  Filter(std::shared_ptr<Config> config) : m_config(config) {}
  Filter(Filter&) = delete;
  Filter(Filter&&) = delete;
  Filter& operator=(Filter&) = delete;
  Filter& operator=(Filter&&) = delete;
  void filter(std::vector<std::string>&& in_playlists, Playlist& out_playlists);

 private:
  std::shared_ptr<Config> m_config;
};

}  // namespace pefti

#endif  // PEFTI_FILTER_H_
