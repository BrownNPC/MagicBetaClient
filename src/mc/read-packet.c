#include <SDL3/SDL_iostream.h>
#include <mc/packet.h>
#include <net/net.h>

ReadPacketPayloadFunc PacketDecoders[0x100] = {};

bool read_invalid(SDL_IOStream* s, EM* em, void* payload) {
  SDL_assert("UNREGISTERED PACKET");
  return true;  // does not have a payload.
}
bool read_keep_alive(SDL_IOStream* s, EM* em, void* payload) {
  return true;  // does not have a payload.
}
// https://pixelbrush.dev/beta-wiki/networking/packets/001-login#clientbound
bool read_login(SDL_IOStream* s, EM* em, void* payload) {
  MC_LoginPacket* p = payload;
  if (!SDL_ReadS32BE(s, &p->entityID))
    return false;
  if (!ReadString16(em, s, &p->username)) // unused.
    return false;
  if (!SDL_ReadS64BE(s, &p->seed))
    return false;
  if (!SDL_ReadS8(s, &p->dimension))
    return false;

  return true;
}

// https://pixelbrush.dev/beta-wiki/networking/packets/002-pre-login#clientbound
bool read_pre_login(SDL_IOStream* s, EM* em, void* payload) {
  MC_PreLoginPacket* p = payload;
  if (!ReadString16(em, s, &p->connectionHash))
    return false;

  return true;
}

// Set the packet decoder function pointer table.
void setPacketDecoders() {
  // Assign the decoder function pointers.
  PacketDecoders[0x00] = read_keep_alive;
  PacketDecoders[0x01] = read_login;
}
