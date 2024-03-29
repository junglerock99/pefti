#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "config.h"

namespace pefti {

class IptvChannel;
class Playlist;

class ChannelsMapper {
 public:
  const ChannelTemplate& get_channel_template(IptvChannel& iptv_channel);
  bool is_allowed_channel(IptvChannel& iptv_channel);
  std::vector<IptvChannel*>& map_template_to_channel(
      ChannelTemplate& channel_template);
  std::optional<ChannelTemplate*> map_channel_to_template(
      IptvChannel& iptv_channel);
  void populate_maps();
  void set_config(ConfigType& config);
  void set_playlist(Playlist& playlist);

 private:
  typedef std::unordered_map<ChannelTemplate*, std::vector<IptvChannel*>>
      TemplateToChannelsMapper;
  typedef std::unordered_map<IptvChannel*, ChannelTemplate*>
      ChannelToTemplateMap;
  typedef std::unordered_map<std::string, ChannelTemplate*> NameToTemplateMap;

  // Maps IPTV channel names to channel templates
  NameToTemplateMap m_name_to_template_map;

  // Maps channel templates to IPTV channels
  TemplateToChannelsMapper m_template_to_channels_map;

  // Maps IPTV channels to channel templates
  ChannelToTemplateMap m_channel_to_template_map;

  ConfigType* m_config;
  Playlist* m_playlist;
};

}  // namespace pefti
