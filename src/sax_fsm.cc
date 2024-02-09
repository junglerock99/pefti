#include <algorithm>
#include <cctype>
#include <string>

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "playlist.h"
#include "sax_fsm.h"

namespace pefti {

SaxFsm::SaxFsm(std::string_view parent_node, 
       std::ofstream& stream, 
       Playlist& playlist)
  : m_parent_node(parent_node), m_stream(stream), m_playlist(playlist) {
}

// SAX2 handler when an element start has been detected by the parser. 
// It provides the namespace informations for the element, 
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
//   attribute values.
void SaxFsm::handler_start_element(void* context, 
                                   const xmlChar* local_name, 
                                   const xmlChar*,
                                   const xmlChar*,
                                   int,
                                   const xmlChar**,
                                   int num_attributes, 
                                   int,
                                   const xmlChar** attributes) {
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  std::string element_name{reinterpret_cast<const char*>(local_name)};
  if (fsm.m_state == State::kWaitingForParentNode) {
    if (element_name.compare(fsm.m_parent_node) == 0) {
      auto attribute_name = (fsm.m_parent_node == KChannel) ? kId : KChannel;
      std::string tvg_id = fsm.get_attribute_value(attribute_name,
          num_attributes, attributes);
      if (!fsm.m_playlist.is_tvg_id_in_playlist(tvg_id)) return;
      fsm.m_indentation = 1;
    } else { 
      return;
    }
  } else if (fsm.m_state == State::kInsideNode) {
    fsm.m_indentation++;
  }
  fsm.m_characters.clear();
  fsm.m_stream << '\n';
  for (int i{}; i < fsm.m_indentation; i++) fsm.m_stream << '\t';
  fsm.m_stream << '<' << element_name;
  unsigned int index = 0;
  for (int i = 0; i < num_attributes; ++i, index += 5 ) {
    const xmlChar* localname = attributes[index];
    const xmlChar* valueBegin = attributes[index + 3];
    const xmlChar* valueEnd = attributes[index + 4];
    std::string value((const char*)valueBegin, (const char*)valueEnd );
        fsm.m_stream << ' ' << localname << R"(=")" << value << '"';
  }
  fsm.m_stream << '>';
  fsm.m_current_node_name = element_name;
  fsm.m_state = State::kInsideNode;
}

// SAX2 handler when an element end has been detected by the parser. 
// It provides the namespace informations for the element.
// context: The user data (XML parser context)
// localname: The local name of the element
// prefix: The element namespace prefix if available
// URI: The element namespace name if available
void SaxFsm::handler_end_element(void* context, 
                                   const xmlChar* local_name, 
                                   const xmlChar*,
                                   const xmlChar*) { 
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  std::string element_name{reinterpret_cast<const char*>(local_name)};
  switch (fsm.m_state) {
    case State::kWaitingForParentNode: return;
    case State::kInsideNode: {
      if (element_name != fsm.m_current_node_name)
        throw std::runtime_error("Invalid XML");
      if (!std::all_of(fsm.m_characters.begin(), fsm.m_characters.end(),
                       isspace)) {
        fsm.m_stream << fsm.m_characters;
      }
    } break;
    case State::kOutsideNode:
      fsm.m_stream << '\n';
      fsm.m_indentation--;
      for (int i{}; i < fsm.m_indentation; i++) fsm.m_stream << '\t';
      break;
  }
  fsm.m_characters.clear();
  fsm.m_stream << "</" << element_name << '>';
  if (element_name == fsm.m_parent_node) {
    fsm.m_state = State::kWaitingForParentNode;
  } else {
    fsm.m_state = State::kOutsideNode;
  }
}

// SAX2 handler for characters between start and end elements
void SaxFsm::handler_characters(void* context, const xmlChar* begin, 
                                int length) {
  SaxFsm& fsm = *(static_cast<SaxFsm*>(context));
  if (fsm.m_state == State::kInsideNode) {
    auto end = begin + length;
    std::for_each(begin, end, [&fsm](unsigned char c) {
      if (!std::isspace(c) || c == ' ')
        fsm.m_characters.push_back(c);
    });
  }
}

std::string SaxFsm::get_attribute_value(std::string_view attribute_name, 
                                        int num_attributes, 
                                        const xmlChar** attributes) {
  std::string value{};
  unsigned int index = 0;
  for (int i = 0; i < num_attributes; ++i, index += 5 ) {
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

}  // namespace pefti

/*
<?xml version="1.0" encoding="utf-8"?>)" << '\n';
<!DOCTYPE tv SYSTEM "xmltv.dtd">)" << '\n';
<tv generator-info-name="pefti">
  <channel id="BBC One HD">
    <display-name lang="en">BBC One HD</display-name>
    <icon src="https://d2n0069hmnqmmx.cloudfront.net/epgdata/1.0/newchanlogos/320/320/skychb2076.png" />
    <url>http://www.tv.sky.com</url>
  </channel>
  <programme start="20240131001500 +0000" stop="20240131004500 +0000" channel="BBC One HD">
    <title lang="en">Celebrity Mastermind</title>
    <desc lang="en">9/14. Clive Myrie asks the questions as Jonathan Agnew, YolanDa Brown, Davood Ghadami and Jessica Knappett go under the spotlight in the celebrity version of the classic quiz. [S] [HD.</desc>
    <category lang="en">10009</category>
    <rating system="UK">
      <value>--</value>
    </rating>
  </programme>
</tv>
*/