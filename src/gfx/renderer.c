#include <gfx/render.h>
// these are copied from raylib :)

SDL_Window* GFX_window;
// Set viewport for a provided width and height
void SetupViewport(int width, int height) {
  rlViewport(0, 0, width, height);

  rlMatrixMode(RL_PROJECTION);
  rlLoadIdentity();
  rlOrtho(0, width, height, 0, 0.0f, 1.0f);

  rlMatrixMode(RL_MODELVIEW);
  rlLoadIdentity();
}
void GFX_init(SDL_Window* window) {
  GFX_window = window;
  SDL_GL_CreateContext(window);
  int width, height = {};
  SDL_GetWindowSizeInPixels(window, &width, &height);
  SetupViewport(width, height);
}

void GFX_BeginDrawing() {
  rlLoadIdentity();  // Reset current matrix (modelview)
}

void GFX_EndDrawing() {
  rlDrawRenderBatchActive();  // Update and draw internal render batch
  SDL_GL_SwapWindow(GFX_window);
}
// Initializes 3D mode with custom camera (3D)
void GFX_BeginMode3D(Camera camera) {
  int width, height = {};
  SDL_GetWindowSize(GFX_window, &width, &height);

  rlDrawRenderBatchActive();  // Update and draw internal render batch

  rlMatrixMode(RL_PROJECTION);  // Switch to projection matrix
  rlPushMatrix();  // Save previous matrix, which contains the settings for the
                   // 2d ortho projection
  rlLoadIdentity();  // Reset current matrix (projection)

  float aspect = (float)width / (float)height;

  // NOTE: zNear and zFar values are important when computing depth buffer
  // values
  if (camera.projection == CAMERA_PERSPECTIVE) {
    // Setup perspective projection
    double top = rlGetCullDistanceNear() * tan(camera.fovy * 0.5 * DEG2RAD);
    double right = top * aspect;

    rlFrustum(-right, right, -top, top, rlGetCullDistanceNear(),
              rlGetCullDistanceFar());
  } else if (camera.projection == CAMERA_ORTHOGRAPHIC) {
    // Setup orthographic projection
    double top = camera.fovy / 2.0;
    double right = top * aspect;

    rlOrtho(-right, right, -top, top, rlGetCullDistanceNear(),
            rlGetCullDistanceFar());
  }

  rlMatrixMode(RL_MODELVIEW);  // Switch back to modelview matrix
  rlLoadIdentity();            // Reset current matrix (modelview)

  // Setup Camera view
  Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
  rlMultMatrixf(MatrixToFloat(
      matView));  // Multiply modelview matrix by view matrix (camera)

  rlEnableDepthTest();  // Enable DEPTH_TEST for 3D
}

// End 3D mode and returns to default 2D orthographic mode
void GFX_EndMode3D(void) {
  rlDrawRenderBatchActive();  // Update and draw internal render batch

  rlMatrixMode(RL_PROJECTION);  // Switch to projection matrix
  rlPopMatrix();  // Restore previous matrix (projection) from matrix stack

  rlMatrixMode(RL_MODELVIEW);  // Switch back to modelview matrix
  rlLoadIdentity();            // Reset current matrix (modelview)

  rlDisableDepthTest();  // Disable DEPTH_TEST for 2D
}

// Draw a color-filled rectangle with pro parameters
void GFX_DrawRectanglePro(Rectangle rec,
                          Vector2 origin,
                          float rotation,
                          Color color) {
  Vector2 topLeft = {0};
  Vector2 topRight = {0};
  Vector2 bottomLeft = {0};
  Vector2 bottomRight = {0};

  // Only calculate rotation if needed
  if (rotation == 0.0f) {
    float x = rec.x - origin.x;
    float y = rec.y - origin.y;
    topLeft = (Vector2){x, y};
    topRight = (Vector2){x + rec.width, y};
    bottomLeft = (Vector2){x, y + rec.height};
    bottomRight = (Vector2){x + rec.width, y + rec.height};
  } else {
    float sinRotation = sinf(rotation * DEG2RAD);
    float cosRotation = cosf(rotation * DEG2RAD);
    float x = rec.x;
    float y = rec.y;
    float dx = -origin.x;
    float dy = -origin.y;

    topLeft.x = x + dx * cosRotation - dy * sinRotation;
    topLeft.y = y + dx * sinRotation + dy * cosRotation;

    topRight.x = x + (dx + rec.width) * cosRotation - dy * sinRotation;
    topRight.y = y + (dx + rec.width) * sinRotation + dy * cosRotation;

    bottomLeft.x = x + dx * cosRotation - (dy + rec.height) * sinRotation;
    bottomLeft.y = y + dx * sinRotation + (dy + rec.height) * cosRotation;

    bottomRight.x =
        x + (dx + rec.width) * cosRotation - (dy + rec.height) * sinRotation;
    bottomRight.y =
        y + (dx + rec.width) * sinRotation + (dy + rec.height) * cosRotation;
  }
  rlBegin(RL_TRIANGLES);

  rlColor4ub(color.r, color.g, color.b, color.a);

  rlVertex2f(topLeft.x, topLeft.y);
  rlVertex2f(bottomLeft.x, bottomLeft.y);
  rlVertex2f(topRight.x, topRight.y);

  rlVertex2f(topRight.x, topRight.y);
  rlVertex2f(bottomLeft.x, bottomLeft.y);
  rlVertex2f(bottomRight.x, bottomRight.y);

  rlEnd();
}
