#include "resource.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <functional>
#include <memory>
#include <ranges>
#include <sstream>
#include <string_view>

// Using libcurl to download resources
#include <curl/curl.h>
#include <curl/mprintf.h>

namespace pefti {

using EasyHandle = std::unique_ptr<CURL, std::function<void(CURL*)>>;

static void load_resource(const std::string& url, std::string& resource);
static size_t write_callback(void* ptr, size_t size, size_t nmemb,
                             void* userdata);

// Initialises libcurl on application start and tidies up on application exit
class CurlGlobalStateGuard {
 public:
  CurlGlobalStateGuard() { curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~CurlGlobalStateGuard() { curl_global_cleanup(); }
};
static CurlGlobalStateGuard handle_curl_state;

// Loads multiple resources, returns each resource in a std::string.
std::vector<std::string> load_resources(const std::vector<std::string>& urls) {
  std::vector<std::string> resources(urls.size());
  for (std::size_t i{}; i < urls.size(); i++)
    load_resource(urls[i], resources[i]);
  return resources;
}

static void load_resource(const std::string& url, std::string& resource) {
  auto handle = EasyHandle(curl_easy_init(), curl_easy_cleanup);
  if (!handle) throw std::runtime_error("curl_easy_init() returned NULL");
  CURLcode code;
  code = curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, 0L);
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, 0L);
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, write_callback);
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &resource);
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_perform(handle.get());
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
}

static size_t write_callback(void* ptr, size_t size, size_t nmemb,
                             void* user_data) {
  std::string& data = *static_cast<std::string*>(user_data);
  size_t total_size = size * nmemb;
  data.append(static_cast<char*>(ptr), total_size);
  return total_size;
}

}  // namespace pefti
