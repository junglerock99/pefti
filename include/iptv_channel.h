#ifndef PEFTI_IPTV_CHANNEL_H_
#define PEFTI_IPTV_CHANNEL_H_

#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pefti {

class IptvChannel {
 public:
  using Tag = std::pair<std::string, std::string>;
  IptvChannel() = default;
  ~IptvChannel() = default;
  IptvChannel(const IptvChannel&) = default;
  IptvChannel(IptvChannel&&) = default;
  IptvChannel& operator=(const IptvChannel&) = default;
  IptvChannel& operator=(IptvChannel&&) = default;
  bool contains_tag(std::string_view tag);
  void delete_tag(std::string_view tag);
  void delete_tags() { m_tags.clear(); }
  const std::string& get_new_name() { return m_new_name; }
  const std::string& get_original_name() { return m_original_name; }
  const std::string& get_tag(std::string_view tag);
  const std::string& get_url() { return m_url; }
  void set_new_name(std::string_view new_name) { m_new_name = new_name; }
  void set_original_name(std::string_view new_name);
  void set_tag(std::string_view tag, std::string_view new_value);
  void set_tag(std::string_view tag, std::string& new_value);
  void set_tag(std::string_view tag, std::string&& new_value);
  void set_tag(std::string_view tag, const std::string& new_value);
  void set_tags(const std::vector<Tag>& tags);
  void set_url(std::string_view new_url) { m_url = new_url; }

 private:
  std::string m_new_name;
  std::string m_original_name;
  std::string m_url;
  std::unordered_map<std::string, std::string> m_tags{};

 public:
  friend std::istream& operator>>(std::istream&, IptvChannel&);
  friend std::ostream& operator<<(std::ostream&, IptvChannel&);
};

}  // namespace pefti

#endif  // PEFTI_IPTV_CHANNEL_H_
