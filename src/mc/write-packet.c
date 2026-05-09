#include <SDL3/SDL_iostream.h>
#include <net/net.h>
#include "mc/packet.h"

WritePacketPayloadFunc PacketEncoders[0x100] = {};
bool write_invalid(SDL_IOStream* s, void* payload) {
  SDL_assert_release("WRITING INVALID PACKET");
  return true;
}

// https://pixelbrush.dev/beta-wiki/networking/packets/001-login#serverbound
bool write_login(SDL_IOStream* s, void* payload) {
  LoginPacket* p = payload;

  if (!SDL_WriteS32BE(s, p->protocolVersion))
    return false;
  if (!WriteString16(s, p->username))
    return false;
  if (!SDL_WriteS32BE(s, 0))  // unused
    return false;
  if (!SDL_WriteU8(s, 0))  // unused
    return false;

  return true;
}
// https://pixelbrush.dev/beta-wiki/networking/packets/002-pre-login#serverbound
bool write_pre_login(SDL_IOStream* s, void* payload) {
  PreLoginPacket* p = payload;
  if (!WriteString16(s, p->username))
    return false;

  return true;
}

void setPacketEncoders() {
  PacketEncoders[0x01] = write_login;
  PacketEncoders[0x02] = write_pre_login;
}
