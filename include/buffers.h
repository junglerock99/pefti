#pragma once

#include <array>
#include <cppcoro/single_producer_sequencer.hpp>
#include <cppcoro/static_thread_pool.hpp>

#include "iptv_channel.h"

using namespace std::literals;

namespace pefti {

template <typename T, std::size_t size>
class CoroutineSingleProducerBuffer {
 public:
  CoroutineSingleProducerBuffer() : sequencer(barrier, size) {}
  std::array<T, size> data;
  cppcoro::sequence_barrier<std::size_t> barrier;
  cppcoro::single_producer_sequencer<size_t> sequencer;
  cppcoro::static_thread_pool* thread_pool;
  constexpr auto get_index_mask() { return size - 1; };
  constexpr auto get_size() { return size; };

 private:
};

using PlaylistLoaderParserBuffer =
    CoroutineSingleProducerBuffer<char8_t, 64 * 1024>;
using PlaylistParserFilterBuffer =
    CoroutineSingleProducerBuffer<IptvChannel, 64>;
using PlaylistFilterTransformerBuffer =
    CoroutineSingleProducerBuffer<IptvChannel, 64>;

static constexpr auto kSentinel = "SENTINEL"sv;

}  // namespace pefti
