#include <core.h>

#include "net/net.h"

typedef enum {
  CMD_CONNECT,
  CMD_PACKET,      // when packet is sent or recieved.
  CMD_DISCONNECT,  // when asking the network thread to disconnect, or when we
  // This command is only sent by the main thread. It means the main thread has
  // copied the command into its own memory. So we can free it.
  CMD_ACK,
  // disconnect from server.
} NetCommandType;  // Commands are bi-drectional.

Uint32 NETWORK_EVENT;  // sdl event type. Set by app init.
constexpr auto NETWORK_THREAD_MEM = 1024 * 10;  // 10kB

typedef struct {
  EM* em;  // memory arena.
  Conn* conn;
} NetworkThreadData;

static inline int NetworkThread(void* _data) {
  NetworkThreadData* d = _data;
  auto em = d->em;
  printf("Network thread started\n");
  return 0;
}

// Initialize the network thread.
// It will create a sub-region from the arena. for its own use.
static inline SDL_Thread* InitNetworkThread(EM* globalEM) {
  NETWORK_EVENT = SDL_RegisterEvents(1);
  auto em = em_create_nested(globalEM, NETWORK_THREAD_MEM);
  auto threadData = new (NetworkThreadData);  // uses em. not globalEM.
  threadData->em = em;
  return SDL_CreateThread(NetworkThread, "net", threadData);
}
