#pragma once
#include <SDL3/SDL_iostream.h>
#include <curl/curl.h>
#include "core.h"

bool __curl_init();
void __curl_deinit();

// Initialize everything in the net package.
static inline bool NET_init() {
  return __curl_init();
}
static inline void NET_deinit() {
  __curl_deinit();
}

// NOTE: hostname must be prefixed with "http://"
ResultDef(CurlCreateSocket, CURL*, error);
CurlCreateSocketResult NET_CurlCreateSocket(string hostname);

// Returns the number of bytes read. Can be 0. -1 means error.
// This is non-blocking. It will not fill the buffer if there is no data.
// Error means no more bytes can be read/written. and you should clean up.
int NET_CurlReadFromSocket(CURL* curl, void* buffer, size_t buflen);
// This is non-blocking.
// Returns the number of bytes written. Can be 0. -1 means error.
// Error means no more bytes can be read/written. and you should clean up.
int NET_CurlWriteToSocket(CURL* curl, const void* buffer, size_t buflen);

// Conn implements SDL_IOStreamInterface.
typedef struct {
  CURL* sock;  // tcp socket created by curl.

  // block makes the read/writes block on the SDL_IOStream
  // until there is an error, or the entire passed buffer is used.
  bool Blocking;
  SDL_IOStream* stream;  // self IO_Stream interface implementation.
} Conn;
ResultDef(ConnDial, Conn*, error);
ConnDialResult NET_ConnDial(EM* em, string hostname);
// Returns true if the connection has closed.
static inline bool NET_IsConnDisconnected(Conn* conn) {
  return SDL_GetIOStatus(conn->stream) == SDL_IO_STATUS_ERROR;
}
