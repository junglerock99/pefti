#ifndef PEFTI_TRANSFORMER_H_
#define PEFTI_TRANSFORMER_H_

#include <memory>
#include <vector>

#include "config.h"
#include "playlist.h"

namespace pefti {

// Contains the application logic for transforming a playlist.
class Transformer {
 public:
  Transformer(std::shared_ptr<Config> config) : m_config(config) {}
  ~Transformer() = default;
  Transformer(Transformer&) = delete;
  Transformer(Transformer&&) = delete;
  Transformer& operator=(Transformer&) = delete;
  Transformer& operator=(Transformer&&) = delete;
  void transform(Playlist& playlist);

 private:
  void set_tags(Playlist& playlist);
  void set_quality(Playlist& playlist);
  void set_name(Playlist& playlist);

 private:
  std::shared_ptr<Config> m_config;
};

}  // namespace pefti

#endif  // PEFTI_TRANSFORMER_H_
