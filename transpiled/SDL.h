#pragma once
#include "so/builtin/builtin.h"
#include "game/game.h"
#include "gfx/gfx.h"
#include "mix/mix.h"
#include "sdl/sdl.h"
#include "so/c/c.h"
#include "so/fmt/fmt.h"
#include "so/time/time.h"

// -- Embeds --

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// -- Types --

typedef struct SDL_AppState SDL_AppState;

typedef struct SDL_AppState {
    time_Time lastTime;
    game_State game;
} SDL_AppState;

// -- Functions and methods --
SDL_AppResult SDL_AppInit(void** appState, int argc, char** argv);
SDL_AppResult SDL_AppIterate(void* appState);
SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* e);
void SDL_AppQuit(void* appState, SDL_AppResult result);
