#pragma once

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <string>

#include "buffers.h"

namespace pefti {

// Loads playlists.
class Loader {
 public:
  Loader() {}
  Loader(Loader&) = delete;
  Loader(Loader&&) = delete;
  Loader& operator=(Loader&) = delete;
  Loader& operator=(Loader&&) = delete;
  cppcoro::task<> load(cppcoro::static_thread_pool& tp,
                       PlaylistLoaderParserBuffer& buffer,
                       const std::string& url);

 private:
  static constexpr std::u8string kPlaylistSentinel = u8"\x89\x89";
  static constexpr auto kPlaylistSentinelSize = kPlaylistSentinel.length();

 private:
  void write_playlist_sentinel(PlaylistLoaderParserBuffer& buffer);
};

}  // namespace pefti
