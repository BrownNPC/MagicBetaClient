//go:build ignore

#include "curl.h"
#include <curl/curl.h>
#include <stdint.h>
#include <string.h>
#include "so/builtin/builtin.h"

bool CurlInit() {
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  return res == CURLE_OK;
}
void CurlDeInit() {
  curl_global_cleanup();
}
// Closes the socket. Does not wait for all data to be sent.
void CurlCloseSocket(CURL* curl) {
  curl_easy_cleanup(curl);
}
so_String CurlErrorimpl(void* self){
  CURLcode code = (intptr_t)self;
  return so_str(curl_easy_strerror(code));
}
so_Error newCurlError(CURLcode code) {
  auto s = so_str(curl_easy_strerror(code));
  return (so_Error){
      .self = (void*)(intptr_t)code,
      .Error = CurlErrorimpl,
  };
}
// NOTE: hostname must be prefixed with "http://"
so_Error CurlCreateSocket(const char* hostname, CURL** curlRet) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    return newCurlError(CURLE_FAILED_INIT);
  }

  curl_easy_setopt(curl, CURLOPT_URL, hostname);
  // raw TCP connection only
  curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

  // connect to the server.
  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    return newCurlError(result);
  }
  *curlRet = curl;
  return (so_Error){};
}
// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
so_R_int_err CurlReadFromSocket(CURL* curl, void* buffer, size_t buflen) {
  size_t n;
  CURLcode res = curl_easy_recv(curl, buffer, buflen, &n);
  if (res == CURLE_AGAIN)
    return (so_R_int_err){.val = 0, .err = NULL};
  if (res != CURLE_OK) {
    return (so_R_int_err){.val = -1, .err = newCurlError(res)};
  }
  return (so_R_int_err){.val = n, .err = NULL};
}

// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
so_R_int_err CurlWriteToSocket(CURL* curl, const void* buffer, size_t buflen) {
  size_t n;
  CURLcode res = curl_easy_send(curl, buffer, buflen, &n);
  if (res == CURLE_AGAIN)
    return (so_R_int_err){.val = 0, .err = NULL};
  if (res != CURLE_OK) {
    return (so_R_int_err){.val = -1, .err = newCurlError(res)};
  }
  return (so_R_int_err){.val = n, .err = NULL};
}
