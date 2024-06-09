#include "sax_fsm.h"

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "playlist.h"

namespace pefti {

SaxFsm::SaxFsm(std::string_view parent_node, std::ofstream& stream,
               Playlist& playlist)
    : parent_node_(parent_node), stream_(stream), playlist_(playlist) {}

std::string SaxFsm::get_attribute_value(std::string_view attribute_name,
                                        int num_attributes,
                                        const xmlChar** attributes) {
  std::string value{};
  unsigned int index = 0;
  for (int i = 0; i < num_attributes; ++i, index += 5) {
    std::string name = reinterpret_cast<const char*>(attributes[index]);
    if (name == attribute_name) {
      const xmlChar* begin = attributes[index + 3];
      const xmlChar* end = attributes[index + 4];
      value = std::string{reinterpret_cast<const char*>(begin),
                          reinterpret_cast<const char*>(end)};
      break;
    }
  }
  return value;
}

// SAX2 handler for characters between start and end elements
void SaxFsm::handler_characters(void* context, const xmlChar* begin,
                                int length) {
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  if (fsm.state_ == State::kInsideNode) {
    auto end = begin + length;
    std::for_each(begin, end, [&fsm](unsigned char c) {
      if (!std::isspace(c) || c == ' ') fsm.characters_.push_back(c);
    });
  }
}

// SAX2 handler when an element end has been detected by the parser.
// It provides the namespace information for the element.
// context: The user data (XML parser context)
// localname: The local name of the element
// prefix: The element namespace prefix if available
// URI: The element namespace name if available
void SaxFsm::handler_end_element(void* context, const xmlChar* local_name,
                                 const xmlChar*, const xmlChar*) {
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  std::string element_name{reinterpret_cast<const char*>(local_name)};
  switch (fsm.state_) {
    case State::kWaitingForParentNode:
      return;
    case State::kInsideNode: {
      if (element_name != fsm.current_node_name_)
        throw std::runtime_error("Invalid XML");
      if (!std::all_of(fsm.characters_.begin(), fsm.characters_.end(),
                       isspace)) {
        fsm.stream_ << fsm.characters_;
      }
    } break;
    case State::kOutsideNode:
      fsm.stream_ << '\n';
      fsm.indentation_--;
      for (int i{}; i < fsm.indentation_; i++) fsm.stream_ << '\t';
      break;
  }
  fsm.characters_.clear();
  fsm.stream_ << "</" << element_name << '>';
  if (element_name == fsm.parent_node_) {
    fsm.state_ = State::kWaitingForParentNode;
  } else {
    fsm.state_ = State::kOutsideNode;
  }
}

// SAX2 handler for when an element start has been detected by the parser.
// It provides the namespace information for the element,
// as well as the new namespace declarations on the element.
// context: The user data (XML parser context)
// localname: The local name of the element
// prefix: The element namespace prefix if available
// URI: The element namespace name if available
// nb_namespaces: Number of namespace definitions on that node
// namespaces: Pointer to the array of prefix/URI pairs namespace definitions
// nb_attributes: The number of attributes on that node
// nb_defaulted: The number of defaulted attributes.
//               The defaulted ones are at the end of the array.
// attributes: pointer to the array of (localname/prefix/URI/value/end)
//             attribute values.
void SaxFsm::handler_start_element(void* context, const xmlChar* local_name,
                                   const xmlChar*, const xmlChar*, int,
                                   const xmlChar**, int num_attributes, int,
                                   const xmlChar** attributes) {
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  std::string element_name{reinterpret_cast<const char*>(local_name)};
  if (fsm.state_ == State::kWaitingForParentNode) {
    if (element_name.compare(fsm.parent_node_) == 0) {
      auto attribute_name = (fsm.parent_node_ == KChannel) ? kId : KChannel;
      std::string tvg_id =
          fsm.get_attribute_value(attribute_name, num_attributes, attributes);
      if (!fsm.playlist_.is_tvg_id_in_playlist(tvg_id)) return;
      fsm.indentation_ = 1;
    } else {
      return;
    }
  } else if (fsm.state_ == State::kInsideNode) {
    fsm.indentation_++;
  }
  fsm.characters_.clear();
  fsm.stream_ << '\n';
  for (int i{}; i < fsm.indentation_; i++) fsm.stream_ << '\t';
  fsm.stream_ << '<' << element_name;
  unsigned int index = 0;
  for (int i = 0; i < num_attributes; ++i, index += 5) {
    const xmlChar* localname = attributes[index];
    const xmlChar* valueBegin = attributes[index + 3];
    const xmlChar* valueEnd = attributes[index + 4];
    std::string value((const char*)valueBegin, (const char*)valueEnd);
    fsm.stream_ << ' ' << localname << R"(=")" << value << '"';
  }
  fsm.stream_ << '>';
  fsm.current_node_name_ = element_name;
  fsm.state_ = State::kInsideNode;
}

}  // namespace pefti
