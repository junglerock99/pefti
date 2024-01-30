#include "playlist.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string_view>
#include <exception>

#include <curl/curl.h>
#include <curl/mprintf.h>

#include "iptv_channel.h"

namespace pefti {

void load_playlists_cleanup(CURLM*& multi_handle, 
                            std::vector<CURL*>& http_handles);
void load_playlists_setup(CURLM*& multi_handle, 
                          std::vector<CURL*>& http_handles, 
                          std::vector<std::string>& playlists, 
                          const std::vector<std::string>& filenames);
void load_playlists_transfer(CURLM*& multi_handle);
static size_t write_callback(
  void* ptr, size_t size, size_t nmemb, void* userdata);

// Loads multiple playlists simultaneously using libcurl's asynchronous method.
// Each playlist is stored in a std::string.
// TODO: add a configuration option to limit the amount of RAM used for loading
// playlists, so that large playlists can be written to a file instead of being
// stored in RAM.
std::vector<std::string> load_playlists(
    const std::vector<std::string>& filenames) {
  std::vector<std::string> playlists(filenames.size());
  std::vector<CURL*> http_handles(filenames.size());
  CURLM* multi_handle;
  load_playlists_setup(multi_handle, http_handles, playlists, filenames);
  load_playlists_transfer(multi_handle);
  load_playlists_cleanup(multi_handle, http_handles);
  return playlists;
}

void store_playlist(std::string_view filename, Playlist& playlist) {
  std::ofstream file{std::string(filename)};
  file << "#EXTM3U\n";
  for (auto& channel : playlist) file << channel;
  file.close();
}

void load_playlists_cleanup(CURLM*& multi_handle, 
                            std::vector<CURL*>& http_handles) {
  std::for_each(http_handles.begin(), http_handles.end(), 
    [&multi_handle](auto& http_handle){
      CURLMcode code = curl_multi_remove_handle(multi_handle, http_handle);
      if (code != CURLM_OK)
        throw std::runtime_error(curl_multi_strerror(code)); });
  curl_multi_cleanup(multi_handle);
  std::for_each(http_handles.begin(), http_handles.end(), 
    [](auto& http_handle){ curl_easy_cleanup(http_handle); });
  curl_global_cleanup();
}

void load_playlists_setup(CURLM*& multi_handle, 
                          std::vector<CURL*>& http_handles, 
                          std::vector<std::string>& playlists, 
                          const std::vector<std::string>& filenames) {
  CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (code != CURLE_OK)
    throw std::runtime_error(curl_easy_strerror(code));
  std::for_each(http_handles.begin(), http_handles.end(), 
    [](auto& http_handle){ 
      http_handle = curl_easy_init(); 
      if (http_handle == NULL)
        throw std::runtime_error("curl_easy_init() returned NULL"); });
  for (std::size_t i{}; i < http_handles.size(); i++) {
    CURLcode code;
    code = curl_easy_setopt(http_handles[i], 
                            CURLOPT_URL, 
                            filenames[i].c_str());
    if (code != CURLE_OK)
      throw std::runtime_error(curl_easy_strerror(code));
    code = curl_easy_setopt(http_handles[i], 
                            CURLOPT_WRITEFUNCTION, 
                            write_callback);
    if (code != CURLE_OK)
      throw std::runtime_error(curl_easy_strerror(code));
    code = curl_easy_setopt(http_handles[i], 
                            CURLOPT_WRITEDATA, 
                            &playlists[i]);
    if (code != CURLE_OK)
      throw std::runtime_error(curl_easy_strerror(code));
  }
  multi_handle = curl_multi_init();
  if (multi_handle == NULL)
    throw std::runtime_error("curl_multi_init() returned NULL");
  std::for_each(http_handles.begin(), http_handles.end(), 
    [&multi_handle](auto& http_handle){
      CURLMcode code =
        curl_multi_add_handle(multi_handle, http_handle);
      if (code != CURLM_OK)
        throw std::runtime_error(curl_multi_strerror(code)); });
}

void load_playlists_transfer(CURLM*& multi_handle) {
  int is_still_running = 1;
  while (is_still_running) {
    CURLMcode code = curl_multi_perform(multi_handle, &is_still_running);
    if (code != CURLM_OK)
      throw std::runtime_error(curl_multi_strerror(code));
    if (is_still_running)
      code = curl_multi_poll(multi_handle, NULL, 0, 1000, NULL);
    if (code)
      break;
    CURLMsg* msg;
    do {
      int queued;
      msg = curl_multi_info_read(multi_handle, &queued);
    } while(msg);
  }
}

static size_t write_callback(void* ptr, 
                             size_t size, 
                             size_t nmemb,
                             void* userdata)
{
  std::string& data = *static_cast<std::string*>(userdata);
  size_t total_size = size * nmemb;
  data.append(static_cast<char*>(ptr), total_size);
  return total_size;
}

}  // namespace pefti
