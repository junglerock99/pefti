#include "epg.h"

#include <curl/curl.h>
#include <curl/mprintf.h>

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "resource.h"

namespace pefti {

std::vector<std::string> load_epgs(const std::vector<std::string>& urls) {
  return load_resources(urls);
}

void store_epg(std::string_view filename, Epg&) {
  std::ofstream file{std::string(filename)};
  file.close();
}

}  // namespace pefti
