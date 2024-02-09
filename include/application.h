#pragma once

#include "config.h"
#include "filter.h"
#include "sorter.h"
#include "transformer.h"

namespace pefti {

class Application {
 public:
  Application()
      : m_config(std::make_shared<Config>()),
        m_filter(std::make_unique<Filter>(m_config)),
        m_transformer(std::make_unique<Transformer>(m_config)),
        m_sorter(std::make_unique<Sorter>(m_config)) {}
  Application(const Application&) = delete;
  Application(const Application&&) = delete;
  void operator=(Application&) = delete;
  void operator=(Application&&) = delete;
  void run();

 private:
  std::shared_ptr<Config> m_config;
  std::unique_ptr<Filter> m_filter;
  std::unique_ptr<Transformer> m_transformer;
  std::unique_ptr<Sorter> m_sorter;
};

}  // namespace pefti
