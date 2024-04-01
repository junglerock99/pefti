#pragma once

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <string>
#include <vector>

#include "buffers.h"

namespace pefti {

class Parser {
 public:
  Parser() {}
  Parser(Parser&) = delete;
  Parser(Parser&&) = delete;
  Parser& operator=(Parser&) = delete;
  Parser& operator=(Parser&&) = delete;
  cppcoro::task<> parse_iptv_channels(cppcoro::static_thread_pool& tp,
                                      PlaylistParserFilterBuffer& buffer,
                                      std::vector<std::string>& playlists);
};

}  // namespace pefti
