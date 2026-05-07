#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_oldnames.h>
#include <core.h>
#include <net/net.h>

typedef enum {
  CMD_CONNECT,
  CMD_PACKET,       // when packet is sent or recieved.
  CMD_DISCONNECT,   // when asking the network thread to disconnect, or when
                    // server disconnects us.
  CMD_USED_PACKET,  // Sent by main thread. Telling us that the last packet we
                    // sent can be free'd.
  // disconnect from server.
} NetCommandType;  // Commands are bi-drectional. Except CMD_ACK

Uint32 NETWORK_EVENT;  // sdl event type. Set by app init.
constexpr auto NETWORK_THREAD_MEM = 1024 * 10;  // 10kB
constexpr auto NETWORK_THREAD_CMD_QUEUE_CAPACITY = 10;

typedef struct {
  NetCommandType Type;
  union {
    string CMD_CONNECT_HOSTNAME;  // when CMD_CONNECT
  };
} NetworkThreadCommand;

typedef struct {
  SDL_Mutex* mutex;
  SDL_Condition* cond;

  NetworkThreadCommand cmds[NETWORK_THREAD_CMD_QUEUE_CAPACITY];
  size_t head;
  size_t tail;
  size_t count;
  bool closed;
} NetworkThreadCommandQueue;

typedef struct {
  EM* em;  // memory arena.
  NetworkThreadCommandQueue* q;
} NetworkThreadData;

// Copies data to the network thread's memory. Cmd can be de-allocated after
// push is successful.
static bool NetworkThreadPushCmd(NetworkThreadData* d,
                                 const NetworkThreadCommand* cmd) {
  auto q = d->q;
  SDL_LockMutex(q->mutex);
  defer {
    SDL_UnlockMutex(q->mutex);
  }
  if (!q->closed && q->count < NETWORK_THREAD_CMD_QUEUE_CAPACITY) {
    q->cmds[q->tail] = *cmd;
    q->tail = (q->tail + 1) % NETWORK_THREAD_CMD_QUEUE_CAPACITY;
    q->count++;
    SDL_SignalCondition(q->cond);
    return true;
  }

  return false;
}

static bool NetworkThreadPopCmd(NetworkThreadData* d,
                                NetworkThreadCommand* out) {
  auto q = d->q;
  SDL_LockMutex(q->mutex);
  defer {
    SDL_UnlockMutex(q->mutex);
  }

  if (q->count == 0) {
    return false;
  }

  *out = q->cmds[q->head];
  q->head = (q->head + 1) % NETWORK_THREAD_CMD_QUEUE_CAPACITY;
  q->count--;

  return true;
}

static inline int NetworkThread(void* _data) {
  NetworkThreadData* d = _data;
  auto em = d->em;
  auto q = d->q;

  printf("Network thread started\n");
  for (;;) {
    NetworkThreadCommand cmd = {};
    if (!NetworkThreadPopCmd(d, &cmd))
      continue
  }
  return 0;
}

// Initialize the network thread.
// It will create a sub-region from the arena. for its own use.
static inline NetworkThreadCommandQueue* InitNetworkThread(EM* globalEM) {
  NETWORK_EVENT = SDL_RegisterEvents(1);
  auto em = em_create_nested(globalEM, NETWORK_THREAD_MEM);
  auto threadData = new (NetworkThreadData);  // uses em. not globalEM.
  threadData->em = em;
  auto q = new (NetworkThreadCommandQueue);
  {  // Init queue
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCondition();
    threadData->q = q;
  }
  SDL_CreateThread(NetworkThread, "net", threadData);
  return q;
}
