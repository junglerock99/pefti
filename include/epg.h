#pragma once

#include <string_view>
#include <vector>

//#include <libxml++/libxml++.h>

namespace pefti {

class Epg {
};

std::vector<std::string> load_epgs(const std::vector<std::string>& urls);
void store_epg(std::string_view filename, Epg& epg);

}  // namespace pefti
