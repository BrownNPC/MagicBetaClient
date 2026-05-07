#pragma once
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_iostream.h>
#include <assert.h>
#include <core.h>
#include <net/net.h>

typedef enum : Uint8 {
  PKT_KeepAlive = 0x00,
  PKT_Login = 0x01,
  PKT_PreLogin = 0x02,
} PacketID;

// string16 is an alias for a regular string.
// But it is handled differently in the protocol de/serialization layer.
//
// UCS-2 string, big-endian. Prefixed by an unsigned short containing the length
// of the string in characters. UCS-2 consists of 16-bit words, each of which
// represent a Unicode code point between U+0000 and U+FFFF inclusive
ArrayDecl(Uint16, string16);

// https://pixelbrush.dev/beta-wiki/networking/packets/001-login
typedef struct {
  union {
    Sint32 entityID, protocolVersion;
  };
  string16 username;
  Sint64 seed;
  Sint8 dimension;
} LoginPacket;  // 0x01

// https://pixelbrush.dev/beta-wiki/networking/packets/002-pre-login
typedef struct {
  union {
    string16 username, connectionHash;
  };
} PreLoginPacket;  // 0x02

typedef struct {
  PacketID id;
  union {
    LoginPacket loginRequest;
    PreLoginPacket preLogin;
  } payload;
} Packet;

// write string8
static inline bool WriteString(SDL_IOStream* dst, string s) {
  if (!SDL_WriteU16BE(dst, s.len))
    return false;
  for (auto i = 0; i < s.len; i++) {
    if (!SDL_WriteU8(dst, s.items[i]))
      return false;
  }
  return true;
};
// ToString16 creates a string16 from a string.
// The string 16 is not null terminated.
static inline string16 ToString16(EM* em, string s) {
  auto len = SDL_utf8strnlen(s.items, s.len);

  string16 out;
  make(em, out, len);

  Uint16* tmpUcs2 = SDL_iconv_utf8_ucs2(s.items);
  defer {
    SDL_free(tmpUcs2);
  }
  for (auto i = 0; i < len; i++) {
    auto u = tmpUcs2[i];
    append(out, u);
  }

  return out;
};
// create a null terminated string from string16
static inline string FromString16(EM* em, string16 s) {
  auto v = SDL_iconv_string("UTF-8", "UCS-2", (char*)s.items,
                            SDL_strlen((char*)s.items));
  defer {
    SDL_free(v);
  }
  // allocate a new null-terminated string on the arena.
  return strCat(em, str(""), str(v));
}

// string16 is an alias for a regular string.
// But it is handled differently in the protocol de/serialization layer.
//
// UCS-2 string, big-endian. Prefixed by an unsigned short containing the length
// of the string in characters. UCS-2 consists of 16-bit words, each of which
// represent a Unicode code point between U+0000 and U+FFFF inclusive
static inline bool WriteString16(SDL_IOStream* dst, string16 s) {
  if (!SDL_WriteU16BE(dst, s.len))  // size of string
    return false;

  for (auto i = 0; i < s.len; i++) {
    auto u = s.items[i];
    if (!SDL_WriteU16BE(dst, u))
      return false;
  }
  return true;
}

// dst should be un-initialized.
static inline bool ReadString16(EM* em, SDL_IOStream* src, string16* dst) {
  Uint8 size;
  if (!SDL_ReadU8(src, &size))
    return false;

  string16 out;
  make(em, out, size);

  for (auto i = 0; i < size; i++) {
    Uint16 codePoint;
    if (!SDL_ReadU16BE(src, &codePoint))
      return false;
    append(out, codePoint);
  }
  *dst = out;
  return true;
}

typedef bool (*ReadPacketPayloadFunc)(Conn* conn, EM* em, void* payload);

// Sets up function pointer table.
void InitPacketDecoders();
ReadPacketPayloadFunc PacketDecoders[0x100];

// a function that panics when used.
bool read_invalid(Conn* conn, EM* em, void* payload);

bool ReadPacket(Conn* conn, EM* em, Packet* p) {
  auto s = conn->stream;
  if (!SDL_ReadU8(s, &p->id))
    return false;
  conn->Blocking = true;
  defer {
    conn->Blocking = false;
  }
  auto decoderFunc = PacketDecoders[p->id];
  if (decoderFunc == read_invalid) {
    SDL_assert_release("INVALID Packet ID");
    return false;
  }
  return decoderFunc(conn, em, &p->payload);
}
