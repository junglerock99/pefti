#include "parser.h"

#include <array>
#include <cppcoro/generator.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <cstring>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "buffers.h"
#include "iptv_channel.h"

using namespace std::literals;

namespace pefti {

std::array<char8_t, 64 * 1024> line_buffer;
const char8_t LF = '\n';

Parser::Parser() {
  states_[static_cast<size_t>(State::kWaitingForExtinf)] =
      new WaitingForExtinfState(*this);
  states_[static_cast<size_t>(State::kWaitingForUrl)] =
      new WaitingForUrlState(*this);
  active_state_ = states_[static_cast<size_t>(State::kWaitingForExtinf)];
}

Parser::~Parser() {
  for (auto state : states_) delete state;
}

// Returns the most recently received complete line. If the line wraps around
// to the start of the buffer then we copy both parts of the line to a
// separate buffer.
std::string_view Parser::get_line() {
  std::string_view line;
  const auto kBufferSize = lp_buffer_->get_size();
  const auto kIndexMask = lp_buffer_->get_index_mask();
  const auto start_block = start_index_ / kBufferSize;
  const auto end_block = read_index_ / kBufferSize;
  const auto line_length = read_index_ - start_index_ + 1;
  if (start_block == end_block) [[likely]] {
    auto index = start_index_ & kIndexMask;
    auto source = reinterpret_cast<const char*>(&lp_buffer_->data[index]);
    line = std::string_view{source, line_length - 1};
  } else {
    const auto size_part_1 = kBufferSize - (start_index_ & kIndexMask);
    const auto size_part_2 = read_index_ - start_index_ - size_part_1 + 1;
    std::strncpy(reinterpret_cast<char*>(&line_buffer[0]),
                 reinterpret_cast<const char*>(
                     &lp_buffer_->data[start_index_ & kIndexMask]),
                 size_part_1);
    std::strncpy(reinterpret_cast<char*>(&line_buffer[size_part_1]),
                 reinterpret_cast<const char*>(lp_buffer_->data.data()),
                 size_part_2);
    line = std::string_view{reinterpret_cast<const char*>(&line_buffer[0]),
                            line_length - 1};
  }
  return line;
}

inline bool Parser::have_received_line_feed() {
  bool have_received_line_feed{false};
  const size_t kIndexMask = lp_buffer_->get_index_mask();
  if (lp_buffer_->data[read_index_ & kIndexMask] == LF)
    have_received_line_feed = true;
  return have_received_line_feed;
}

inline bool Parser::have_received_sentinel() {
  bool have_received_sentinel{false};
  const size_t kIndexMask = lp_buffer_->get_index_mask();
  if (lp_buffer_->data[read_index_ & kIndexMask] == u8'\x89')
    if (read_index_ > 0)
      if (lp_buffer_->data[(read_index_ - 1) & kIndexMask] == u8'\x89')
        have_received_sentinel = true;
  return have_received_sentinel;
}

// Parses the stream of characters received in `lp_buffer` into IptvChannel
// objects and writes them to `pf_buffer`.
cppcoro::task<> Parser::parse(cppcoro::static_thread_pool& tp,
                              PlaylistLoaderParserBuffer& lp_buffer,
                              PlaylistParserFilterBuffer& pf_buffer) {
  co_await tp.schedule();
  lp_buffer_ = &lp_buffer;
  pf_buffer_ = &pf_buffer;
  bool received_sentinel{false};
  IptvChannel iptv_channel;
  while (!received_sentinel) {
    const size_t write_index =
        co_await lp_buffer_->sequencer.wait_until_published(read_index_, tp);
    do {
      if (have_received_line_feed()) {
        auto line = get_line();
        co_await active_state_->process_line(tp, line, iptv_channel);
        lp_buffer_->barrier.publish(read_index_);
        start_index_ = read_index_ + 1;
      } else if (have_received_sentinel()) {
        received_sentinel = true;
        break;
      }

    } while (read_index_++ != write_index);
  }
  co_await publish_sentinel(tp);
}

// Writes a sentinel to `pf_buffer`.
cppcoro::task<> Parser::publish_sentinel(cppcoro::static_thread_pool& tp) {
  co_await tp.schedule();
  const size_t kIndexMask = pf_buffer_->get_index_mask();
  size_t write_index = co_await pf_buffer_->sequencer.claim_one(tp);
  IptvChannel sentinel;
  sentinel.set_original_name(kSentinel);
  pf_buffer_->data[write_index & kIndexMask] = std::move(sentinel);
  pf_buffer_->sequencer.publish(write_index);
}

// StateBase

Parser::StateBase::StateBase(Parser& parser) : parser_(&parser) {}

//
// WaitingForExtinfState
//

// Extracts key/value pairs from `line_view` and copies them to `channel`.
void Parser::WaitingForExtinfState::get_key_value_pairs(
    IptvChannel& channel, std::string_view line_view) {
  const std::string line{line_view};
  std::regex kv_pattern{R"(([\w\-]*=\"[\w\-: \/\.]*\"))"s};
  std::sregex_iterator kv_iterator{line.cbegin() + 7, line.cend(), kv_pattern};
  std::sregex_iterator kv_end;
  while (kv_iterator != kv_end) {
    std::string kv_pair = (*kv_iterator)[0];
    auto pos = kv_pair.find("=");
    const bool have_key_value_pair = ((pos > 0) && (pos != std::string::npos));
    if (have_key_value_pair) {
      auto key = kv_pair.substr(0, pos);
      auto length = kv_pair.length();
      std::string value;
      const bool is_empty_value =
          (kv_pair[pos + 1] == '"' && kv_pair[pos + 2] == '"');
      if (is_empty_value)
        value = "";
      else {
        value = kv_pair.substr(
            kv_pair[pos + 1] == '"' ? pos + 2 : pos + 1,
            kv_pair[length - 1] == '"' ? length - pos - 3 : length - pos - 2);
      }
      channel.set_tag(key, value);
    }
    ++kv_iterator;
  }
}

// Extracts the channel data in `line` and writes it to `iptv_channel`.
// Gets the channel name by finding the first comma after the final
// key=value pair.
void Parser::WaitingForExtinfState::handle_extinf(std::string_view line,
                                                  IptvChannel& iptv_channel) {
  auto pos = line.rfind('=');
  pos = line.find(',', (pos != std::string::npos) ? pos : 0);
  if (pos != std::string::npos) {
    iptv_channel.set_original_name(line.substr(pos + 1));
    line = std::string_view{line.cbegin(), pos};  // Removes channel name
    get_key_value_pairs(iptv_channel, std::string_view{&line[0], pos});
  } else {
    iptv_channel.set_original_name(""sv);
  }
}

// If `line` contains a EXTINF directive, the IPTV channel data is extracted
// from the line and stored in an IptvChannel object. Then we change state
// to wait for the channel's URL.
cppcoro::task<> Parser::WaitingForExtinfState::process_line(
    cppcoro::static_thread_pool& tp, std::string_view line,
    IptvChannel& iptv_channel) {
  co_await tp.schedule();
  const auto& p{parser_};
  if (line.starts_with(kExtinf_)) {
    handle_extinf(line, iptv_channel);
    p->active_state_ = p->states_[static_cast<size_t>(State::kWaitingForUrl)];
  }
}

//
// WaitingForUrlState
//

// If `line` contains a URL, then the IPTV channel is complete and
// is moved to the ring buffer.
cppcoro::task<> Parser::WaitingForUrlState::process_line(
    cppcoro::static_thread_pool& tp, std::string_view line,
    IptvChannel& iptv_channel) {
  co_await tp.schedule();
  const auto& p{parser_};
  if (line.starts_with(kHttp)) {
    iptv_channel.set_url(line);
    const size_t kIndexMask = p->pf_buffer_->get_index_mask();
    size_t write_index = co_await p->pf_buffer_->sequencer.claim_one(tp);
    p->pf_buffer_->data[write_index & kIndexMask] = std::move(iptv_channel);
    p->pf_buffer_->sequencer.publish(write_index);
    p->active_state_ =
        p->states_[static_cast<size_t>(State::kWaitingForExtinf)];
  }
}

}  // namespace pefti
