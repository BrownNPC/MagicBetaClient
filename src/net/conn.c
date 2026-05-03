#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_timer.h>
#include <curl/curl.h>
#include "core.h"
#include "easy_memory.h"
#include "net.h"

typedef struct {
  CURL* sock;  // tcp socket created by curl.
} Conn;

size_t ConnRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) {
  Conn* conn = userdata;
  size_t n = CurlReadFromSocket(conn->sock, ptr, size);
  if (n == -1) {
    *status = SDL_IO_STATUS_ERROR;
    return 0;
  }
  if (n == 0) {
    *status = SDL_IO_STATUS_NOT_READY;
    return n;
  }
  *status = SDL_IO_STATUS_READY;
  return n;
}
size_t ConnWrite(void* userdata,
                 const void* ptr,
                 size_t size,
                 SDL_IOStatus* status) {
  Conn* conn = userdata;
  auto n = CurlWriteToSocket(conn->sock, ptr, size);
  if (n == 0) {
    *status = SDL_IO_STATUS_NOT_READY;
    return 0;
  }
  if (n == -1) {
    *status = SDL_IO_STATUS_ERROR;
    return 0;
  }
  return n;
};
bool ConnClose(void* userdata) {
  Conn* conn = userdata;
  curl_easy_cleanup(conn->sock);
  delete(conn);
  return true;
}
ConnDialResult ConnDial(EM* em, string hostname) {
  Bump* scratch = em_bump_create(em, 2048);
  defer {
    em_bump_destroy(scratch);
  }
  // Allocate interface
  SDL_IOStreamInterface i;
  i.read = ConnRead;
  i.write = ConnWrite;
  i.close = ConnClose;
  // Open TCP socket using curl.
  hostname = strCat(str("http://"), hostname);
  auto curl = CurlCreateSocket(hostname);
  if (!curl.ok) {
    return Err(ConnDial, curl.err);
  }
  // return the Conn.
  auto conn = new (Conn);
  conn->sock = curl.result;
  return Ok(ConnDial, SDL_OpenIO(&i, conn));
};
