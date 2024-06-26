#include "mapper.h"

#include <optional>
#include <ranges>

#include "config.h"
#include "playlist.h"

namespace pefti {

void to_lower(std::string_view original, std::string& lowercase);

const ChannelTemplate& ChannelsMapper::get_channel_template(
    IptvChannel& iptv_channel) {
  return *(map_channel_to_template(iptv_channel).value());
}

// Checks if an IPTV channel has a corresponding channel template
bool ChannelsMapper::is_allowed_channel(IptvChannel& iptv_channel) {
  return map_channel_to_template(iptv_channel) != std::nullopt;
}

// Case insensitive comparisons are performed by converting both text
// strings to lowercase. Comparing a channel name to all of the templates
// in the configuration is an expensive operation so the results are
// cached in m_name_to_template_map.
std::optional<ChannelTemplate*> ChannelsMapper::map_channel_to_template(
    IptvChannel& iptv_channel) {
  const auto& name = iptv_channel.get_original_name();
  if (name_to_template_map_.contains(name)) return name_to_template_map_[name];
  //
  // Convert channel_name to lowercase
  std::string name_lower{};
  name_lower.reserve(name.size());
  to_lower(name, name_lower);
  //
  // Checks if `search_text` is contained within the channel name
  auto channel_name_contains = [&name_lower](const auto& search_text) {
    std::string search_text_lower{};
    search_text_lower.reserve(search_text.size());
    to_lower(search_text, search_text_lower);
    auto pos = name_lower.find(search_text_lower);
    if (pos != std::string::npos) {
      bool char_before_is_alphanumeric{
          (pos != 0) && (std::isalnum(name_lower[pos - 1]) != 0)};
      bool char_after_is_alphanumeric{
          std::isalnum(name_lower[pos + search_text_lower.length()]) != 0};
      if (!char_before_is_alphanumeric && !char_after_is_alphanumeric)
        return true;
    }
    return false;
  };
  for (auto& ct : config_->config_.channels_templates) {
    bool is_included = std::all_of(ct.include.cbegin(), ct.include.cend(),
                                   channel_name_contains);
    bool is_excluded = std::any_of(ct.exclude.cbegin(), ct.exclude.cend(),
                                   channel_name_contains);
    if (is_included && !is_excluded) {
      // Add this result to the cache
      name_to_template_map_[name] = &ct;
      return &ct;
    }
  }
  return std::nullopt;
}

std::vector<IptvChannel*>& ChannelsMapper::map_template_to_channel(
    ChannelTemplate& channel_template) {
  return template_to_channels_map_[&channel_template];
}

// name_to_template_map_ gets populated during filtering,
// now we can use it to populate the other maps.
void ChannelsMapper::populate_maps() {
  std::ranges::for_each(*playlist_, [this](auto&& iptv_channel) {
    const auto& name = iptv_channel.get_original_name();
    if (name_to_template_map_.contains(name)) {
      auto channel_template = name_to_template_map_[name];
      channel_to_template_map_[&iptv_channel] = channel_template;
      template_to_channels_map_[channel_template].push_back(&iptv_channel);
    }
  });
}

void ChannelsMapper::set_config(ConfigType& config) { config_ = &config; }

void ChannelsMapper::set_playlist(Playlist& playlist) { playlist_ = &playlist; }

void to_lower(std::string_view original, std::string& lowercase) {
  std::transform(original.cbegin(), original.cend(),
                 std::back_inserter(lowercase),
                 [](unsigned char c) { return std::tolower(c); });
}

}  // namespace pefti
