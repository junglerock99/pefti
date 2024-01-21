#include "application.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <string_view>

#include "config.h"
#include "filter.h"
#include "playlist.h"
#include "sorter.h"
#include "transformer.h"

namespace pefti {

void Application::run() {
  Playlist new_playlist;
  const auto& filenames = m_config->get_input_playlists_filenames();
  auto input_playlists = load_playlists(filenames);
  m_filter->filter(std::move(input_playlists), new_playlist);
  m_transformer->transform(new_playlist);
  m_sorter->sort(new_playlist);
  store_playlist(m_config->get_new_playlist_filename(), new_playlist);
}

}  // namespace pefti
