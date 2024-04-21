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
  return tags_.contains(std::string{tag_name});
}

void IptvChannel::delete_tag(std::string_view tag_name) {
  if (contains_tag(tag_name)) tags_.erase(std::string{tag_name});
}

std::optional<std::string> IptvChannel::get_tag_value(
    std::string_view tag_name) {
  if (tags_.contains(std::string{tag_name})) {
    return make_optional<std::string>(tags_[std::string{tag_name}]);
  } else {
    return std::nullopt;
  }
}

// Writes a channel to the output stream
std::ostream& operator<<(std::ostream& stream, IptvChannel& channel) {
  std::string line("#EXTINF:-1");
  for (const auto& tag : channel.tags_) {
    line += ' ' + tag.first + "=\"" + tag.second + '"';
  }
  line += ',';
  line += channel.new_name_;
  stream << line << '\n';
  // stream << "# " << channel.get_original_name() << '\n';
  stream << channel.url_ << '\n';
  return stream;
}

void IptvChannel::set_original_name(std::string_view new_name) {
  original_name_ = new_name;
}

void IptvChannel::set_tag(std::string_view tag, std::string_view new_value) {
  tags_[std::string{tag}] = std::string{new_value};
}

void IptvChannel::set_tag(std::string_view tag, std::string& new_value) {
  tags_[std::string{tag}] = new_value;
}

void IptvChannel::set_tag(std::string_view tag, std::string&& new_value) {
  tags_[std::string{tag}] = std::move(new_value);
}

void IptvChannel::set_tag(std::string_view tag, const std::string& new_value) {
  tags_[std::string{tag}] = new_value;
}

void IptvChannel::set_tags(const std::vector<Tag>& tags) {
  for (auto& tag : tags) tags_[tag.first] = tag.second;
}

}  // namespace pefti
