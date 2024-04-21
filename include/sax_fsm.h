#pragma once

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <fstream>
#include <string>

#include "playlist.h"

using namespace std::string_view_literals;

namespace pefti {

// Finite state machine for XML SAX handlers
class SaxFsm {
 public:
  static constexpr auto KChannel = "channel"sv;
  static constexpr auto kId = "id"sv;
  static constexpr auto kProgramme = "programme"sv;
  enum class State { kWaitingForParentNode, kInsideNode, kOutsideNode };
  enum class ParentNode { kChannel, kProgramme };

 public:
  SaxFsm(std::string_view parent_node, std::ofstream& stream,
         Playlist& playlist);
  // SAX handlers
  static void handler_start_element(void* context, const xmlChar* localname,
                                    const xmlChar*, const xmlChar*, int,
                                    const xmlChar**, int nb_attributes, int,
                                    const xmlChar** attributes);
  static void handler_end_element(void* context, const xmlChar* localname,
                                  const xmlChar*, const xmlChar*);
  static void handler_characters(void* context, const xmlChar* begin,
                                 int length);

  State state_{State::kWaitingForParentNode};

 private:
  std::string get_attribute_value(std::string_view attribute_name,
                                  int num_attributes,
                                  const xmlChar** attributes);

 private:
  const std::string_view parent_node_;
  std::ofstream& stream_;
  Playlist& playlist_;
  int indentation_{1};
  std::string characters_;
  std::string current_node_name_;
};

}  // namespace pefti
