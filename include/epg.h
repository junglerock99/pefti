#pragma once

#include <string>
#include <vector>

namespace pefti {

class Epg {};

std::vector<std::string> load_epgs(const std::vector<std::string>& urls);

}  // namespace pefti
