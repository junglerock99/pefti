#include "parser.h"

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "buffers.h"
#include "iptv_channel.h"

namespace pefti {

cppcoro::task<> Parser::parse_iptv_channels(
    cppcoro::static_thread_pool& tp, PlaylistParserFilterBuffer& buffer,
    std::vector<std::string>& playlists) {
  std::vector<std::istringstream> streams(playlists.size());
  std::ranges::for_each(playlists, [&streams](auto&& playlist) {
    streams.emplace_back(std::istringstream{playlist});
  });
  //
  // Schedule this coroutine onto the threadpool
  co_await tp.schedule();
  //
  // Buffer size must be a power of 2
  constexpr auto size = std::tuple_size<decltype(buffer.data)>::value;
  static_assert((size > 0) && ((size & (size - 1)) == 0));
  const size_t index_mask = buffer.data.size() - 1;
  for (auto& stream : streams) {
    while (stream) {
      //
      // Wait for free slot in the buffer
      size_t write_index = co_await buffer.sequencer.claim_one(tp);
      //
      // Parse next channel and add to buffer
      IptvChannel new_channel;
      stream >> new_channel;
      auto& channel_in_buffer = buffer.data[write_index & index_mask];
      channel_in_buffer = std::move(new_channel);
      buffer.sequencer.publish(write_index);
    }
  }
  //
  // Publish sentinel
  size_t write_index = co_await buffer.sequencer.claim_one(tp);
  IptvChannel sentinel;
  sentinel.set_original_name(kSentinel);
  auto& channel_in_buffer = buffer.data[write_index & index_mask];
  channel_in_buffer = std::move(sentinel);
  buffer.sequencer.publish(write_index);
}

}  // namespace pefti
