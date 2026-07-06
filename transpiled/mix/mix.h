#pragma once
#include "so/builtin/builtin.h"
#include <SDL3_mixer/SDL_mixer.h>
#include "sdl/sdl.h"

// -- Types --

typedef void (*mix_TrackStoppedCallback)(void*, MIX_Track*);
