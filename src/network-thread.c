
#include <stdio.h>

static inline int NetworkThread(void* data) {
  printf("Network thread started\n");
  return 0;
}
