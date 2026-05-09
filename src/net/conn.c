#include <core.h>
#include <curl/curl.h>
#include <net/net.h>
#include <stdio.h>

constexpr auto ConnPollRate = Time_Millisecond * 10;

// Blocking mode:
//  Retry until output buffer is filled or there is an error.
static size_t NET_ConnRead(void* userdata,
                       void* ptr,
                       size_t size,
                       SDL_IOStatus* status) {
  Conn* conn = userdata;
  Uint8* out = ptr;
  size_t total = 0;

  *status = SDL_IO_STATUS_READY;

  while (total < size) {
    auto n = NET_CurlReadFromSocket(conn->sock, out + total, size - total);

    if (n < 0) {
      *status = SDL_IO_STATUS_ERROR;
      return total;
    }

    if (n == 0) {
      if (conn->Blocking) {
        Time_Sleep(ConnPollRate);
        continue;  // retry
      }

      if (total == 0)  // there is no data.
        *status = SDL_IO_STATUS_NOT_READY;
      return total;
    }

    total += n;

    if (!conn->Blocking)
      return total;
  }

  return total;
}
// Blocking mode:
//  Block untill all bytes are written or there is an error.
static size_t NET_ConnWrite(void* userdata,
                        const void* ptr,
                        size_t size,
                        SDL_IOStatus* status) {
  Conn* conn = userdata;
  const Uint8* in = ptr;
  size_t total = 0;

  *status = SDL_IO_STATUS_READY;

  while (total < size) {
    auto n = NET_CurlWriteToSocket(conn->sock, in + total, size - total);

    if (n < 0) {
      *status = SDL_IO_STATUS_ERROR;
      return total;
    }

    if (n == 0) {
      if (conn->Blocking) {
        Time_Sleep(ConnPollRate);
        continue;
      }

      *status = SDL_IO_STATUS_NOT_READY;
      return total;
    }

    total += n;

    if (!conn->Blocking)
      return total;
  }

  return total;
}

static bool NET_ConnClose(void* userdata) {
  Time_Sleep(Time_Second); // wait for all data to be sent.
  Conn* conn = userdata;
  curl_easy_cleanup(conn->sock);
  delete (conn);
  return true;
}

ConnDialResult NET_ConnDial(EM* em, string hostname) {
  EM* scratch = em_create_nested(em, 2048);
  defer {
    em_destroy(scratch);
  }
  // Allocate interface
  SDL_IOStreamInterface i;
  SDL_INIT_INTERFACE(&i);
  i.read = NET_ConnRead;
  i.write = NET_ConnWrite;
  i.close = NET_ConnClose;
  // Open TCP socket using curl.
  hostname = strCat(scratch, str("http://"), hostname);
  auto curl = NET_CurlCreateSocket(hostname);
  if (!curl.ok) {
    return Err(ConnDial, curl.err);
  }
  // return the Conn.
  auto conn = new (Conn);
  conn->sock = curl.result;
  conn->stream = SDL_OpenIO(&i, conn);
  return Ok(ConnDial, conn);
};
