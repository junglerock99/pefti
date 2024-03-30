#include "parser.h"

#include <algorithm>
#include <cppcoro/single_producer_sequencer.hpp>
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
  co_await tp.schedule();
  //
  // Buffer size must be a power of 2
  constexpr auto size = std::tuple_size<decltype(buffer.data)>::value;
  static_assert((size > 0) && ((size & (size - 1)) == 0));
  const size_t index_mask = buffer.data.size() - 1;
  for (auto& stream : streams) {
    IptvChannel new_channel;
    while (stream) {
      size_t write_index = co_await buffer.sequencer.claim_one(tp);
      stream >> new_channel;
      auto& channel = buffer.data[write_index & index_mask];
      channel = std::move(new_channel);
      buffer.sequencer.publish(write_index);
    }
  }
  //
  // Publish sentinel
  size_t write_index = co_await buffer.sequencer.claim_one(tp);
  auto& channel = buffer.data[write_index & index_mask];
  IptvChannel sentinel;
  sentinel.set_original_name(kSentinel);
  channel = sentinel;
  buffer.sequencer.publish(write_index);
}

}  // namespace pefti
