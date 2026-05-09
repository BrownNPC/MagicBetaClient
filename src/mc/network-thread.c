#include <core.h>
#include <mc/mc.h>
#include <net/net.h>

Uint8 mem[NETWORK_THREAD_MEM];  // memory used by network thread.
Uint32 MC_NETWORK_EVENT;

NetworkThreadData* MC_StartNetworkThread(string hostname) {
  auto em = em_create_static(mem, NETWORK_THREAD_MEM);
  auto threadData = new (NetworkThreadData);
  threadData->em = em;
  threadData->hostname = hostname;
  // Init queue
  threadData->q = new (MC_NetworkPacketQueue);
  threadData->q->mutex = SDL_CreateMutex();
  // Start the thread.
  threadData->thread = SDL_CreateThread(MC_NetworkThread, "net", threadData);
  SDL_DetachThread(threadData->thread);
  return threadData;
}

void SendSDLEvent(MC_NetworkEventCode t, void* data1, void* data2) {
  SDL_Event ev = {};
  ev.type = MC_NETWORK_EVENT;
  ev.user = (SDL_UserEvent){
      .type = MC_NETWORK_EVENT,
      .code = t,
      .data1 = data1,
      .data2 = data2,
  };
  if (!SDL_PushEvent(&ev)) {
    SDL_Log("SDL_PushEvent failed: %s", SDL_GetError());
    SDL_assert(false);
  }
}

bool MC_NetworkThreadPopPacket(NetworkThreadData* d,
                            MC_Packet** packet,
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
int MC_NetworkThread(void* userdata) {
  NetworkThreadData* d = userdata;
  auto em = d->em;
  // Connect to the server.
  auto res = NET_ConnDial(em, d->hostname);
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
    MC_Packet* packet = nullptr;
    EM* packetMemory = nullptr;
    bool toSend = false;  // whether the packet is to be free'd or to be sent.
    // write a packet or free it.
    if (MC_NetworkThreadPopPacket(d, &packet, &packetMemory, &toSend)) {
      // write the packet to socket.
      if (toSend) {
        if (MC_WritePacket(conn, packet)) {
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
    if (MC_ReadPacket(conn, packetMemory, packet)) {
      // Send the packet that is read along with memory
      SendSDLEvent(NET_PACKET_RECV, packet, packetMemory);
    } else if (NET_IsConnDisconnected(conn)) {
      res.err = str("Connection was closed.");
      return -1;
    } else {  // no packet arrived. free the memory.
      em_destroy(packetMemory);
    }
  }
}

bool MC_NetworkThreadPushPacket(NetworkThreadData* d,
                             MC_Packet* packet,
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
