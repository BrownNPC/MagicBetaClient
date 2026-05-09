#include "mc/packet.h"
#include <SDL3/SDL_iostream.h>
#include <core.h>

void setPacketEncoders();
void setPacketDecoders();
// Sets up function pointer table for reading packets.
void _createPacketHandlerFunctionTable() {
  for (auto i = 0; i < 0x100; i++) {
    PacketDecoders[i] = read_invalid;
    PacketEncoders[i] = write_invalid;
  }
  setPacketDecoders();
  setPacketEncoders();
}
bool MC_ReadPacket(Conn* conn, EM* em, MC_Packet* p) {
  auto s = conn->stream;
  if (!SDL_ReadU8(s, &p->id))
    return false;
  auto decoderFunc = PacketDecoders[p->id];
  if (decoderFunc == read_invalid) {
    SDL_assert("READING Invalid Packet ID" && p->id);
    return false;
  }
  // block while decoding, only return on success or error.
  conn->Blocking = true;
  defer {
    conn->Blocking = false;
  }
  return decoderFunc(conn->stream, em, &p->payload);
}

bool MC_WritePacket(Conn* conn, MC_Packet* p) {
  auto s = conn->stream;
  conn->Blocking = true;
  defer {
    conn->Blocking = false;
  }
  if (!SDL_WriteU8(s, p->id))
    return false;

  auto encoderFunc = PacketEncoders[p->id];
  if (encoderFunc == write_invalid) {
    SDL_assert_release("WRITING Invalid Packet ID" && p->id);
    return false;
  }

  if (!encoderFunc(s, &p->payload))
    return false;

  return true;
}
