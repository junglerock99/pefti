#include "resource.h"

#include <curl/curl.h>

#include <exception>
#include <gsl/gsl>

namespace pefti {

static size_t write_callback(void* ptr, size_t size, size_t nmemb,
                             void* userdata);

// Initialises libcurl on application start and tidies up on application exit
class CurlGlobalStateGuard {
 public:
  CurlGlobalStateGuard() { curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~CurlGlobalStateGuard() { curl_global_cleanup(); }
};
static CurlGlobalStateGuard handle_curl_state;

void add_transfer(CURLM* multi_handle, const char* url, std::string& output) {
  auto easy_handle = curl_easy_init();
  if (!easy_handle) throw std::runtime_error("curl_easy_init() returned NULL");
  CURLcode ecode;
  ecode = curl_easy_setopt(easy_handle, CURLOPT_URL, url);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  ecode = curl_easy_setopt(easy_handle, CURLOPT_PRIVATE, url);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  ecode = curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  ecode = curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &output);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  ecode = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  ecode = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
  if (ecode != CURLE_OK) throw std::runtime_error(curl_easy_strerror(ecode));
  CURLMcode mcode = curl_multi_add_handle(multi_handle, easy_handle);
  if (mcode != CURLM_OK) throw std::runtime_error(curl_multi_strerror(mcode));
}

// Loads multiple resources concurrently.
// Returns each resource in a std::string.
std::vector<std::string> load_resources(const std::vector<std::string>& urls) {
  const int num_resources{static_cast<int>(urls.size())};
  std::vector<std::string> resources(num_resources);
  auto mhandle = curl_multi_init();
  if (!mhandle) throw std::runtime_error("curl_multi_init() returned NULL");
  CURLMcode mcode;
  mcode = curl_multi_setopt(mhandle, CURLMOPT_MAXCONNECTS,
                            static_cast<long>(num_resources));
  if (mcode != CURLM_OK) throw std::runtime_error(curl_multi_strerror(mcode));
  for (int i{0}; i < num_resources; ++i)
    add_transfer(mhandle, urls[i].c_str(), resources[i]);
  int num_transfers_running{num_resources};
  do {
    mcode = curl_multi_perform(mhandle, &num_transfers_running);
    if (mcode != CURLM_OK) throw std::runtime_error(curl_multi_strerror(mcode));
    CURLMsg* msg;
    int num_msgs_in_queue;
    while ((msg = curl_multi_info_read(mhandle, &num_msgs_in_queue))) {
      if (msg->msg == CURLMSG_DONE) {
        CURL* ehandle = msg->easy_handle;
        curl_multi_remove_handle(mhandle, ehandle);
        curl_easy_cleanup(ehandle);
      }
    }
    if (num_transfers_running > 0) {
      mcode = curl_multi_wait(mhandle, NULL, 0, 1000, NULL);
      if (mcode != CURLM_OK)
        throw std::runtime_error(curl_multi_strerror(mcode));
    }
  } while (num_transfers_running > 0);
  mcode = curl_multi_cleanup(mhandle);
  if (mcode != CURLM_OK) throw std::runtime_error(curl_multi_strerror(mcode));
  Ensures(resources.size() == urls.size());
  return resources;
}

static size_t write_callback(void* ptr, size_t size, size_t nmemb,
                             void* user_data) {
  std::string& data = *static_cast<std::string*>(user_data);
  size_t total_size = size * nmemb;
  data.append(static_cast<char*>(ptr), total_size);
  return total_size;
}

}  // namespace pefti
