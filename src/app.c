#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <core.h>
#include <game/game.h>
#include <gfx/gfx.h>
#include <mc/mc.h>
#include "easy_memory.h"
#include "game/state.h"
#include "gfx/render.h"
#include "gfx/rlgl.h"
#include "net/net.h"

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

// 20 MB memory.
constexpr auto MEM_SIZE = 20 * 1024 * 1024;
Uint8 memory[MEM_SIZE];

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  auto window = SDL_CreateWindow("MagicBetaClient", 640, 480,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  GFX_init(window);
  *appstate = GAME_Init(em_create_static(memory, MEM_SIZE), window);

  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
  }
  if (event->type == MC_NETWORK_EVENT) {
    printf("NETWORK_EVENT_CODE=%d", event->user.code);
  }
  return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {
  GAME_State* s = appstate;

  GFX_BeginDrawing();
  GFX_DrawRectanglePro((Rectangle){0, 0, 480, 272}, (Vector2){}, 0,
                       (Color){.r = 255, .a = 255});
  GFX_EndDrawing();

  return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  NET_deinit();
  rlglClose();
  /* SDL will clean up the window/renderer for us. */
}
