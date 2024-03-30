#pragma once

#include <array>
#include <cppcoro/single_producer_sequencer.hpp>

#include "iptv_channel.h"

namespace pefti {

template <typename T, std::size_t size>
class CoroutineSingleProducerBuffer {
 public:
  CoroutineSingleProducerBuffer() : sequencer(barrier, size) {}
  std::array<T, size> data;
  cppcoro::sequence_barrier<std::size_t> barrier;
  cppcoro::single_producer_sequencer<size_t> sequencer;
};

using PlaylistParserFilterBuffer =
    CoroutineSingleProducerBuffer<IptvChannel, 64>;

static constexpr auto kSentinel = "sentinel"sv;

}  // namespace pefti
