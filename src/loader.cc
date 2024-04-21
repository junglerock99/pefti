#include "loader.h"

#include <curl/curl.h>

#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "buffers.h"

namespace pefti {

using EasyHandle = std::unique_ptr<CURL, std::function<void(CURL*)>>;

static size_t write_callback(void* new_data, size_t, size_t data_size,
                             void* context);
cppcoro::task<size_t> write_to_buffer(void* new_data, size_t num_chars,
                                      void* context);

static void load_resource(const std::string& url,
                          PlaylistLoaderParserBuffer& buffer) {
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
  code = curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &buffer);
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
  code = curl_easy_perform(handle.get());
  if (code != CURLE_OK) throw std::runtime_error(curl_easy_strerror(code));
}

cppcoro::task<> Loader::load(cppcoro::static_thread_pool& tp,
                             PlaylistLoaderParserBuffer& buffer,
                             const std::string& url) {
  co_await tp.schedule();
  buffer.thread_pool = &tp;
  load_resource(url, buffer);
  write_playlist_sentinel(buffer);
}

// Callback used by libcurl when data has been received.
// Writes the new data to the ring buffer.
static size_t write_callback(void* new_data, size_t, size_t num_chars,
                             void* context) {
  auto num_chars_written =
      cppcoro::sync_wait(write_to_buffer(new_data, num_chars, context));
  return num_chars_written;
}

// Writes data to the ring buffer.
cppcoro::task<size_t> write_to_buffer(void* new_data, size_t num_chars,
                                      void* context) {
  char* data = static_cast<char*>(new_data);
  PlaylistLoaderParserBuffer& buffer =
      *static_cast<PlaylistLoaderParserBuffer*>(context);
  const auto kBufferSize = buffer.get_size();
  const auto kIndexMask = buffer.get_index_mask();
  size_t num_chars_remaining{num_chars};
  while (num_chars_remaining > 0) {
    auto num_slots_claimed = (std::min(num_chars_remaining, kBufferSize >> 1));
    auto write_range = co_await buffer.sequencer.claim_up_to(
        num_slots_claimed, *(buffer.thread_pool));
    const auto num_slots_available = *write_range.end() - *write_range.begin();
    const auto start_block = *write_range.begin() / kBufferSize;
    const auto end_block = (*write_range.end() - 1) / kBufferSize;
    const bool will_overflow = (start_block != end_block);
    if (!will_overflow) [[likely]] {
      std::memcpy(&buffer.data[*write_range.begin() & kIndexMask], data,
                  num_slots_available);
    } else {
      auto distance_to_end = kBufferSize - (*write_range.begin() & kIndexMask);
      std::memcpy(&buffer.data[*write_range.begin() & kIndexMask], data,
                  distance_to_end);
      std::memcpy(&buffer.data[0], data + distance_to_end,
                  num_slots_available - distance_to_end);
    }
    num_chars_remaining -= num_slots_available;
    data += num_slots_available;
    buffer.sequencer.publish(*write_range.end() - 1);
  }
  co_return num_chars;
}

// Writes a sentinel to the buffer.
void Loader::write_playlist_sentinel(PlaylistLoaderParserBuffer& buffer) {
  cppcoro::sync_wait(write_to_buffer(
      static_cast<void*>(const_cast<char8_t*>(kPlaylistSentinel.c_str())),
      kPlaylistSentinelSize, &buffer));
}

}  // namespace pefti
