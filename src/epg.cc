#include "epg.h"

#include <string>
#include <vector>

#include "resource.h"

namespace pefti {

std::vector<std::string> load_epgs(const std::vector<std::string>& urls) {
  return load_resources(urls);
}

}  // namespace pefti
