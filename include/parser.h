#pragma once

#include <cppcoro/generator.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "buffers.h"
#include "iptv_channel.h"

using namespace std::literals;

namespace pefti {

class Parser {
 private:
  enum class State { kWaitingForExtinf, kWaitingForUrl, kNumStates };

  // Base class of Parser state machine classes.
  class StateBase {
   public:
    StateBase(Parser& parser);
    virtual ~StateBase() {}
    virtual cppcoro::task<> process_line(cppcoro::static_thread_pool& tp,
                                         std::string_view line,
                                         IptvChannel& iptv_channel) = 0;

   protected:
    Parser* parser_;
  };

  // Parser state machine class.
  class WaitingForExtinfState : public StateBase {
   public:
    WaitingForExtinfState(Parser& parser) : StateBase(parser) {}
    cppcoro::task<> process_line(cppcoro::static_thread_pool& tp,
                                 std::string_view line,
                                 IptvChannel& iptv_channel) override;

   private:
    void get_key_value_pairs(IptvChannel& channel, std::string_view line);
    void handle_extinf(std::string_view line, IptvChannel& iptv_channel);
  };

  // Parser state machine class.
  class WaitingForUrlState : public StateBase {
   public:
    WaitingForUrlState(Parser& parser) : StateBase(parser) {}
    cppcoro::task<> process_line(cppcoro::static_thread_pool& tp,
                                 std::string_view line,
                                 IptvChannel& iptv_channel) override;
  };

 public:
  enum class ParserState { kWaitingForExtinf, kWaitingForUrl };
  Parser();
  ~Parser();
  Parser(Parser&) = delete;
  Parser(Parser&&) = delete;
  Parser& operator=(Parser&) = delete;
  Parser& operator=(Parser&&) = delete;
  cppcoro::task<> parse(cppcoro::static_thread_pool& tp,
                        PlaylistLoaderParserBuffer& lp_buffer,
                        PlaylistParserFilterBuffer& pf_buffer);

 private:
  static constexpr std::string_view kExtinf_ = "#EXTINF"sv;
  static constexpr std::string_view kHttp = "http"sv;

  PlaylistLoaderParserBuffer* lp_buffer_;
  PlaylistParserFilterBuffer* pf_buffer_;
  size_t start_index_{0};
  size_t read_index_{0};
  std::array<StateBase*, static_cast<unsigned long>(State::kNumStates)> states_;
  StateBase* active_state_;

  std::string_view get_line();
  bool have_received_line_feed();
  bool have_received_sentinel();
  void parse_channnel(PlaylistLoaderParserBuffer& lp_buffer,
                      size_t& start_index, size_t& end_index,
                      const size_t kIndexMask);
  cppcoro::task<> publish_sentinel(cppcoro::static_thread_pool& tp);
};

}  // namespace pefti
