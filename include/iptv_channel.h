#pragma once

#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std::string_view_literals;

namespace pefti {

class IptvChannel {
 public:
  static constexpr auto kTagDelete = "delete"sv;
  static constexpr auto kTagGroupTitle = "group-title"sv;
  static constexpr auto kTagQuality = "quality"sv;
  static constexpr auto kTagTvgId = "tvg-id"sv;

 public:
  using Tag = std::pair<std::string, std::string>;
  IptvChannel() = default;
  ~IptvChannel() = default;
  IptvChannel(const IptvChannel&) = default;
  IptvChannel(IptvChannel&&) = default;
  IptvChannel& operator=(const IptvChannel&) = default;
  IptvChannel& operator=(IptvChannel&&) = default;
  bool contains_tag(std::string_view tag_name);
  void delete_tag(std::string_view tag_name);
  void delete_tags() { tags_.clear(); }
  const std::string& get_new_name() { return new_name_; }
  const std::string& get_original_name() { return original_name_; }
  std::optional<std::string> get_tag_value(std::string_view tag_name);
  const std::string& get_url() { return url_; }
  void set_new_name(std::string_view new_name) { new_name_ = new_name; }
  void set_original_name(std::string_view new_name);
  void set_tag(std::string_view tag, std::string_view new_value);
  void set_tag(std::string_view tag, std::string& new_value);
  void set_tag(std::string_view tag, std::string&& new_value);
  void set_tag(std::string_view tag, const std::string& new_value);
  void set_tags(const std::vector<Tag>& tags);
  void set_url(std::string_view new_url) { url_ = new_url; }

 private:
  std::string new_name_;
  std::string original_name_;
  std::string url_;
  std::unordered_map<std::string, std::string> tags_{};

 public:
  friend std::ostream& operator<<(std::ostream&, IptvChannel&);
};

}  // namespace pefti
