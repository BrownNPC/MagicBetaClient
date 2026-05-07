#include <SDL3/SDL_iostream.h>
#include <core.h>
#include <mc/packet.h>
#include "net/net.h"

bool read_invalid(Conn* conn, EM* em, void* payload) {
  SDL_assert_release("UNREGISTERED PACKET");
  return true;  // does not have a payload.
}
bool read_keep_alive(Conn* conn, EM* em, void* payload) {
  return true;  // does not have a payload.
}
// https://pixelbrush.dev/beta-wiki/networking/packets/001-login#clientbound
bool read_login(Conn* conn, EM* em, void* payload) {
  LoginPacket* p = payload;
  auto s = conn->stream;
  if (!SDL_ReadS32BE(s, &p->entityID))
    return false;
  if (!ReadString16(em, s, &p->username))
    return false;
  if (!SDL_ReadS64BE(s, &p->seed))
    return false;
  if (!SDL_ReadS8(s, &p->dimension))
    return false;
  return true;
}

// https://pixelbrush.dev/beta-wiki/networking/packets/002-pre-login#clientbound
bool read_pre_login(Conn* conn, EM* em, void* payload) {
  PreLoginPacket* p = payload;
  auto s = conn->stream;
  if (!ReadString16(em, s, &p->connectionHash))
    return false;
  return true;
}
// Sets up function pointer table for reading packets.
void InitPacketDecoders() {
  for (auto i = 0; i < 0x100; i++)
    PacketDecoders[i] = read_invalid;
  // Assign the decoder function pointers.
  PacketDecoders[0x00] = read_keep_alive;
  PacketDecoders[0x01] = read_login;
}
