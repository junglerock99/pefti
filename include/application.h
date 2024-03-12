#pragma once

#include "channels_mapper.h"
#include "config.h"
#include "filter.h"
#include "playlist.h"
#include "transformer.h"

namespace pefti {

class Application {
 public:
  Application(std::string&& config_filename);
  Application(const Application&) = delete;
  Application(const Application&&) = delete;
  void operator=(Application&) = delete;
  void operator=(Application&&) = delete;
  void run();

 private:
  ConfigType m_config;
  Playlist m_playlist;
  Filter m_filter;
  Transformer m_transformer;
  ChannelsMapper m_channels_mapper;
};

}  // namespace pefti
