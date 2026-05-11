#pragma once
#include <core.h>
#include <gfx/gfx.h>
typedef struct GAME_System GAME_System;
ArrayDecl(GAME_System, GAME_Systems);

typedef struct {
  EM* em;
  SDL_Window* window;
  GAME_Systems systems;
  Camera camera;
} GAME_State;

typedef void (*GAME_updatefunc)(void* self, GAME_State* s);

// Game system interface.
struct GAME_System {
  void* self;
  GAME_updatefunc update;
};

// Initialize the entire game.
GAME_State* GAME_Init(EM* em, SDL_Window* window);
