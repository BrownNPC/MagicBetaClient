#include <SDL3/SDL_gpu.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

#include "net.h"

bool __curl_init() {
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  return res == CURLE_OK;
}
void __curl_deinit() {
  curl_global_cleanup();
}
// NOTE: hostname must be prefixed with "http://"
CurlCreateSocketResult NET_CurlCreateSocket(string hostname) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return Err(CurlCreateSocket, str("failed to init curl"));
  }
  curl_easy_setopt(curl, CURLOPT_URL, hostname.items);
  // raw TCP connection only
  curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

  // connect to the server.
  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    return Err(CurlCreateSocket, str(curl_easy_strerror(result)));
  }
  return Ok(CurlCreateSocket, curl);
}
// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
int NET_CurlReadFromSocket(CURL* curl, void* buffer, size_t buflen) {
  size_t n;
  CURLcode res = curl_easy_recv(curl, buffer, buflen, &n);
  if (res == CURLE_AGAIN)
    return 0;
  if (res != CURLE_OK) {
    return -1;
  }
  return n;
}

// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
int NET_CurlWriteToSocket(CURL* curl, const void* buffer, size_t buflen) {
  size_t n;
  CURLcode res = curl_easy_send(curl, buffer, buflen, &n);
  if (res == CURLE_AGAIN)
    return 0;
  if (res != CURLE_OK) {
    return -1;
  }
  return n;
}
