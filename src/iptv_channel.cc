#include "iptv_channel.h"

#include <fstream>
#include <optional>
#include <ranges>
#include <regex>
#include <string>

using namespace std::literals;

namespace pefti {

void get_key_value_pairs(std::string& line, IptvChannel& channel);

bool IptvChannel::contains_tag(std::string_view tag_name) {
  return m_tags.contains(std::string{tag_name});
}

void IptvChannel::delete_tag(std::string_view tag_name) {
  if (contains_tag(tag_name)) m_tags.erase(std::string{tag_name});
}

// Parses key=value pairs and adds them to the channel
void get_key_value_pairs(std::string& line, IptvChannel& channel) {
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

std::optional<std::string> IptvChannel::get_tag_value(
    std::string_view tag_name) {
  if (m_tags.contains(std::string{tag_name})) {
    return make_optional<std::string>(m_tags[std::string{tag_name}]);
  } else {
    return std::nullopt;
  }
}

// Writes a channel to the output stream
std::ostream& operator<<(std::ostream& stream, IptvChannel& channel) {
  std::string line("#EXTINF:-1");
  for (const auto& tag : channel.m_tags) {
    line += ' ' + tag.first + "=\"" + tag.second + '"';
  }
  line += ',';
  line += channel.m_new_name;
  stream << line << '\n';
  // stream << "# " << channel.get_original_name() << '\n';
  stream << channel.m_url << '\n';
  return stream;
}

// Parses a channel from the input stream
std::istream& operator>>(std::istream& stream, IptvChannel& channel) {
  std::string line;
  channel.delete_tags();
  bool have_channel{false};
  while (stream && !have_channel) {
    std::getline(stream, line);
    if (line.starts_with("#EXTINF"sv)) {
      //
      // Get channel name by finding first comma after last key=value pair
      auto pos = line.rfind('=');
      pos = line.find(',', (pos != std::string::npos) ? pos : 0);
      if (pos != std::string::npos) {
        channel.set_original_name(line.substr(pos + 1));
        channel.set_new_name(line.substr(pos + 1));
        line.resize(pos);  // Removes channel name
        get_key_value_pairs(line, channel);
        while (stream && !have_channel) {
          if (stream) {
            //
            // Get URL
            std::getline(stream, line);
            if (line.compare(0, 4, "http"sv, 0, 4) == 0) {
              channel.set_url(line);
              have_channel = true;
            }
          }
        }
      }
    }
  }
  return stream;
}

void IptvChannel::set_original_name(std::string_view new_name) {
  m_original_name = new_name;
}

void IptvChannel::set_tag(std::string_view tag, std::string_view new_value) {
  m_tags[std::string{tag}] = std::string{new_value};
}

void IptvChannel::set_tag(std::string_view tag, std::string& new_value) {
  m_tags[std::string{tag}] = new_value;
}

void IptvChannel::set_tag(std::string_view tag, std::string&& new_value) {
  m_tags[std::string{tag}] = std::move(new_value);
}

void IptvChannel::set_tag(std::string_view tag, const std::string& new_value) {
  m_tags[std::string{tag}] = new_value;
}

void IptvChannel::set_tags(const std::vector<Tag>& tags) {
  for (auto& tag : tags) m_tags[tag.first] = tag.second;
}

}  // namespace pefti
