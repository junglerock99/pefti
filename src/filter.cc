#include "filter.h"

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/task.hpp>
#include <fstream>
#include <gsl/gsl>
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
  SaxFsm fsm(std::string{node_name}, destination, playlist_);
  int result = xmlSAXUserParseMemory(&sax_handler, &fsm, source.c_str(),
                                     int(source.size()));
  if (result != 0) throw std::runtime_error("Failed to parse XML document");
  xmlCleanupParser();
}

// Filters EPGs and creates a new EPG file.
// Copies <channel> and <programme> nodes from all input EPGs to the new EPG.
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

// Filters IPTV channels.
cppcoro::task<> Filter::filter(cppcoro::static_thread_pool& tp,
                               PlaylistParserFilterBuffer& pf_buffer,
                               PlaylistFilterTransformerBuffer& ft_buffer) {
  //
  // Lambdas
  //
  const auto is_blocked_group = [this](auto&& channel) {
    const auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    return ((group_title != std::nullopt) &&
            (config_.is_blocked_group(*group_title)));
  };
  //
  const auto is_blocked_channel = [this](auto&& channel) {
    return config_.is_blocked_channel(channel.get_original_name());
  };
  //
  const auto is_blocked_url = [this](auto&& channel) {
    return config_.is_blocked_url(channel.get_url());
  };
  //
  // A special case is that if no allowed groups or allowed channels are
  // specified in the configuration then all channels are allowed.
  const auto are_all_channels_allowed = [this]() {
    return (config_.get_num_channels_templates() +
            config_.get_num_allowed_groups()) == 0;
  };
  //
  const auto is_allowed_group = [this](auto&& channel) {
    const auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    return ((group_title != std::nullopt) &&
            (config_.is_allowed_group(*group_title)));
  };
  //
  const auto is_allowed_channel = [this](auto&& channel) {
    return channels_mapper_.is_allowed_channel(channel);
  };
  //
  // End of lambdas
  //
  co_await tp.schedule();
  const auto kPfIndexMask = pf_buffer.get_index_mask();
  const auto kFtIndexMask = ft_buffer.get_index_mask();
  size_t pf_read_index{0};
  bool received_sentinel{false};
  while (!received_sentinel) {
    const size_t pf_write_index =
        co_await pf_buffer.sequencer.wait_until_published(pf_read_index, tp);
    do {
      auto& channel = pf_buffer.data[pf_read_index & kPfIndexMask];
      const bool is_sentinel = channel.get_original_name() == kSentinel;
      if (!is_sentinel) {
        if (!is_blocked_group(channel) && !is_blocked_channel(channel) &&
            !is_blocked_url(channel) &&
            (are_all_channels_allowed() || is_allowed_group(channel) ||
             is_allowed_channel(channel))) {
          size_t ft_write_index = co_await ft_buffer.sequencer.claim_one(tp);
          ft_buffer.data[ft_write_index & kFtIndexMask] = std::move(channel);
          ft_buffer.sequencer.publish(ft_write_index);
        }
      } else {
        received_sentinel = true;
        break;
      }
    } while (pf_read_index++ != pf_write_index);
    pf_buffer.barrier.publish(pf_read_index);
  }
  co_await publish_sentinel(tp, ft_buffer);
}

// Publishes a sentinel to the ring buffer to indicate end of data.
cppcoro::task<> Filter::publish_sentinel(
    cppcoro::static_thread_pool& tp,
    PlaylistFilterTransformerBuffer& ft_buffer) {
  co_await tp.schedule();
  const auto kIndexMask = ft_buffer.get_index_mask();
  size_t write_index = co_await ft_buffer.sequencer.claim_one(tp);
  IptvChannel sentinel;
  sentinel.set_original_name(kSentinel);
  ft_buffer.data[write_index & kIndexMask] = std::move(sentinel);
  ft_buffer.sequencer.publish(write_index);
}

}  // namespace pefti
