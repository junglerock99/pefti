#pragma once

#include <string>
#include <vector>

namespace pefti {

// Loads multiple resources from URLs. Returns the contents of each resource 
// in a string.
std::vector<std::string> load_resources(const std::vector<std::string>& urls);

}  // namespace pefti
