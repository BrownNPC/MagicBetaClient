#pragma once
#include <SDL3/SDL.h>
#include <gfx/camera.h>
#include <gfx/raymath.h>
#include <gfx/rlgl.h>

// Color, 4 components, R8G8B8A8 (32bit)
typedef struct Color {
  Uint8 r;  // Color red value
  Uint8 g;  // Color green value
  Uint8 b;  // Color blue value
  Uint8 a;  // Color alpha value
} Color;

// Rectangle, 4 components
typedef struct Rectangle {
  float x;       // Rectangle top-left corner position x
  float y;       // Rectangle top-left corner position y
  float width;   // Rectangle width
  float height;  // Rectangle height
} Rectangle;

void GFX_init(SDL_Window* window);

void GFX_BeginDrawing(void);
void GFX_EndDrawing(void);

void GFX_BeginMode3D(Camera);
void GFX_EndMode3D();

// Draw a color-filled rectangle with pro parameters
void GFX_DrawRectanglePro(Rectangle rec,
                      Vector2 origin,
                      float rotation,
                      Color color);
