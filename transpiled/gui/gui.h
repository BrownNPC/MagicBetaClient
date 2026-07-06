#pragma once
#include "so/builtin/builtin.h"
#include "gfx/assets/assets.h"
#include "gfx/gfx.h"
#include "so/math/math.h"
#include "so/time/time.h"

// -- Variables and constants --
extern gfx_Rectangle gui_ButtonSize;
static const int64_t gui_MaxAutoScale = 3;
extern gfx_Rectangle gui_Base;
extern gfx_TexturePack gui_ActivePack;
extern float gui_Scale;

// size of Minecraft Logo
extern gfx_Rectangle gui_MinecraftLogoSize;

// -- Functions and methods --
void gui_Button(so_String Text, gfx_Rectangle bbox, bool Hovered, bool Enabled);

// Must be called whenever screen size changes.
void gui_Update(float screenW, float screenH, gfx_TexturePack pack);
void gui_MinecraftLogo(so_String Splash, gfx_Rectangle bbox);
void gui_TextField(so_String Text, so_String placeholder, gfx_Rectangle bbox, bool Hovered, bool Enabled);
