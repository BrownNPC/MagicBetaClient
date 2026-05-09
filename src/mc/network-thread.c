#include <SDL3/SDL_events.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_thread.h>
#include <core.h>
#include <mc/mc.h>
#include "easy_memory.h"
#include "mc/packet.h"
#include "net/net.h"

Uint8 mem[NETWORK_THREAD_MEM];  // memory used by network thread.
Uint32 NETWORK_EVENT;

NetworkThreadData* InitNetworkThread(string hostname) {
  auto em = em_create_static(mem, NETWORK_THREAD_MEM);
  auto threadData = new (NetworkThreadData);
  threadData->em = em;
  threadData->hostname = hostname;
  // Init queue
  threadData->q = new (NetworkPacketQueue);
  threadData->q->mutex = SDL_CreateMutex();
  // Start the thread.
  threadData->thread = SDL_CreateThread(NetworkThread, "net", threadData);
  SDL_DetachThread(threadData->thread);
  return threadData;
}

void SendSDLEvent(NetworkEventCode t, void* data1, void* data2) {
  SDL_Event ev = {};
  ev.type = NETWORK_EVENT;
  ev.user = (SDL_UserEvent){
      .type = NETWORK_EVENT,
      .code = t,
      .data1 = data1,
      .data2 = data2,
  };
  if (!SDL_PushEvent(&ev)) {
    SDL_Log("SDL_PushEvent failed: %s", SDL_GetError());
    SDL_assert(false);
  }
}

bool NetworkThreadPopPacket(NetworkThreadData* d,
                            Packet** packet,
                            EM** packetMemory,
                            bool* toSend) {
  auto q = d->q;
  SDL_LockMutex(q->mutex);
  defer {
    SDL_UnlockMutex(q->mutex);
  }

  if (q->count == 0) {
    return false;
  }

  *packet = q->packets[q->head];
  *packetMemory = q->packetArenas[q->head];
  *toSend = q->toSend[q->head];
  q->head = (q->head + 1) % NETWORK_PACKET_QUEUE_CAPACITY;
  q->count--;

  return true;
}
// Network thread code.
int NetworkThread(void* userdata) {
  NetworkThreadData* d = userdata;
  auto em = d->em;
  // Connect to the server.
  auto res = ConnDial(em, d->hostname);
  defer {
    SendSDLEvent(NET_CLOSED, res.err.items, nullptr);
  }
  Conn* conn = nullptr;

  if (res.ok) {
    SendSDLEvent(NET_CONNECTED, nullptr, nullptr);
    conn = res.result;
  } else {
    return -1;
  }

  // Connected.
  for (;;) {
    Packet* packet = nullptr;
    EM* packetMemory = nullptr;
    bool toSend = false;  // whether the packet is to be free'd or to be sent.
    // write a packet or free it.
    if (NetworkThreadPopPacket(d, &packet, &packetMemory, &toSend)) {
      // write the packet to socket.
      if (toSend) {
        if (WritePacket(conn, packet)) {
          // Send the packet as an event. so the main thread can free it.
          SendSDLEvent(NET_PACKET_SENT, packet, packetMemory);
        } else {
          // there was an error and connection has closed.
          SDL_CloseIO(conn->stream);
          res.err = str("Connection was closed.");
          return -1;
        }
      } else {  // to be free'd
        em_destroy(packetMemory);
      }
      continue;
    }
    // read a packet.
    packetMemory = em_create_nested(em, 1024 * 10);  // 10kb
    if (ReadPacket(conn, packetMemory, packet)) {
      // Send the packet that is read along with memory
      SendSDLEvent(NET_PACKET_RECV, packet, packetMemory);
    } else if (IsConnDisconnected(conn)) {
      res.err = str("Connection was closed.");
      return -1;
    } else {  // no packet arrived. free the memory.
      em_destroy(packetMemory);
    }
  }
}

bool NetworkThreadPushPacket(NetworkThreadData* d,
                             Packet* packet,
                             EM* packetMemory,
                             bool toSend) {
  auto q = d->q;
  SDL_LockMutex(q->mutex);
  defer {
    SDL_UnlockMutex(q->mutex);
  }
  if (!q->closed && q->count < NETWORK_PACKET_QUEUE_CAPACITY) {
    q->packets[q->tail] = packet;
    q->packetArenas[q->tail] = packetMemory;
    q->packetArenas[q->tail] = packetMemory;
    q->toSend[q->tail] = toSend;
    q->tail = (q->tail + 1) % NETWORK_PACKET_QUEUE_CAPACITY;
    q->count++;
    return true;
  }

  return false;
};
