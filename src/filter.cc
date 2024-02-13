#include "filter.h"

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"
#include "iptv_channel.h"
#include "playlist.h"
#include "sax_fsm.h"

using namespace std::literals;

namespace pefti {

// Filters playlists and creates a new playlist
Playlist& Filter::filter(std::vector<std::string>&& playlists,
                         Playlist& new_playlist) {
  std::vector<std::istringstream> streams(playlists.size());
  std::ranges::for_each(playlists, [&streams](auto&& playlist) {
    streams.emplace_back(std::istringstream{playlist});
  });
  //
  // Filters out a channel if it is a member of a blocked group
  auto block_group = [this](auto&& channel) {
    auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    if (group_title != std::nullopt) {
      return (!m_config->is_blocked_group(*group_title));
    } else {
      return true;
    }
  };
  //
  // Filters out a channel if its name contains any of the text strings
  // that identify blocked channels
  auto block_channel = [this](auto&& channel) {
    return (!m_config->is_blocked_channel(channel.get_original_name()));
  };
  //
  // Filters out a channel if it has a blocked URL
  auto block_url = [this](auto&& channel) {
    return (!m_config->is_blocked_url(channel.get_url()));
  };
  //
  // Filters out a channel if it is not an allowed channel or in an allowed
  // group. A special case is that if no allowed groups or allowed channels are
  // specified in the configuration then all channels are unfiltered.
  auto allow_groups_and_channels = [this](auto&& channel) {
    const bool are_all_channels_allowed =
        (m_config->get_num_allowed_channels() +
         m_config->get_num_allowed_groups()) == 0;
    const auto group_title = channel.get_tag_value(IptvChannel::kTagGroupTitle);
    bool is_allowed{false};
    if (are_all_channels_allowed) {
      is_allowed = true;
    } else if ((group_title != std::nullopt) &&
               (m_config->is_allowed_group(*group_title))) {
      is_allowed = true;
    } else if (m_config->is_allowed_channel(channel.get_original_name())) {
      is_allowed = true;
    } else
      is_allowed = false;
    return is_allowed;
  };
  //
  // Removes tags from a channel if the tags are blocked in the configuration
  auto block_tags = [this](auto&& channel) {
    for (auto tag : m_config->get_blocked_tags()) channel.delete_tag(tag);
    return channel;
  };
  //
  // Filter all channels from the input playlists to create the new playlist
  for (auto& stream : streams) {
    auto view = std::views::istream<IptvChannel>(stream) |
                std::views::filter(block_group) |
                std::views::filter(block_channel) |
                std::views::filter(block_url) |
                std::views::filter(allow_groups_and_channels) |
                std::views::transform(block_tags);
    for (auto channel : view) {
      new_playlist.push_back(std::move(channel));
    }
  }
  return new_playlist;
}

// Filters EPGs and creates a new EPG file.
// Copies <channel> nodes from all EPGs to the new EPG,
// then copies <programme> nodes.
void Filter::filter(std::vector<std::string>&& epgs,
                    std::string_view new_epg_filename, Playlist& playlist) {
  if (epgs.empty()) return;
  LIBXML_TEST_VERSION  // Check for library ABI mismatch
      std::ofstream new_epg_stream{std::string{new_epg_filename}};
  new_epg_stream << R"(<?xml version="1.0" encoding="utf-8"?>)" << '\n';
  new_epg_stream << R"(<!DOCTYPE tv SYSTEM "xmltv.dtd">)" << '\n';
  new_epg_stream << R"(<tv generator-info-name="pefti">)";
  for (const auto& epg : epgs) {
    copy_xml_nodes(epg, SaxFsm::KChannel, playlist, new_epg_stream);
    copy_xml_nodes(epg, SaxFsm::kProgramme, playlist, new_epg_stream);
  }
  new_epg_stream << "\n</tv>" << '\n';
  new_epg_stream.close();
}

// Copies XML nodes from source to destination if the channel in the source
// matches a channel in the playlist
void Filter::copy_xml_nodes(const std::string& source,
                            std::string_view node_name, Playlist& playlist,
                            std::ofstream& destination) {
  xmlSAXHandler sax_handler;
  memset(&sax_handler, 0, sizeof(sax_handler));
  sax_handler.initialized = XML_SAX2_MAGIC;
  sax_handler.startElementNs = &SaxFsm::handler_start_element;
  sax_handler.endElementNs = &SaxFsm::handler_end_element;
  sax_handler.characters = &SaxFsm::handler_characters;
  SaxFsm fsm(std::string{node_name}, destination, playlist);
  int result = xmlSAXUserParseMemory(&sax_handler, &fsm, source.c_str(),
                                     int(source.size()));
  if (result != 0) throw std::runtime_error("Failed to parse XML document");
  xmlCleanupParser();
}

}  // namespace pefti
