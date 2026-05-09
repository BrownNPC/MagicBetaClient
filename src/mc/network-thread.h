#pragma once
#include <SDL3/SDL_thread.h>
#include <core.h>
#include <mc/packet.h>

typedef enum {
  NET_CONNECTED,  // Sent when TCP socket is connected. data1,data2 = nullptr
  NET_CLOSED,     // Send when connection is closed. data1: null terminated C
                  // string (reason)

  NET_PACKET_SENT,  // the packet was sent, the main thread can free it. data1
                    // is packet, data2 is packet memory.
  NET_PACKET_RECV,  // new packet arrived: data 1 is packet, data 2 is packet
                    // memory.
} NetworkEventCode;

constexpr auto NETWORK_PACKET_QUEUE_CAPACITY = 10;
// Network thread uses this much memory.
constexpr auto NETWORK_THREAD_MEM = 1024 * 300;

typedef struct {
  SDL_Mutex* mutex;

  Packet* packets[NETWORK_PACKET_QUEUE_CAPACITY];
  EM* packetArenas[NETWORK_PACKET_QUEUE_CAPACITY];
  // should the network thread free this packet or send it?
  bool toSend[NETWORK_PACKET_QUEUE_CAPACITY];
  size_t head;
  size_t tail;
  size_t count;
  bool closed;
} NetworkPacketQueue;

typedef struct {
  EM* em;  // memory arena.
  NetworkPacketQueue* q;
  SDL_Thread* thread;
  string hostname;    // hostname to connect to.
} NetworkThreadData;  // Userdata passed to NetworkThread.

extern Uint32 NETWORK_EVENT;  // sdl event type. Set by app init.

// must be called when doing app init.
static inline void initNetworkThreadEvents() {
  NETWORK_EVENT = SDL_RegisterEvents(1);
}

// network thread function pointer.
extern int NetworkThread(void* userdata);

// Start the network thread.
// connects to the hostname.
// An event is sent if successfully connected to socket, or disconnected.
NetworkThreadData* InitNetworkThread(string hostname);

// Push a packet to the network thread.
// This can be used to send packets, and free packets that are read.
bool NetworkThreadPushPacket(NetworkThreadData* d,
                             Packet* packet,
                             EM* packetMemory,
                             bool toSend);
