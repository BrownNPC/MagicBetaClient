#pragma once
#include <mc/packet.h>
#include <mc/network-thread.h>


// Initialize everything in the mc packge.
static inline void MC_init(){
  _registerNetworkThreadEvent();
  _createPacketHandlerFunctionTable();
}
