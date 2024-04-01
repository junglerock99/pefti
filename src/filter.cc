#include "filter.h"

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <algorithm>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <gsl/gsl>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

#include "buffers.h"
#include "config.h"
#include "iptv_channel.h"
#include "mapper.h"
#include "playlist.h"
#include "sax_fsm.h"

using namespace std::literals;

namespace pefti {

// Copies XML nodes from source to destination if the channel in the source
// matches a channel in the playlist
void Filter::copy_xml_nodes(const std::string& source,
                            std::string_view node_name,
                            std::ofstream& destination) {
  xmlSAXHandler sax_handler;
  memset(&sax_handler, 0, sizeof(sax_handler));
  sax_handler.initialized = XML_SAX2_MAGIC;
  sax_handler.startElementNs = &SaxFsm::handler_start_element;
  sax_handler.endElementNs = &SaxFsm::handler_end_element;
  sax_handler.characters = &SaxFsm::handler_characters;
  SaxFsm fsm(std::string{node_name}, destination, m_playlist);
  int result = xmlSAXUserParseMemory(&sax_handler, &fsm, source.c_str(),
                                     int(source.size()));
  if (result != 0) throw std::runtime_error("Failed to parse XML document");
  xmlCleanupParser();
}

// Filters EPGs and creates a new EPG file.
// Copies <channel> and <programme> nodes from all EPGs to the new EPG.
void Filter::filter(std::vector<std::string>&& epgs,
                    std::string_view new_epg_filename) {
  if (epgs.empty()) return;
  Expects(!new_epg_filename.empty());
  LIBXML_TEST_VERSION;  // Check for library ABI mismatch
  std::ofstream new_epg_stream{std::string{new_epg_filename}};
  new_epg_stream << R"(<?xml version="1.0" encoding="utf-8"?>)" << '\n';
  new_epg_stream << R"(<!DOCTYPE tv SYSTEM "xmltv.dtd">)" << '\n';
  new_epg_stream << R"(<tv generator-info-name="pefti">)";
  for (const auto& epg : epgs) {
    copy_xml_nodes(epg, SaxFsm::KChannel, new_epg_stream);
    copy_xml_nodes(epg, SaxFsm::kProgramme, new_epg_stream);
  }
  new_epg_stream << "\n</tv>" << '\n';
  new_epg_stream.close();
}

cppcoro::task<> Filter::filter_iptv_channels(
    cppcoro::static_thread_pool& tp, PlaylistParserFilterBuffer& buffer) {
  //
  // Lambdas
  //
  const auto is_blocked_group = [this](auto&& channel) {
    const auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    return ((group_title != std::nullopt) &&
            (m_config.is_blocked_group(*group_title)));
  };
  //
  const auto is_blocked_channel = [this](auto&& channel) {
    return m_config.is_blocked_channel(channel.get_original_name());
  };
  //
  const auto is_blocked_url = [this](auto&& channel) {
    return m_config.is_blocked_url(channel.get_url());
  };
  //
  // A special case is that if no allowed groups or allowed channels are
  // specified in the configuration then all channels are allowed.
  const auto are_all_channels_allowed = [this]() {
    return (m_config.get_num_channels_templates() +
            m_config.get_num_allowed_groups()) == 0;
  };
  //
  const auto is_allowed_group = [this](auto&& channel) {
    const auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    return ((group_title != std::nullopt) &&
            (m_config.is_allowed_group(*group_title)));
  };
  //
  const auto is_allowed_channel = [this](auto&& channel) {
    return m_channels_mapper.is_allowed_channel(channel);
  };
  //
  // End of lambdas
  //
  // Schedule this coroutine onto the threadpool
  co_await tp.schedule();
  //
  // Buffer size must be a power of 2
  constexpr auto size = std::tuple_size<decltype(buffer.data)>::value;
  static_assert((size > 0) && ((size & (size - 1)) == 0));
  const size_t index_mask = buffer.data.size() - 1;
  size_t read_index{0};
  while (true) {
    //
    // Wait for next channel to be added to the buffer
    const size_t write_index =
        co_await buffer.sequencer.wait_until_published(read_index, tp);
    do {
      auto& channel = buffer.data[read_index & index_mask];
      const bool is_sentinel = channel.get_original_name() == kSentinel;
      if (!is_sentinel) {
        //
        // Apply filters
        if (!is_blocked_group(channel) && !is_blocked_channel(channel) &&
            !is_blocked_url(channel) &&
            (are_all_channels_allowed() || is_allowed_group(channel) ||
             is_allowed_channel(channel))) {
          m_playlist.push_back(std::move(channel));
        }
      } else {
        co_return;
      }
    } while (read_index++ != write_index);
    buffer.barrier.publish(read_index);
  }
}

}  // namespace pefti
