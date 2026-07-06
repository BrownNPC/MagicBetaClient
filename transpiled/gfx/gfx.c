#include "gfx.h"

// -- Variables and constants --
double gfx_CameraCullDistanceNear = 0.05;
double gfx_CameraCullDistanceFar = 4000.0;

// Java edition chat colors
gfx_Color gfx_Black = (gfx_Color){0, 0, 0, 255};
gfx_Color gfx_DarkBlue = (gfx_Color){0, 0, 170, 255};
gfx_Color gfx_DarkGreen = (gfx_Color){0, 170, 0, 255};
gfx_Color gfx_DarkAqua = (gfx_Color){0, 170, 170, 255};
gfx_Color gfx_DarkRed = (gfx_Color){170, 0, 0, 255};
gfx_Color gfx_DarkPurple = (gfx_Color){170, 0, 170, 255};
gfx_Color gfx_Gold = (gfx_Color){255, 170, 0, 255};
gfx_Color gfx_Gray = (gfx_Color){170, 170, 170, 255};
gfx_Color gfx_DarkGray = (gfx_Color){85, 85, 85, 255};
gfx_Color gfx_Blue = (gfx_Color){85, 85, 255, 255};
gfx_Color gfx_Green = (gfx_Color){85, 255, 85, 255};
gfx_Color gfx_Aqua = (gfx_Color){85, 255, 255, 255};
gfx_Color gfx_Red = (gfx_Color){255, 85, 85, 255};
gfx_Color gfx_LightPurple = (gfx_Color){255, 85, 255, 255};
gfx_Color gfx_Yellow = (gfx_Color){255, 255, 0, 255};
gfx_Color gfx_White = (gfx_Color){255, 255, 255, 255};
SDL_Window* gfx_Window = NULL;
so_String gfx_AssetsPath = so_str("");

// gl.GenTextures(1, &t.ID)
// gl.BindTexture(gl.TEXTURE_2D, t.ID)
// gl.PixelStorei(gl.UNPACK_ALIGNMENT, 1)
// gl.TexImage2D(
// 	gl.TEXTURE_2D,
// 	0,
// 	gl.RGBA,
// 	int32(t.Width),
// 	int32(t.Height),
// 	0,
// 	gl.RGBA,
// 	gl.UNSIGNED_BYTE,
// 	img.Surface.Pixels(),
// )
// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
// gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
so_int gfx_TexturesLoaded = 0;

// Approximately the GPU memory used by textures in bytes.
so_int gfx_TextureMemoryUsed = 0;
static gfx_Texture texShapes = (gfx_Texture){.ID = 1, .Width = 1, .Height = 1};

// so:extern RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
static const int64_t rlPIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 7;
static const double epsilon = 1e-6;

// font glyphs per row in the atlas
static const int64_t glyphsPerRow = 16;

// -- camera.go --

// GetForward - Returns the cameras forward vector (normalized)
gfx_Vector3 gfx_Camera_GetForward(void* self) {
    gfx_Camera* camera = self;
    return gfx_Vector3Normalize(gfx_Vector3Subtract(camera->Target, camera->Position));
}

// GetUp - Returns the cameras up vector (normalized)
// Note: The up vector might not be perpendicular to the forward vector
gfx_Vector3 gfx_Camera_GetUp(void* self) {
    gfx_Camera* camera = self;
    return gfx_Vector3Normalize(camera->Up);
}

// GetRight - Returns the cameras right vector (normalized)
gfx_Vector3 gfx_Camera_GetRight(void* self) {
    gfx_Camera* camera = self;
    gfx_Vector3 forward = gfx_Camera_GetForward(camera);
    gfx_Vector3 up = gfx_Camera_GetUp(camera);
    return gfx_Vector3CrossProduct(forward, up);
}

// MoveForward - Moves the camera in its forward direction
void gfx_Camera_MoveForward(void* self, float distance, bool moveInWorldPlane) {
    gfx_Camera* camera = self;
    gfx_Vector3 forward = gfx_Camera_GetForward(camera);
    if (moveInWorldPlane) {
        // Project vector onto world plane
        forward.Y = (float)(0);
        forward = gfx_Vector3Normalize(forward);
    }
    // Scale by distance
    forward = gfx_Vector3Scale(forward, distance);
    // Move position and target
    camera->Position = gfx_Vector3Add(camera->Position, forward);
    camera->Target = gfx_Vector3Add(camera->Target, forward);
}

// MoveUp - Moves the camera in its up direction
void gfx_Camera_MoveUp(void* self, float distance) {
    gfx_Camera* camera = self;
    gfx_Vector3 up = gfx_Camera_GetUp(camera);
    // Scale by distance
    up = gfx_Vector3Scale(up, distance);
    // Move position and target
    camera->Position = gfx_Vector3Add(camera->Position, up);
    camera->Target = gfx_Vector3Add(camera->Target, up);
}

// MoveRight - Moves the camera target in its current right direction
void gfx_Camera_MoveRight(void* self, float distance, bool moveInWorldPlane) {
    gfx_Camera* camera = self;
    gfx_Vector3 right = gfx_Camera_GetRight(camera);
    if (moveInWorldPlane) {
        // Project vector onto world plane
        right.Y = (float)(0);
        right = gfx_Vector3Normalize(right);
    }
    // Scale by distance
    right = gfx_Vector3Scale(right, distance);
    // Move position and target
    camera->Position = gfx_Vector3Add(camera->Position, right);
    camera->Target = gfx_Vector3Add(camera->Target, right);
}

// MoveToTarget - Moves the camera position closer/farther to/from the camera target
void gfx_Camera_MoveToTarget(void* self, float delta) {
    gfx_Camera* camera = self;
    float distance = gfx_Vector3Distance(camera->Position, camera->Target);
    // Apply delta
    distance = distance + delta;
    // Distance must be greater than 0
    if (distance <= (float)(0)) {
        distance = 0.001;
    }
    // Set new distance by moving the position along the forward vector
    gfx_Vector3 forward = gfx_Camera_GetForward(camera);
    camera->Position = gfx_Vector3Add(camera->Target, gfx_Vector3Scale(forward, -distance));
}

// Yaw - Rotates the camera around its up vector
// Yaw is "looking left and right"
// If rotateAroundTarget is false, the camera rotates around its position
// Note: angle must be provided in radians
void gfx_Camera_Yaw(void* self, float angle, bool rotateAroundTarget) {
    gfx_Camera* camera = self;
    // Rotation axis
    gfx_Vector3 up = gfx_Camera_GetUp(camera);
    // View vector
    gfx_Vector3 targetPosition = gfx_Vector3Subtract(camera->Target, camera->Position);
    // Rotate view vector around up axis
    targetPosition = gfx_Vector3RotateByAxisAngle(targetPosition, up, angle);
    if (rotateAroundTarget) {
        // Move position relative to target
        camera->Position = gfx_Vector3Subtract(camera->Target, targetPosition);
    } else {
        // Move target relative to position
        camera->Target = gfx_Vector3Add(camera->Position, targetPosition);
    }
}

// Pitch - Rotates the camera around its right vector, pitch is "looking up and down"
//   - lockView prevents camera overrotation (aka "somersaults")
//   - rotateAroundTarget defines if rotation is around target or around its position
//   - rotateUp rotates the up direction as well (typically only useful in CAMERA_FREE)
//
// NOTE: angle must be provided in radians
void gfx_Camera_Pitch(void* self, float angle, bool lockView, bool rotateAroundTarget, bool rotateUp) {
    gfx_Camera* camera = self;
    // Up direction
    gfx_Vector3 up = gfx_Camera_GetUp(camera);
    // View vector
    gfx_Vector3 targetPosition = gfx_Vector3Subtract(camera->Target, camera->Position);
    if (lockView) {
        // In these camera modes we clamp the Pitch angle
        // to allow only viewing straight up or down.
        // Clamp view up
        float maxAngleUp = gfx_Vector3Angle(up, targetPosition);
        // avoid numerical errors
        maxAngleUp = maxAngleUp - 0.001;
        if (angle > maxAngleUp) {
            angle = maxAngleUp;
        }
        // Clamp view down
        float maxAngleDown = gfx_Vector3Angle(gfx_Vector3Negate(up), targetPosition);
        // downwards angle is negative
        maxAngleDown = maxAngleDown * -1.0;
        // avoid numerical errors
        maxAngleDown = maxAngleDown + 0.001;
        if (angle < maxAngleDown) {
            angle = maxAngleDown;
        }
    }
    // Rotation axis
    gfx_Vector3 right = gfx_Camera_GetRight(camera);
    // Rotate view vector around right axis
    targetPosition = gfx_Vector3RotateByAxisAngle(targetPosition, right, angle);
    if (rotateAroundTarget) {
        // Move position relative to target
        camera->Position = gfx_Vector3Subtract(camera->Target, targetPosition);
    } else {
        // Move target relative to position
        camera->Target = gfx_Vector3Add(camera->Position, targetPosition);
    }
    if (rotateUp) {
        // Rotate up direction around right axis
        camera->Up = gfx_Vector3RotateByAxisAngle(camera->Up, right, angle);
    }
}

// Roll - Rotates the camera around its forward vector
// Roll is "turning your head sideways to the left or right"
// Note: angle must be provided in radians
void gfx_Camera_Roll(void* self, float angle) {
    gfx_Camera* camera = self;
    // Rotation axis
    gfx_Vector3 forward = gfx_Camera_GetForward(camera);
    // Rotate up direction around forward axis
    camera->Up = gfx_Vector3RotateByAxisAngle(camera->Up, forward, angle);
}

// ViewMatrix - Returns the camera view matrix
gfx_Matrix gfx_Camera_ViewMatrix(void* self) {
    gfx_Camera* camera = self;
    return gfx_MatrixLookAt(camera->Position, camera->Target, camera->Up);
}

// ProjectionMatrix - Returns the camera projection matrix
gfx_Matrix gfx_Camera_ProjectionMatrix(void* self, float aspect) {
    gfx_Camera* camera = self;
    return gfx_MatrixPerspective(camera->Fovy * (gfx_Pi / 180.0), aspect, 0.01, 1000.0);
}

// Update - Update camera movement, movement/rotation values should be provided by user
// Required values
// movement.X - Move forward/backward
// movement.Y - Move right/left
// movement.Z - Move up/down
// rotation.X - yaw
// rotation.Y - pitch
// rotation.Z - roll
// zoom - Move towards target
void gfx_Camera_Update(void* self, gfx_Vector3 movement, gfx_Vector3 rotation, float zoom) {
    gfx_Camera* camera = self;
    bool lockView = true;
    bool rotateAroundTarget = false;
    bool rotateUp = false;
    bool moveInWorldPlane = true;
    // Camera rotation
    gfx_Camera_Pitch(camera, -rotation.Y * (gfx_Pi / 180.0), lockView, rotateAroundTarget, rotateUp);
    gfx_Camera_Yaw(camera, -rotation.X * (gfx_Pi / 180.0), rotateAroundTarget);
    gfx_Camera_Roll(camera, rotation.Z * (gfx_Pi / 180.0));
    // Camera movement
    gfx_Camera_MoveForward(camera, movement.X, moveInWorldPlane);
    gfx_Camera_MoveRight(camera, movement.Y, moveInWorldPlane);
    gfx_Camera_MoveUp(camera, movement.Z);
    // Zoom target distance
    gfx_Camera_MoveToTarget(camera, zoom);
}

gfx_Matrix gfx_GetCameraMatrix2D(gfx_Camera2D cam) {
    // The camera in world-space is set by
    //   1. Move it to target
    //   2. Rotate by -rotation and scale by (1/zoom)
    //      When setting higher scale, it's more intuitive for the world to become bigger (= camera become smaller),
    //      not for the camera getting bigger, hence the invert. Same deal with rotation
    //   3. Move it by (-offset);
    //      Offset defines target transform relative to screen, but since effectively "moving" screen (camera)
    //      it needs to be moved into opposite direction (inverse transform)
    // Having camera transform in world-space, inverse of it gives the modelview transform
    // Since (A*B*C)' = C'*B'*A', the modelview is
    //   1. Move to offset
    //   2. Rotate and Scale
    //   3. Move by -target
    gfx_Matrix matOrigin = gfx_MatrixTranslate(-cam.Target.X, -cam.Target.Y, 0);
    gfx_Matrix matRotation = gfx_MatrixRotate((gfx_Vector3){0.0, 0.0, 1.0}, cam.Rotation * gfx_Deg2rad);
    gfx_Matrix matScale = gfx_MatrixScale(cam.Zoom, cam.Zoom, 1.0);
    gfx_Matrix matTranslation = gfx_MatrixTranslate(cam.Offset.X, cam.Offset.Y, 0.0);
    gfx_Matrix matTransform = gfx_MatrixMultiply(gfx_MatrixMultiply(matOrigin, gfx_MatrixMultiply(matScale, matRotation)), matTranslation);
    return matTransform;
}

// -- gfx.go --

// Set viewport for a provided width and height
void gfx_SetupViewport(so_int width, so_int height) {
    // gl.Viewport(0, 0, int32(width), int32(height))
    rlViewport(0, 0, (int32_t)(width), (int32_t)(height));
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, (double)(width), (double)(height), 0, 0.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

void gfx_EnableTexture(gfx_Texture t) {
    rlSetTexture((so_int)(t.ID));
}

void gfx_DisableTexture(void) {
    rlSetTexture(0);
}

void gfx_Init(SDL_Window* win) {
    gfx_Window = win;
    SDL_GL_CreateContext(win);
    so_R_int_int _res1 = gfx_GetWindowSize();
    so_int width = _res1.val;
    so_int height = _res1.val2;
    rlLoadExtensions(SDL_GL_GetProcAddress);
    rlglInit(width, height);
    // initGLDefaultState()
    gfx_SetupViewport(width, height);
    if (so_string_eq(sdl_GetPlatform(), so_str("Android"))) {
        gfx_AssetsPath = so_str("./assets");
    } else {
        gfx_AssetsPath = so_str("./assets");
    }
}

so_R_int_int gfx_GetWindowSize(void) {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(gfx_Window, &w, &h);
    return (so_R_int_int){.val = (so_int)(w), .val2 = (so_int)(h)};
}

void gfx_BeginDrawing(void) {
    rlLoadIdentity();
}

void gfx_EndDrawing(void) {
    rlDrawRenderBatchActive();
    SDL_GL_SwapWindow(gfx_Window);
}

void gfx_ClearBackground(gfx_Color c) {
    rlClearColor((float)(c.R) / 255, (float)(c.G) / 255, (float)(c.B) / 255, (float)(c.A) / 255);
    rlClearScreenBuffers();
}

void gfx_BeginMode3D(gfx_Camera cam) {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(gfx_Window, &w, &h);
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    float aspect = (float)(w) / (float)(h);
    double top = gfx_CameraCullDistanceNear * math_Tan((double)(cam.Fovy * 0.5 * gfx_Deg2rad));
    double right = top * (double)(aspect);
    // perspective projection
    rlFrustum(-right, right, -top, top, gfx_CameraCullDistanceNear, gfx_CameraCullDistanceFar);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    gfx_Matrix matView = gfx_MatrixLookAt(cam.Position, cam.Target, cam.Up);
    // modelview * projection
    gfx_Float16 mv = gfx_Matrix_ToFloat(matView);
    rlMultMatrixf(&mv.V[0]);
    rlEnableDepthTest();
}

void gfx_EndMode3D(void) {
    // Switch to projection matrix
    rlMatrixMode(RL_PROJECTION);
    // Restore previous matrix (projection) from matrix stack
    rlPopMatrix();
    // Switch back to modelview matrix
    rlMatrixMode(RL_MODELVIEW);
    // Reset current matrix (modelview)
    rlLoadIdentity();
    // Disable DEPTH_TEST for 2D
    rlDisableDepthTest();
}

void gfx_BeginMode2D(gfx_Camera2D cam) {
    // Reset current matrix (modelview)
    rlLoadIdentity();
    float matCamera = gfx_Matrix_ToFloat(gfx_GetCameraMatrix2D(cam)).V[0];
    // Apply 2d camera transformation to modelview
    rlMultMatrixf(&matCamera);
}

void gfx_EndMode2D(void) {
    rlDrawRenderBatchActive();
    rlLoadIdentity();
}

// Get the screen space position for a 2d camera world space position
gfx_Vector2 gfx_GetWorldToScreen2D(gfx_Vector2 position, gfx_Camera2D camera) {
    gfx_Matrix matCamera = gfx_GetCameraMatrix2D(camera);
    gfx_Vector3 transform = gfx_Vector3Transform((gfx_Vector3){position.X, position.Y, 0}, matCamera);
    return (gfx_Vector2){transform.X, transform.Y};
}

// Get the world space position for a 2d camera screen space position
gfx_Vector2 gfx_GetScreenToWorld2D(gfx_Vector2 position, gfx_Camera2D camera) {
    gfx_Matrix invMatCamera = gfx_MatrixInvert(gfx_GetCameraMatrix2D(camera));
    gfx_Vector3 transform = gfx_Vector3Transform((gfx_Vector3){position.X, position.Y, 0}, invMatCamera);
    return (gfx_Vector2){transform.X, transform.Y};
}

gfx_ImageResult gfx_LoadImage(so_String path) {
    SDL_Surface* src = SDL_LoadSurface(so_cstr(path));
    if (src == NULL) {
        gfx_ImageResult _res1 = (gfx_ImageResult){.val = (gfx_Image){}, .err = sdl_GetError()};
        SDL_DestroySurface(src);
        return _res1;
    }
    SDL_Surface* converted = SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA32);
    if (converted == NULL) {
        gfx_ImageResult _res2 = (gfx_ImageResult){.val = (gfx_Image){}, .err = sdl_GetError()};
        SDL_DestroySurface(src);
        return _res2;
    }
    gfx_ImageResult _res3 = (gfx_ImageResult){.val = (gfx_Image){.Surface = converted}, .err = (so_Error){0}};
    SDL_DestroySurface(src);
    return _res3;
}

void gfx_Image_Destroy(void* self) {
    gfx_Image* i = self;
    SDL_DestroySurface(i->Surface);
}

so_R_int_int gfx_Image_Size(void* self) {
    gfx_Image* i = self;
    return (so_R_int_int){.val = SDL_Surface_Width(*i->Surface), .val2 = SDL_Surface_Height(*i->Surface)};
}

// Get a pixel from the image.
gfx_Color gfx_Image_Get(void* self, so_int x, so_int y) {
    gfx_Image* i = self;
    if (x < 0 || y < 0 || x >= SDL_Surface_Width(*i->Surface) || y >= SDL_Surface_Height(*i->Surface)) {
        so_panic("out of bounds");
    }
    SDL_Surface* s = i->Surface;
    uint8_t* base = SDL_Surface_Pixels(*s);
    uint8_t* p = c_PtrAdd(uint8_t, (base), (y * SDL_Surface_Pitch(*s) + x * 4));
    return (gfx_Color){.R = *p, .G = *(c_PtrAdd(uint8_t, (p), (1))), .B = *(c_PtrAdd(uint8_t, (p), (2))), .A = *(c_PtrAdd(uint8_t, (p), (3)))};
}

so_Slice gfx_Image_Pixels(void* self) {
    gfx_Image* i = self;
    uint8_t* base = SDL_Surface_Pixels(*i->Surface);
    so_int size = 4 * SDL_Surface_Width(*i->Surface) * SDL_Surface_Height(*i->Surface);
    return c_Slice(uint8_t, (base), (size), (size));
}

gfx_TextureResult gfx_LoadTextureFromImage(gfx_Image img) {
    gfx_Texture t = (gfx_Texture){};
    so_R_int_int _res1 = gfx_Image_Size(&img);
    t.Width = _res1.val;
    t.Height = _res1.val2;
    t.ID = rlLoadTexture(SDL_Surface_Pixels(*img.Surface), t.Width, t.Height, rlPIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    return (gfx_TextureResult){.val = t, .err = (so_Error){0}};
}

gfx_TextureResult gfx_LoadTexture(so_String path) {
    gfx_ImageResult _res1 = gfx_LoadImage(path);
    gfx_Image img = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        gfx_TextureResult _res2 = (gfx_TextureResult){.val = (gfx_Texture){}, .err = err};
        gfx_Image_Destroy(&img);
        return _res2;
    }
    gfx_TextureResult _res3 = gfx_LoadTextureFromImage(img);
    gfx_Texture t = _res3.val;
    err = _res3.err;
    if (err.self != NULL) {
        gfx_TextureResult _res4 = (gfx_TextureResult){.val = t, .err = err};
        gfx_Image_Destroy(&img);
        return _res4;
    }
    gfx_TexturesLoaded++;
    gfx_TextureMemoryUsed += t.Width * t.Height * 4;
    gfx_TextureResult _res5 = (gfx_TextureResult){.val = t, .err = (so_Error){0}};
    gfx_Image_Destroy(&img);
    return _res5;
}

void gfx_SetTextureConfig(gfx_Texture t, bool blur, bool clamp) {
    gfx_EnableTexture(t);
    // if blur {
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR)
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
    // } else {
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
    // }
    // if clamp {
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP)
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP)
    // } else {
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT)
    // 	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT)
    // }
    gfx_DisableTexture();
}

void gfx_UnloadTexture(gfx_Texture texture) {
    if (texture.ID != 0) {
        gfx_TexturesLoaded--;
        gfx_TextureMemoryUsed -= texture.Width * texture.Height * 4;
        // gl.DeleteTextures(1, &texture.ID)
        rlUnloadTexture(texture.ID);
    }
}

void gfx_DrawTexture(gfx_Texture texture, gfx_Vector2 pos) {
    gfx_DrawTextureEx(texture, gfx_NewRectangle(0, 0, (float)(texture.Width), (float)(texture.Height)), gfx_NewRectangle((float)(pos.X), (float)(pos.Y), (float)(texture.Width), (float)(texture.Height)));
}

void gfx_DrawTextureEx(gfx_Texture texture, gfx_Rectangle src, gfx_Rectangle dst) {
    gfx_DrawTexturePro(texture, src, dst, (gfx_Vector2){}, 0, gfx_White);
}

void gfx_DrawTextureRec(gfx_Texture texture, gfx_Rectangle src, gfx_Rectangle dst) {
    gfx_DrawTexturePro(texture, src, dst, (gfx_Vector2){}, 0, gfx_White);
}

void gfx_DrawTextureTiled(gfx_Texture texture, gfx_Rectangle dest, float scale, gfx_Color tint) {
    if (texture.ID == 0) {
        return;
    }
    if (scale <= 0) {
        scale = 1;
    }
    float tileW = (float)(texture.Width) * scale;
    float tileH = (float)(texture.Height) * scale;
    // UVs larger than 1.0 cause GL_REPEAT wrapping
    float u = dest.W / tileW;
    float v = dest.H / tileH;
    gfx_EnableTexture(texture);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.R, tint.G, tint.B, tint.A);
    rlNormal3f(0, 0, 1);
    // Top-left
    rlTexCoord2f(0, 0);
    rlVertex2f(dest.X, dest.Y);
    rlTexCoord2f(0, v);
    rlVertex2f(dest.X, dest.Y + dest.H);
    // Bottom-right
    rlTexCoord2f(u, v);
    rlVertex2f(dest.X + dest.W, dest.Y + dest.H);
    // Top-right
    rlTexCoord2f(u, 0);
    rlVertex2f(dest.X + dest.W, dest.Y);
    rlEnd();
    gfx_DisableTexture();
}

// DrawTexturePro draws a portion of a texture into a destination rectangle,
// optionally rotated around origin.
//
// origin is relative to dest's size, matching raylib-style semantics.
void gfx_DrawTexturePro(gfx_Texture texture, gfx_Rectangle source, gfx_Rectangle dest, gfx_Vector2 origin, float rotation, gfx_Color tint) {
    if (texture.ID == 0) {
        return;
    }
    float width = (float)(texture.Width);
    float height = (float)(texture.Height);
    bool flipX = false;
    if (source.W < 0) {
        flipX = true;
        source.W *= -1;
    }
    // Match raylib exactly
    if (source.H < 0) {
        source.Y -= source.H;
    }
    if (dest.W < 0) {
        dest.W *= -1;
    }
    if (dest.H < 0) {
        dest.H *= -1;
    }
    gfx_Vector2 topLeft = {0}, topRight = {0}, bottomLeft = {0}, bottomRight = {0};
    if (rotation == 0) {
        float x = dest.X - origin.X;
        float y = dest.Y - origin.Y;
        topLeft = (gfx_Vector2){x, y};
        topRight = (gfx_Vector2){x + dest.W, y};
        bottomLeft = (gfx_Vector2){x, y + dest.H};
        bottomRight = (gfx_Vector2){x + dest.W, y + dest.H};
    } else {
        float rad = rotation * (math_Pi / 180.0);
        float sinR = (float)(math_Sin((double)(rad)));
        float cosR = (float)(math_Cos((double)(rad)));
        float x = dest.X;
        float y = dest.Y;
        float dx = -origin.X;
        float dy = -origin.Y;
        topLeft.X = x + dx * cosR - dy * sinR;
        topLeft.Y = y + dx * sinR + dy * cosR;
        topRight.X = x + (dx + dest.W) * cosR - dy * sinR;
        topRight.Y = y + (dx + dest.W) * sinR + dy * cosR;
        bottomLeft.X = x + dx * cosR - (dy + dest.H) * sinR;
        bottomLeft.Y = y + dx * sinR + (dy + dest.H) * cosR;
        bottomRight.X = x + (dx + dest.W) * cosR - (dy + dest.H) * sinR;
        bottomRight.Y = y + (dx + dest.W) * sinR + (dy + dest.H) * cosR;
    }
    gfx_EnableTexture(texture);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.R, tint.G, tint.B, tint.A);
    rlNormal3f(0, 0, 1);
    // Top-left
    if (flipX) {
        rlTexCoord2f((source.X + source.W) / width, source.Y / height);
    } else {
        rlTexCoord2f(source.X / width, source.Y / height);
    }
    rlVertex2f(topLeft.X, topLeft.Y);
    // Bottom-left
    if (flipX) {
        rlTexCoord2f((source.X + source.W) / width, (source.Y + source.H) / height);
    } else {
        rlTexCoord2f(source.X / width, (source.Y + source.H) / height);
    }
    rlVertex2f(bottomLeft.X, bottomLeft.Y);
    // Bottom-right
    if (flipX) {
        rlTexCoord2f(source.X / width, (source.Y + source.H) / height);
    } else {
        rlTexCoord2f((source.X + source.W) / width, (source.Y + source.H) / height);
    }
    rlVertex2f(bottomRight.X, bottomRight.Y);
    // Top-right
    if (flipX) {
        rlTexCoord2f(source.X / width, source.Y / height);
    } else {
        rlTexCoord2f((source.X + source.W) / width, source.Y / height);
    }
    rlVertex2f(topRight.X, topRight.Y);
    rlEnd();
    gfx_DisableTexture();
}

// These are all the characters allowed by Minecraft.
bool gfx_IsRuneAllowed(so_rune r) {
    return r >= 0 && r <= unicode_MaxLatin1;
}

// Load Minecraft bitmap font
gfx_FontResult gfx_LoadFont(so_String path) {
    gfx_ImageResult _res1 = gfx_LoadImage(path);
    gfx_Image img = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        gfx_FontResult _res2 = (gfx_FontResult){.val = (gfx_Font){}, .err = err};
        gfx_Image_Destroy(&img);
        return _res2;
    }
    gfx_Font fnt = (gfx_Font){};
    gfx_TextureResult _res3 = gfx_LoadTextureFromImage(img);
    fnt.Atlas = _res3.val;
    err = _res3.err;
    if (err.self != NULL) {
        gfx_FontResult _res4 = (gfx_FontResult){.val = fnt, .err = err};
        gfx_Image_Destroy(&img);
        return _res4;
    }
    so_int atlasSize = SDL_Surface_Width(*img.Surface);
    so_int glyphSize = atlasSize / glyphsPerRow;
    for (so_int charCode = 0; charCode < 256; charCode++) {
        so_int col = charCode % glyphsPerRow;
        so_int row = charCode / glyphsPerRow;
        so_int glyphWidth = glyphSize - 1;
        for (; glyphWidth >= 0;) {
            bool emptyColumn = true;
            so_int pixelX = col * glyphSize + glyphWidth;
            for (so_int y = 0; y < glyphSize; y++) {
                so_int pixelY = row * glyphSize + y;
                if (gfx_Image_Get(&img, pixelX, pixelY).A > 0) {
                    emptyColumn = false;
                    break;
                }
            }
            if (!emptyColumn) {
                break;
            }
            glyphWidth--;
        }
        if (charCode == U' ') {
            glyphWidth = 2;
        }
        fnt.CharWidths[charCode] = (uint8_t)(glyphWidth + 2);
    }
    gfx_FontResult _res5 = (gfx_FontResult){.val = fnt, .err = (so_Error){0}};
    gfx_Image_Destroy(&img);
    return _res5;
}

// TextHeight is the same as the full glyph bounding box in the Atlas.
so_int gfx_Font_TextHeight(void* self) {
    gfx_Font* fnt = self;
    return fnt->Atlas.Width / glyphsPerRow;
}

gfx_Vector2 gfx_Font_TextSize(void* self, so_Slice text) {
    gfx_Font* fnt = self;
    return (gfx_Vector2){.X = (float)(gfx_Font_TextWidth(fnt, text)), .Y = (float)(gfx_Font_TextHeight(fnt))};
}

gfx_Vector2 gfx_Font_GlyphSize(void* self, so_rune charCode) {
    gfx_Font* fnt = self;
    return (gfx_Vector2){.X = (float)(fnt->CharWidths[charCode]), .Y = (float)(gfx_Font_TextHeight(fnt))};
}

// Get text width.
so_int gfx_Font_TextWidth(void* self, so_Slice text) {
    gfx_Font* fnt = self;
    if (so_len(text) == 0) {
        return 0;
    }
    so_int width = 0.0;
    for (so_int i = 0; i < so_len(text); i++) {
        so_rune r = so_at(so_rune, text, i);
        if (r == gfx_SectionSign) {
            i++;
            continue;
        }
        if (gfx_IsRuneAllowed(r)) {
            width += (so_int)(fnt->CharWidths[r]);
        }
    }
    return width;
}

void gfx_Font_Destroy(void* self) {
    gfx_Font* fnt = self;
    gfx_UnloadTexture(fnt->Atlas);
    *fnt = (gfx_Font){};
}

void gfx_Font_DrawRunes(void* self, so_Slice text, gfx_Vector2 position, float scale, float rotation, gfx_Color color, bool darken) {
    gfx_Font* fnt = self;
    if (so_len(text) == 0) {
        return;
    }
    if (darken) {
        color.R /= 4;
        color.G /= 4;
        color.B /= 4;
    }
    float cellSize = (float)(gfx_Font_TextHeight(fnt));
    gfx_Vector2 textSize = gfx_Vector2_Scale(gfx_Font_TextSize(fnt, text), scale);
    // Pivot at center of the whole text block.
    gfx_Vector2 pivot = gfx_Vector2_Add(position, gfx_Vector2_Half(textSize));
    rlPushMatrix();
    // Move to pivot, rotate, then move back to local text space.
    rlTranslatef(pivot.X, pivot.Y, 0);
    rlRotatef(rotation, 0, 0, 1);
    rlTranslatef(-textSize.X * 0.5, -textSize.Y * 0.5, 0);
    float textOffsetX = (float)(0);
    // newlines
    float textOffsetY = (float)(0);
    for (so_int i = 0; i < so_len(text); i++) {
        for (; so_len(text) > i + 1 && so_at(so_rune, text, i) == gfx_SectionSign;) {
            // colored text using format strings
            so_int colorCode = slices_Index(so_rune, (so_string_runes(so_str("0123456789abcdef"))), (unicode_ToLower(so_at(so_rune, text, i + 1))));
            if (colorCode < 0) {
                colorCode = 15;
            }
            i += 2;
            uint8_t colorIndex = (uint8_t)(colorCode);
            if (darken) {
                colorIndex += 16;
            }
            // no clue wtf this is, thanks Notch!
            uint8_t base = (uint8_t)(((colorIndex >> 3) & 1) * 85);
            uint8_t red = (uint8_t)(((colorIndex >> 2) & 1) * 170 + base);
            uint8_t green = (uint8_t)(((colorIndex >> 1) & 1) * 170 + base);
            uint8_t blue = (uint8_t)(((colorIndex >> 0) & 1) * 170 + base);
            if (colorIndex == 6) {
                green += 85;
            }
            if (colorIndex >= 16) {
                red /= 4;
                green /= 4;
                blue /= 4;
            }
            color = (gfx_Color){red, green, blue, color.A};
        }
        so_rune charCode = so_at(so_rune, text, i);
        if (charCode == U'\n') {
            textOffsetX = 0;
            textOffsetY = +textSize.Y;
            continue;
        }
        so_rune col = charCode % glyphsPerRow;
        so_rune row = charCode / glyphsPerRow;
        gfx_Rectangle src = (gfx_Rectangle){.X = (float)(col) * cellSize, .Y = (float)(row) * cellSize, .W = cellSize, .H = cellSize};
        gfx_Rectangle dst = (gfx_Rectangle){.X = textOffsetX, .Y = 0 + textOffsetY, .W = cellSize * (float)(scale), .H = cellSize * (float)(scale)};
        gfx_DrawTexturePro(fnt->Atlas, src, dst, (gfx_Vector2){}, 0, color);
        textOffsetX += (float)(fnt->CharWidths[charCode]) * scale;
    }
    rlPopMatrix();
}

void gfx_DrawRectangle(gfx_Rectangle rectangle, gfx_Color color) {
    gfx_DrawRectanglePro(rectangle, (gfx_Vector2){}, 0, color);
}

// Draw a color-filled rectangle with pro parameters
// DrawRectanglePro draws a color-filled rectangle with rotation and origin.
//
// origin is relative to rectangle size, matching raylib semantics.
void gfx_DrawRectanglePro(gfx_Rectangle rectangle, gfx_Vector2 origin, float rotation, gfx_Color color) {
    gfx_Vector2 topLeft = {0}, topRight = {0}, bottomLeft = {0}, bottomRight = {0};
    // Normalize negative sizes
    if (rectangle.W < 0) {
        rectangle.X += rectangle.W;
        rectangle.W = -rectangle.W;
    }
    if (rectangle.H < 0) {
        rectangle.Y += rectangle.H;
        rectangle.H = -rectangle.H;
    }
    // Fast path: no rotation
    if (rotation == 0) {
        float x = rectangle.X - origin.X;
        float y = rectangle.Y - origin.Y;
        topLeft = (gfx_Vector2){x, y};
        topRight = (gfx_Vector2){x + rectangle.W, y};
        bottomLeft = (gfx_Vector2){x, y + rectangle.H};
        bottomRight = (gfx_Vector2){x + rectangle.W, y + rectangle.H};
    } else {
        float rad = rotation * gfx_Deg2rad;
        float sinR = (float)(math_Sin((double)(rad)));
        float cosR = (float)(math_Cos((double)(rad)));
        float x = rectangle.X;
        float y = rectangle.Y;
        float dx = -origin.X;
        float dy = -origin.Y;
        topLeft.X = x + dx * cosR - dy * sinR;
        topLeft.Y = y + dx * sinR + dy * cosR;
        topRight.X = x + (dx + rectangle.W) * cosR - dy * sinR;
        topRight.Y = y + (dx + rectangle.W) * sinR + dy * cosR;
        bottomLeft.X = x + dx * cosR - (dy + rectangle.H) * sinR;
        bottomLeft.Y = y + dx * sinR + (dy + rectangle.H) * cosR;
        bottomRight.X = x + (dx + rectangle.W) * cosR - (dy + rectangle.H) * sinR;
        bottomRight.Y = y + (dx + rectangle.W) * sinR + (dy + rectangle.H) * cosR;
    }
    gfx_DisableTexture();
    rlBegin(RL_TRIANGLES);
    rlColor4ub(color.R, color.G, color.B, color.A);
    rlVertex2f(topLeft.X, topLeft.Y);
    rlVertex2f(bottomLeft.X, bottomLeft.Y);
    rlVertex2f(topRight.X, topRight.Y);
    rlVertex2f(topRight.X, topRight.Y);
    rlVertex2f(bottomLeft.X, bottomLeft.Y);
    rlVertex2f(bottomRight.X, bottomRight.Y);
    rlEnd();
}

// -- math.go --

// Clamp - Clamp float value
//
float gfx_Clamp(float value, float min, float max) {
    float res = 0;
    if (value < min) {
        res = min;
    } else {
        res = value;
    }
    if (res > max) {
        return max;
    }
    return res;
}

// Lerp - Calculate linear interpolation between two floats
float gfx_Lerp(float start, float end, float amount) {
    return start + amount * (end - start);
}

// Normalize - Normalize input value within input range
float gfx_Normalize(float value, float start, float end) {
    return (value - start) / (end - start);
}

// Remap - Remap input value within input range to output range
float gfx_Remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd) {
    return (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart;
}

// Wrap - Wrap input value from min to max
float gfx_Wrap(float value, float min, float max) {
    return value - (max - min) * (float)(math_Floor((double)((value - min) / (max - min))));
}

// FloatEquals - Check whether two given floats are almost equal
bool gfx_FloatEquals(float x, float y) {
    return (math_Abs((double)(x - y)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(x)), math_Abs((double)(y)))));
}

// Vector2Zero - Vector with components value 0.0
gfx_Vector2 gfx_Vector2Zero(void) {
    return gfx_NewVector2(0.0, 0.0);
}

// Vector2One - Vector with components value 1.0
gfx_Vector2 gfx_Vector2One(void) {
    return gfx_NewVector2(1.0, 1.0);
}

// Vector2Add - Add two vectors (v1 + v2)
gfx_Vector2 gfx_Vector2Add(gfx_Vector2 v1, gfx_Vector2 v2) {
    return gfx_NewVector2(v1.X + v2.X, v1.Y + v2.Y);
}

// Vector2AddValue - Add vector and float value
gfx_Vector2 gfx_Vector2AddValue(gfx_Vector2 v, float add) {
    return gfx_NewVector2(v.X + add, v.Y + add);
}

// Vector2Subtract - Subtract two vectors (v1 - v2)
gfx_Vector2 gfx_Vector2Subtract(gfx_Vector2 v1, gfx_Vector2 v2) {
    return gfx_NewVector2(v1.X - v2.X, v1.Y - v2.Y);
}

// Vector2SubtractValue - Subtract vector by float value
gfx_Vector2 gfx_Vector2SubtractValue(gfx_Vector2 v, float sub) {
    return gfx_NewVector2(v.X - sub, v.Y - sub);
}

// Vector2Length - Calculate vector length
float gfx_Vector2Length(gfx_Vector2 v) {
    return (float)(math_Sqrt((double)((v.X * v.X) + (v.Y * v.Y))));
}

// Vector2LengthSqr - Calculate vector square length
float gfx_Vector2LengthSqr(gfx_Vector2 v) {
    return v.X * v.X + v.Y * v.Y;
}

// Vector2DotProduct - Calculate two vectors dot product
float gfx_Vector2DotProduct(gfx_Vector2 v1, gfx_Vector2 v2) {
    return v1.X * v2.X + v1.Y * v2.Y;
}

// Vector2Distance - Calculate distance between two vectors
float gfx_Vector2Distance(gfx_Vector2 v1, gfx_Vector2 v2) {
    return (float)(math_Sqrt((double)((v1.X - v2.X) * (v1.X - v2.X) + (v1.Y - v2.Y) * (v1.Y - v2.Y))));
}

// Vector2DistanceSqr - Calculate square distance between two vectors
float gfx_Vector2DistanceSqr(gfx_Vector2 v1, gfx_Vector2 v2) {
    return (v1.X - v2.X) * (v1.X - v2.X) + (v1.Y - v2.Y) * (v1.Y - v2.Y);
}

// Vector2Angle - Calculate angle from two vectors in radians
// NOTE: Coordinate system convention: positive X right, positive Y down,
// positive angles appear clockwise, and negative angles appear counterclockwise
float gfx_Vector2Angle(gfx_Vector2 v1, gfx_Vector2 v2) {
    float dot = v1.X * v2.X + v1.Y * v2.Y;
    float det = v1.X * v2.Y - v1.Y * v2.X;
    return (float)(math_Atan2((double)(det), (double)(dot)));
}

// Vector2LineAngle - Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized. Current implementation should be aligned with glm::angle
float gfx_Vector2LineAngle(gfx_Vector2 start, gfx_Vector2 end) {
    return (float)(-math_Atan2((double)(end.Y - start.Y), (double)(end.X - start.X)));
}

// Vector2Scale - Scale vector (multiply by value)
gfx_Vector2 gfx_Vector2Scale(gfx_Vector2 v, float scale) {
    return gfx_NewVector2(v.X * scale, v.Y * scale);
}

// Vector2Multiply - Multiply vector by vector
gfx_Vector2 gfx_Vector2Multiply(gfx_Vector2 v1, gfx_Vector2 v2) {
    return gfx_NewVector2(v1.X * v2.X, v1.Y * v2.Y);
}

// Vector2Negate - Negate vector
gfx_Vector2 gfx_Vector2Negate(gfx_Vector2 v) {
    return gfx_NewVector2(-v.X, -v.Y);
}

// Vector2Divide - Divide vector by vector
gfx_Vector2 gfx_Vector2Divide(gfx_Vector2 v1, gfx_Vector2 v2) {
    return gfx_NewVector2(v1.X / v2.X, v1.Y / v2.Y);
}

// Vector2Normalize - Normalize provided vector
gfx_Vector2 gfx_Vector2Normalize(gfx_Vector2 v) {
    {
        float l = gfx_Vector2Length(v);
        if (l > 0) {
            return gfx_Vector2Scale(v, 1 / l);
        }
    }
    return v;
}

// Vector2Transform - Transforms a Vector2 by a given Matrix
gfx_Vector2 gfx_Vector2Transform(gfx_Vector2 v, gfx_Matrix mat) {
    gfx_Vector2 result = (gfx_Vector2){};
    float x = v.X;
    float y = v.Y;
    float z = 0;
    result.X = mat.M0 * x + mat.M4 * y + mat.M8 * z + mat.M12;
    result.Y = mat.M1 * x + mat.M5 * y + mat.M9 * z + mat.M13;
    return result;
}

// Vector2Lerp - Calculate linear interpolation between two vectors
gfx_Vector2 gfx_Vector2Lerp(gfx_Vector2 v1, gfx_Vector2 v2, float amount) {
    return gfx_NewVector2(v1.X + amount * (v2.X - v1.X), v1.Y + amount * (v2.Y - v1.Y));
}

// Vector2Reflect - Calculate reflected vector to normal
gfx_Vector2 gfx_Vector2Reflect(gfx_Vector2 v, gfx_Vector2 normal) {
    gfx_Vector2 result = (gfx_Vector2){};
    // Dot product
    float dotProduct = v.X * normal.X + v.Y * normal.Y;
    result.X = v.X - 2.0 * normal.X * dotProduct;
    result.Y = v.Y - 2.0 * normal.Y * dotProduct;
    return result;
}

// Vector2Rotate - Rotate vector by angle
gfx_Vector2 gfx_Vector2Rotate(gfx_Vector2 v, float angle) {
    gfx_Vector2 result = (gfx_Vector2){};
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    result.X = v.X * cosres - v.Y * sinres;
    result.Y = v.X * sinres + v.Y * cosres;
    return result;
}

// Vector2MoveTowards - Move Vector towards target
gfx_Vector2 gfx_Vector2MoveTowards(gfx_Vector2 v, gfx_Vector2 target, float maxDistance) {
    gfx_Vector2 result = (gfx_Vector2){};
    float dx = target.X - v.X;
    float dy = target.Y - v.Y;
    float value = dx * dx + dy * dy;
    if (value == 0 || maxDistance >= 0 && value <= maxDistance * maxDistance) {
        return target;
    }
    float dist = (float)(math_Sqrt((double)(value)));
    result.X = v.X + dx / dist * maxDistance;
    result.Y = v.Y + dy / dist * maxDistance;
    return result;
}

// Vector2Invert - Invert the given vector
gfx_Vector2 gfx_Vector2Invert(gfx_Vector2 v) {
    return gfx_NewVector2(1.0 / v.X, 1.0 / v.Y);
}

// Vector2Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector2 gfx_Vector2Clamp(gfx_Vector2 v, gfx_Vector2 min, gfx_Vector2 max) {
    gfx_Vector2 result = (gfx_Vector2){};
    result.X = (float)(math_Min((double)(max.X), math_Max((double)(min.X), (double)(v.X))));
    result.Y = (float)(math_Min((double)(max.Y), math_Max((double)(min.Y), (double)(v.Y))));
    return result;
}

// Vector2ClampValue - Clamp the magnitude of the vector between two min and max values
gfx_Vector2 gfx_Vector2ClampValue(gfx_Vector2 v, float min, float max) {
    gfx_Vector2 result = v;
    float length = v.X * v.X + v.Y * v.Y;
    if (length > 0.0) {
        length = (float)(math_Sqrt((double)(length)));
        if (length < min) {
            float scale = min / length;
            result.X = v.X * scale;
            result.Y = v.Y * scale;
        } else if (length > max) {
            float scale = max / length;
            result.X = v.X * scale;
            result.Y = v.Y * scale;
        }
    }
    return result;
}

// Vector2Equals - Check whether two given vectors are almost equal
bool gfx_Vector2Equals(gfx_Vector2 p, gfx_Vector2 q) {
    return (math_Abs((double)(p.X - q.X)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(p.X)), math_Abs((double)(q.X)))) && math_Abs((double)(p.Y - q.Y)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(p.Y)), math_Abs((double)(q.Y)))));
}

// Vector2CrossProduct - Calculate two vectors cross product
float gfx_Vector2CrossProduct(gfx_Vector2 v1, gfx_Vector2 v2) {
    return v1.X * v2.Y - v1.Y * v2.X;
}

// Vector2Cross - Calculate the cross product of a vector and a value
gfx_Vector2 gfx_Vector2Cross(float value, gfx_Vector2 vector) {
    return gfx_NewVector2(-value * vector.Y, value * vector.X);
}

// Vector3Zero - Vector with components value 0.0
gfx_Vector3 gfx_Vector3Zero(void) {
    return gfx_NewVector3(0.0, 0.0, 0.0);
}

// Vector3One - Vector with components value 1.0
gfx_Vector3 gfx_Vector3One(void) {
    return gfx_NewVector3(1.0, 1.0, 1.0);
}

// Vector3Add - Add two vectors
gfx_Vector3 gfx_Vector3Add(gfx_Vector3 v1, gfx_Vector3 v2) {
    return gfx_NewVector3(v1.X + v2.X, v1.Y + v2.Y, v1.Z + v2.Z);
}

// Vector3AddValue - Add vector and float value
gfx_Vector3 gfx_Vector3AddValue(gfx_Vector3 v, float add) {
    return gfx_NewVector3(v.X + add, v.Y + add, v.Z + add);
}

// Vector3Subtract - Subtract two vectors
gfx_Vector3 gfx_Vector3Subtract(gfx_Vector3 v1, gfx_Vector3 v2) {
    return gfx_NewVector3(v1.X - v2.X, v1.Y - v2.Y, v1.Z - v2.Z);
}

// Vector3SubtractValue - Subtract vector by float value
gfx_Vector3 gfx_Vector3SubtractValue(gfx_Vector3 v, float sub) {
    return gfx_NewVector3(v.X - sub, v.Y - sub, v.Z - sub);
}

// Vector3Scale - Scale provided vector
gfx_Vector3 gfx_Vector3Scale(gfx_Vector3 v, float scale) {
    return gfx_NewVector3(v.X * scale, v.Y * scale, v.Z * scale);
}

// Vector3Multiply - Multiply vector by vector
gfx_Vector3 gfx_Vector3Multiply(gfx_Vector3 v1, gfx_Vector3 v2) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = v1.X * v2.X;
    result.Y = v1.Y * v2.Y;
    result.Z = v1.Z * v2.Z;
    return result;
}

// Vector3CrossProduct - Calculate two vectors cross product
gfx_Vector3 gfx_Vector3CrossProduct(gfx_Vector3 v1, gfx_Vector3 v2) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = v1.Y * v2.Z - v1.Z * v2.Y;
    result.Y = v1.Z * v2.X - v1.X * v2.Z;
    result.Z = v1.X * v2.Y - v1.Y * v2.X;
    return result;
}

// Vector3Perpendicular - Calculate one vector perpendicular vector
gfx_Vector3 gfx_Vector3Perpendicular(gfx_Vector3 v) {
    double min = math_Abs((double)(v.X));
    gfx_Vector3 cardinalAxis = gfx_NewVector3(1.0, 0.0, 0.0);
    if (math_Abs((double)(v.Y)) < min) {
        min = math_Abs((double)(v.Y));
        cardinalAxis = gfx_NewVector3(0.0, 1.0, 0.0);
    }
    if (math_Abs((double)(v.Z)) < min) {
        cardinalAxis = gfx_NewVector3(0.0, 0.0, 1.0);
    }
    gfx_Vector3 result = gfx_Vector3CrossProduct(v, cardinalAxis);
    return result;
}

// Vector3Length - Calculate vector length
float gfx_Vector3Length(gfx_Vector3 v) {
    return (float)(math_Sqrt((double)(v.X * v.X + v.Y * v.Y + v.Z * v.Z)));
}

// Vector3LengthSqr - Calculate vector square length
float gfx_Vector3LengthSqr(gfx_Vector3 v) {
    return v.X * v.X + v.Y * v.Y + v.Z * v.Z;
}

// Vector3DotProduct - Calculate two vectors dot product
float gfx_Vector3DotProduct(gfx_Vector3 v1, gfx_Vector3 v2) {
    return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
}

// Vector3Distance - Calculate distance between two vectors
float gfx_Vector3Distance(gfx_Vector3 v1, gfx_Vector3 v2) {
    float dx = v2.X - v1.X;
    float dy = v2.Y - v1.Y;
    float dz = v2.Z - v1.Z;
    return (float)(math_Sqrt((double)(dx * dx + dy * dy + dz * dz)));
}

// Vector3DistanceSqr - Calculate square distance between two vectors
float gfx_Vector3DistanceSqr(gfx_Vector3 v1, gfx_Vector3 v2) {
    float result = 0;
    float dx = v2.X - v1.X;
    float dy = v2.Y - v1.Y;
    float dz = v2.Z - v1.Z;
    result = dx * dx + dy * dy + dz * dz;
    return result;
}

// Vector3Angle - Calculate angle between two vectors
float gfx_Vector3Angle(gfx_Vector3 v1, gfx_Vector3 v2) {
    float result = 0;
    gfx_Vector3 cross = (gfx_Vector3){.X = v1.Y * v2.Z - v1.Z * v2.Y, .Y = v1.Z * v2.X - v1.X * v2.Z, .Z = v1.X * v2.Y - v1.Y * v2.X};
    float length = (float)(math_Sqrt((double)(cross.X * cross.X + cross.Y * cross.Y + cross.Z * cross.Z)));
    float dot = v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z;
    result = (float)(math_Atan2((double)(length), (double)(dot)));
    return result;
}

// Vector3Negate - Negate provided vector (invert direction)
gfx_Vector3 gfx_Vector3Negate(gfx_Vector3 v) {
    return gfx_NewVector3(-v.X, -v.Y, -v.Z);
}

// Vector3Divide - Divide vector by vector
gfx_Vector3 gfx_Vector3Divide(gfx_Vector3 v1, gfx_Vector3 v2) {
    return gfx_NewVector3(v1.X / v2.X, v1.Y / v2.Y, v1.Z / v2.Z);
}

// Vector3Normalize - Normalize provided vector
gfx_Vector3 gfx_Vector3Normalize(gfx_Vector3 v) {
    gfx_Vector3 result = v;
    float length = 0, ilength = 0;
    length = gfx_Vector3Length(v);
    if (length == 0) {
        length = 1.0;
    }
    ilength = 1.0 / length;
    result.X *= ilength;
    result.Y *= ilength;
    result.Z *= ilength;
    return result;
}

// Vector3Project - Calculate the projection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3Project(gfx_Vector3 v1, gfx_Vector3 v2) {
    gfx_Vector3 result = (gfx_Vector3){};
    float v1dv2 = (v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z);
    float v2dv2 = (v2.X * v2.X + v2.Y * v2.Y + v2.Z * v2.Z);
    float mag = v1dv2 / v2dv2;
    result.X = v2.X * mag;
    result.Y = v2.Y * mag;
    result.Z = v2.Z * mag;
    return result;
}

// Vector3Reject - Calculate the rejection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3Reject(gfx_Vector3 v1, gfx_Vector3 v2) {
    gfx_Vector3 result = (gfx_Vector3){};
    float v1dv2 = (v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z);
    float v2dv2 = (v2.X * v2.X + v2.Y * v2.Y + v2.Z * v2.Z);
    float mag = v1dv2 / v2dv2;
    result.X = v1.X - (v2.X * mag);
    result.Y = v1.Y - (v2.Y * mag);
    result.Z = v1.Z - (v2.Z * mag);
    return result;
}

// Vector3OrthoNormalize - Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
void gfx_Vector3OrthoNormalize(gfx_Vector3* v1, gfx_Vector3* v2) {
    *v1 = gfx_Vector3Normalize(*v1);
    gfx_Vector3 vn1 = gfx_Vector3CrossProduct(*v1, *v2);
    vn1 = gfx_Vector3Normalize(vn1);
    gfx_Vector3 vn2 = gfx_Vector3CrossProduct(vn1, *v1);
    *v2 = vn2;
}

// Vector3Transform - Transforms a Vector3 by a given Matrix
gfx_Vector3 gfx_Vector3Transform(gfx_Vector3 v, gfx_Matrix mat) {
    gfx_Vector3 result = (gfx_Vector3){};
    float x = v.X;
    float y = v.Y;
    float z = v.Z;
    result.X = mat.M0 * x + mat.M4 * y + mat.M8 * z + mat.M12;
    result.Y = mat.M1 * x + mat.M5 * y + mat.M9 * z + mat.M13;
    result.Z = mat.M2 * x + mat.M6 * y + mat.M10 * z + mat.M14;
    return result;
}

// Vector3RotateByQuaternion - Transform a vector by quaternion rotation
gfx_Vector3 gfx_Vector3RotateByQuaternion(gfx_Vector3 v, gfx_Vector4 q) {
    gfx_Vector3 result = {0};
    result.X = v.X * (q.X * q.X + q.W * q.W - q.Y * q.Y - q.Z * q.Z) + v.Y * (2 * q.X * q.Y - 2 * q.W * q.Z) + v.Z * (2 * q.X * q.Z + 2 * q.W * q.Y);
    result.Y = v.X * (2 * q.W * q.Z + 2 * q.X * q.Y) + v.Y * (q.W * q.W - q.X * q.X + q.Y * q.Y - q.Z * q.Z) + v.Z * (-2 * q.W * q.X + 2 * q.Y * q.Z);
    result.Z = v.X * (-2 * q.W * q.Y + 2 * q.X * q.Z) + v.Y * (2 * q.W * q.X + 2 * q.Y * q.Z) + v.Z * (q.W * q.W - q.X * q.X - q.Y * q.Y + q.Z * q.Z);
    return result;
}

// Vector3RotateByAxisAngle - Rotates a vector around an axis
gfx_Vector3 gfx_Vector3RotateByAxisAngle(gfx_Vector3 v, gfx_Vector3 axis, float angle) {
    // Using Euler-Rodrigues Formula
    // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula
    gfx_Vector3 result = v;
    // Vector3Normalize(axis);
    float length = (float)(math_Sqrt((double)(axis.X * axis.X + axis.Y * axis.Y + axis.Z * axis.Z)));
    if (length == 0.0) {
        length = 1.0;
    }
    float ilength = 1.0 / length;
    axis.X *= ilength;
    axis.Y *= ilength;
    axis.Z *= ilength;
    angle /= 2.0;
    float a = (float)(math_Sin((double)(angle)));
    float b = axis.X * a;
    float c = axis.Y * a;
    float d = axis.Z * a;
    a = (float)(math_Cos((double)(angle)));
    gfx_Vector3 w = gfx_NewVector3(b, c, d);
    // Vector3CrossProduct(w, v)
    gfx_Vector3 wv = gfx_NewVector3(w.Y * v.Z - w.Z * v.Y, w.Z * v.X - w.X * v.Z, w.X * v.Y - w.Y * v.X);
    // Vector3CrossProduct(w, wv)
    gfx_Vector3 wwv = gfx_NewVector3(w.Y * wv.Z - w.Z * wv.Y, w.Z * wv.X - w.X * wv.Z, w.X * wv.Y - w.Y * wv.X);
    // Vector3Scale(wv, 2*a)
    a *= 2;
    wv.X *= a;
    wv.Y *= a;
    wv.Z *= a;
    // Vector3Scale(wwv, 2)
    wwv.X *= 2;
    wwv.Y *= 2;
    wwv.Z *= 2;
    result.X += wv.X;
    result.Y += wv.Y;
    result.Z += wv.Z;
    result.X += wwv.X;
    result.Y += wwv.Y;
    result.Z += wwv.Z;
    return result;
}

// Vector3Lerp - Calculate linear interpolation between two vectors
gfx_Vector3 gfx_Vector3Lerp(gfx_Vector3 v1, gfx_Vector3 v2, float amount) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = v1.X + amount * (v2.X - v1.X);
    result.Y = v1.Y + amount * (v2.Y - v1.Y);
    result.Z = v1.Z + amount * (v2.Z - v1.Z);
    return result;
}

// Vector3Reflect - Calculate reflected vector to normal
gfx_Vector3 gfx_Vector3Reflect(gfx_Vector3 vector, gfx_Vector3 normal) {
    // I is the original vector
    // N is the normal of the incident plane
    // R = I - (2*N*( DotProduct[ I,N] ))
    gfx_Vector3 result = (gfx_Vector3){};
    float dotProduct = gfx_Vector3DotProduct(vector, normal);
    result.X = vector.X - (2.0 * normal.X) * dotProduct;
    result.Y = vector.Y - (2.0 * normal.Y) * dotProduct;
    result.Z = vector.Z - (2.0 * normal.Z) * dotProduct;
    return result;
}

// Vector3Min - Return min value for each pair of components
gfx_Vector3 gfx_Vector3Min(gfx_Vector3 vec1, gfx_Vector3 vec2) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = (float)(math_Min((double)(vec1.X), (double)(vec2.X)));
    result.Y = (float)(math_Min((double)(vec1.Y), (double)(vec2.Y)));
    result.Z = (float)(math_Min((double)(vec1.Z), (double)(vec2.Z)));
    return result;
}

// Vector3Max - Return max value for each pair of components
gfx_Vector3 gfx_Vector3Max(gfx_Vector3 vec1, gfx_Vector3 vec2) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = (float)(math_Max((double)(vec1.X), (double)(vec2.X)));
    result.Y = (float)(math_Max((double)(vec1.Y), (double)(vec2.Y)));
    result.Z = (float)(math_Max((double)(vec1.Z), (double)(vec2.Z)));
    return result;
}

// Vector3Barycenter - Barycenter coords for p in triangle abc
gfx_Vector3 gfx_Vector3Barycenter(gfx_Vector3 p, gfx_Vector3 a, gfx_Vector3 b, gfx_Vector3 c) {
    gfx_Vector3 v0 = gfx_Vector3Subtract(b, a);
    gfx_Vector3 v1 = gfx_Vector3Subtract(c, a);
    gfx_Vector3 v2 = gfx_Vector3Subtract(p, a);
    float d00 = gfx_Vector3DotProduct(v0, v0);
    float d01 = gfx_Vector3DotProduct(v0, v1);
    float d11 = gfx_Vector3DotProduct(v1, v1);
    float d20 = gfx_Vector3DotProduct(v2, v0);
    float d21 = gfx_Vector3DotProduct(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    gfx_Vector3 result = (gfx_Vector3){};
    result.Y = (d11 * d20 - d01 * d21) / denom;
    result.Z = (d00 * d21 - d01 * d20) / denom;
    result.X = 1.0 - (result.Z + result.Y);
    return result;
}

// Vector3Unproject - Projects a Vector3 from screen space into object space
// NOTE: We are avoiding calling other raymath functions despite available
gfx_Vector3 gfx_Vector3Unproject(gfx_Vector3 source, gfx_Matrix projection, gfx_Matrix view) {
    gfx_Vector3 result = (gfx_Vector3){};
    // Calculate unprojected matrix (multiply view matrix by projection matrix) and invert it
    gfx_Matrix matViewProj = (gfx_Matrix){.M0 = view.M0 * projection.M0 + view.M1 * projection.M4 + view.M2 * projection.M8 + view.M3 * projection.M12, .M4 = view.M0 * projection.M1 + view.M1 * projection.M5 + view.M2 * projection.M9 + view.M3 * projection.M13, .M8 = view.M0 * projection.M2 + view.M1 * projection.M6 + view.M2 * projection.M10 + view.M3 * projection.M14, .M12 = view.M0 * projection.M3 + view.M1 * projection.M7 + view.M2 * projection.M11 + view.M3 * projection.M15, .M1 = view.M4 * projection.M0 + view.M5 * projection.M4 + view.M6 * projection.M8 + view.M7 * projection.M12, .M5 = view.M4 * projection.M1 + view.M5 * projection.M5 + view.M6 * projection.M9 + view.M7 * projection.M13, .M9 = view.M4 * projection.M2 + view.M5 * projection.M6 + view.M6 * projection.M10 + view.M7 * projection.M14, .M13 = view.M4 * projection.M3 + view.M5 * projection.M7 + view.M6 * projection.M11 + view.M7 * projection.M15, .M2 = view.M8 * projection.M0 + view.M9 * projection.M4 + view.M10 * projection.M8 + view.M11 * projection.M12, .M6 = view.M8 * projection.M1 + view.M9 * projection.M5 + view.M10 * projection.M9 + view.M11 * projection.M13, .M10 = view.M8 * projection.M2 + view.M9 * projection.M6 + view.M10 * projection.M10 + view.M11 * projection.M14, .M14 = view.M8 * projection.M3 + view.M9 * projection.M7 + view.M10 * projection.M11 + view.M11 * projection.M15, .M3 = view.M12 * projection.M0 + view.M13 * projection.M4 + view.M14 * projection.M8 + view.M15 * projection.M12, .M7 = view.M12 * projection.M1 + view.M13 * projection.M5 + view.M14 * projection.M9 + view.M15 * projection.M13, .M11 = view.M12 * projection.M2 + view.M13 * projection.M6 + view.M14 * projection.M10 + view.M15 * projection.M14, .M15 = view.M12 * projection.M3 + view.M13 * projection.M7 + view.M14 * projection.M11 + view.M15 * projection.M15};
    // Calculate inverted matrix -> MatrixInvert(matViewProj);
    // Cache the matrix values (speed optimization)
    float a00 = matViewProj.M0;
    float a01 = matViewProj.M1;
    float a02 = matViewProj.M2;
    float a03 = matViewProj.M3;
    float a10 = matViewProj.M4;
    float a11 = matViewProj.M5;
    float a12 = matViewProj.M6;
    float a13 = matViewProj.M7;
    float a20 = matViewProj.M8;
    float a21 = matViewProj.M9;
    float a22 = matViewProj.M10;
    float a23 = matViewProj.M11;
    float a30 = matViewProj.M12;
    float a31 = matViewProj.M13;
    float a32 = matViewProj.M14;
    float a33 = matViewProj.M15;
    float b00 = a00 * a11 - a01 * a10;
    float b01 = a00 * a12 - a02 * a10;
    float b02 = a00 * a13 - a03 * a10;
    float b03 = a01 * a12 - a02 * a11;
    float b04 = a01 * a13 - a03 * a11;
    float b05 = a02 * a13 - a03 * a12;
    float b06 = a20 * a31 - a21 * a30;
    float b07 = a20 * a32 - a22 * a30;
    float b08 = a20 * a33 - a23 * a30;
    float b09 = a21 * a32 - a22 * a31;
    float b10 = a21 * a33 - a23 * a31;
    float b11 = a22 * a33 - a23 * a32;
    // Calculate the invert determinant (inlined to avoid double-caching)
    float invDet = 1.0 / (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06);
    gfx_Matrix matViewProjInv = (gfx_Matrix){.M0 = (a11 * b11 - a12 * b10 + a13 * b09) * invDet, .M4 = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet, .M8 = (a31 * b05 - a32 * b04 + a33 * b03) * invDet, .M12 = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet, .M1 = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet, .M5 = (a00 * b11 - a02 * b08 + a03 * b07) * invDet, .M9 = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet, .M13 = (a20 * b05 - a22 * b02 + a23 * b01) * invDet, .M2 = (a10 * b10 - a11 * b08 + a13 * b06) * invDet, .M6 = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet, .M10 = (a30 * b04 - a31 * b02 + a33 * b00) * invDet, .M14 = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet, .M3 = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet, .M7 = (a00 * b09 - a01 * b07 + a02 * b06) * invDet, .M11 = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet, .M15 = (a20 * b03 - a21 * b01 + a22 * b00) * invDet};
    // Create quaternion from source point
    gfx_Vector4 quat = (gfx_Vector4){.X = source.X, .Y = source.Y, .Z = source.Z, .W = 1.0};
    // Multiply quat point by unprojecte matrix
    gfx_Vector4 qtransformed = (gfx_Vector4){.X = matViewProjInv.M0 * quat.X + matViewProjInv.M4 * quat.Y + matViewProjInv.M8 * quat.Z + matViewProjInv.M12 * quat.W, .Y = matViewProjInv.M1 * quat.X + matViewProjInv.M5 * quat.Y + matViewProjInv.M9 * quat.Z + matViewProjInv.M13 * quat.W, .Z = matViewProjInv.M2 * quat.X + matViewProjInv.M6 * quat.Y + matViewProjInv.M10 * quat.Z + matViewProjInv.M14 * quat.W, .W = matViewProjInv.M3 * quat.X + matViewProjInv.M7 * quat.Y + matViewProjInv.M11 * quat.Z + matViewProjInv.M15 * quat.W};
    // Normalized world points in vectors
    result.X = qtransformed.X / qtransformed.W;
    result.Y = qtransformed.Y / qtransformed.W;
    result.Z = qtransformed.Z / qtransformed.W;
    return result;
}

// Vector3ToFloatV - Get Vector3 as float array
gfx_Float3 gfx_Vector3ToFloat(gfx_Vector3 v) {
    gfx_Float3 result = {0};
    result.V[0] = v.X;
    result.V[1] = v.Y;
    result.V[2] = v.Z;
    return result;
}

// Vector3Invert - Invert the given vector
gfx_Vector3 gfx_Vector3Invert(gfx_Vector3 v) {
    return gfx_NewVector3(1.0 / v.X, 1.0 / v.Y, 1.0 / v.Z);
}

// Vector3Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector3 gfx_Vector3Clamp(gfx_Vector3 v, gfx_Vector3 min, gfx_Vector3 max) {
    gfx_Vector3 result = (gfx_Vector3){};
    result.X = (float)(math_Min((double)(max.X), math_Max((double)(min.X), (double)(v.X))));
    result.Y = (float)(math_Min((double)(max.Y), math_Max((double)(min.Y), (double)(v.Y))));
    result.Z = (float)(math_Min((double)(max.Z), math_Max((double)(min.Z), (double)(v.Z))));
    return result;
}

// Vector3ClampValue - Clamp the magnitude of the vector between two values
gfx_Vector3 gfx_Vector3ClampValue(gfx_Vector3 v, float min, float max) {
    gfx_Vector3 result = v;
    float length = v.X * v.X + v.Y * v.Y + v.Z * v.Z;
    if (length > 0.0) {
        length = (float)(math_Sqrt((double)(length)));
        if (length < min) {
            float scale = min / length;
            result.X = v.X * scale;
            result.Y = v.Y * scale;
            result.Z = v.Z * scale;
        } else if (length > max) {
            float scale = max / length;
            result.X = v.X * scale;
            result.Y = v.Y * scale;
            result.Z = v.Z * scale;
        }
    }
    return result;
}

// Vector3Equals - Check whether two given vectors are almost equal
bool gfx_Vector3Equals(gfx_Vector3 p, gfx_Vector3 q) {
    return (math_Abs((double)(p.X - q.X)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(p.X)), math_Abs((double)(q.X)))) && math_Abs((double)(p.Y - q.Y)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(p.Y)), math_Abs((double)(q.Y)))) && math_Abs((double)(p.Z - q.Z)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(p.Z)), math_Abs((double)(q.Z)))));
}

// Vector3Refract - Compute the direction of a refracted ray
//
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface
gfx_Vector3 gfx_Vector3Refract(gfx_Vector3 v, gfx_Vector3 n, float r) {
    gfx_Vector3 result = (gfx_Vector3){};
    float dot = v.X * n.X + v.Y * n.Y + v.Z * n.Z;
    float d = 1.0 - r * r * (1.0 - dot * dot);
    if (d >= 0.0) {
        d = (float)(math_Sqrt((double)(d)));
        v.X = r * v.X - (r * dot + d) * n.X;
        v.Y = r * v.Y - (r * dot + d) * n.Y;
        v.Z = r * v.Z - (r * dot + d) * n.Z;
        result = v;
    }
    return result;
}

// Mat2Radians - Creates a matrix 2x2 from a given radians value
gfx_Mat2 gfx_Mat2Radians(float radians) {
    so_R_f32_f32 _res1 = gfx_Sincos(radians);
    float s = _res1.val;
    float c = _res1.val2;
    return gfx_NewMat2(c, -s, s, c);
}

// Mat2Set - Set values from radians to a created matrix 2x2
void gfx_Mat2Set(gfx_Mat2* matrix, float radians) {
    so_R_f32_f32 _res1 = gfx_Sincos(radians);
    float sin = _res1.val;
    float cos = _res1.val2;
    matrix->M00 = cos;
    matrix->M01 = -sin;
    matrix->M10 = sin;
    matrix->M11 = cos;
}

// Mat2Transpose - Returns the transpose of a given matrix 2x2
gfx_Mat2 gfx_Mat2Transpose(gfx_Mat2 matrix) {
    return gfx_NewMat2(matrix.M00, matrix.M10, matrix.M01, matrix.M11);
}

// Mat2MultiplyVector2 - Multiplies a vector by a matrix 2x2
gfx_Vector2 gfx_Mat2MultiplyVector2(gfx_Mat2 matrix, gfx_Vector2 vector) {
    return gfx_NewVector2(matrix.M00 * vector.X + matrix.M01 * vector.Y, matrix.M10 * vector.X + matrix.M11 * vector.Y);
}

// MatrixDeterminant - Compute matrix determinant
float gfx_MatrixDeterminant(gfx_Matrix mat) {
    float m0 = mat.M0;
    float m1 = mat.M1;
    float m2 = mat.M2;
    float m3 = mat.M3;
    float m4 = mat.M4;
    float m5 = mat.M5;
    float m6 = mat.M6;
    float m7 = mat.M7;
    float m8 = mat.M8;
    float m9 = mat.M9;
    float m10 = mat.M10;
    float m11 = mat.M11;
    float m12 = mat.M12;
    float m13 = mat.M13;
    float m14 = mat.M14;
    float m15 = mat.M15;
    return (m0 * (m5 * (m10 * m15 - m11 * m14) - m9 * (m6 * m15 - m7 * m14) + m13 * (m6 * m11 - m7 * m10)) - m4 * (m1 * (m10 * m15 - m11 * m14) - m9 * (m2 * m15 - m3 * m14) + m13 * (m2 * m11 - m3 * m10)) + m8 * (m1 * (m6 * m15 - m7 * m14) - m5 * (m2 * m15 - m3 * m14) + m13 * (m2 * m7 - m3 * m6)) - m12 * (m1 * (m6 * m11 - m7 * m10) - m5 * (m2 * m11 - m3 * m10) + m9 * (m2 * m7 - m3 * m6)));
}

// MatrixTrace - Returns the trace of the matrix (sum of the values along the diagonal)
float gfx_MatrixTrace(gfx_Matrix mat) {
    return mat.M0 + mat.M5 + mat.M10 + mat.M15;
}

// MatrixTranspose - Transposes provided matrix
gfx_Matrix gfx_MatrixTranspose(gfx_Matrix mat) {
    gfx_Matrix result = {0};
    result.M0 = mat.M0;
    result.M1 = mat.M4;
    result.M2 = mat.M8;
    result.M3 = mat.M12;
    result.M4 = mat.M1;
    result.M5 = mat.M5;
    result.M6 = mat.M9;
    result.M7 = mat.M13;
    result.M8 = mat.M2;
    result.M9 = mat.M6;
    result.M10 = mat.M10;
    result.M11 = mat.M14;
    result.M12 = mat.M3;
    result.M13 = mat.M7;
    result.M14 = mat.M11;
    result.M15 = mat.M15;
    return result;
}

// MatrixInvert - Invert provided matrix
gfx_Matrix gfx_MatrixInvert(gfx_Matrix mat) {
    gfx_Matrix result = {0};
    float a00 = mat.M0;
    float a01 = mat.M1;
    float a02 = mat.M2;
    float a03 = mat.M3;
    float a10 = mat.M4;
    float a11 = mat.M5;
    float a12 = mat.M6;
    float a13 = mat.M7;
    float a20 = mat.M8;
    float a21 = mat.M9;
    float a22 = mat.M10;
    float a23 = mat.M11;
    float a30 = mat.M12;
    float a31 = mat.M13;
    float a32 = mat.M14;
    float a33 = mat.M15;
    float b00 = a00 * a11 - a01 * a10;
    float b01 = a00 * a12 - a02 * a10;
    float b02 = a00 * a13 - a03 * a10;
    float b03 = a01 * a12 - a02 * a11;
    float b04 = a01 * a13 - a03 * a11;
    float b05 = a02 * a13 - a03 * a12;
    float b06 = a20 * a31 - a21 * a30;
    float b07 = a20 * a32 - a22 * a30;
    float b08 = a20 * a33 - a23 * a30;
    float b09 = a21 * a32 - a22 * a31;
    float b10 = a21 * a33 - a23 * a31;
    float b11 = a22 * a33 - a23 * a32;
    // Calculate the invert determinant (inlined to avoid double-caching)
    float invDet = 1.0 / (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06);
    result.M0 = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
    result.M1 = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
    result.M2 = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
    result.M3 = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
    result.M4 = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
    result.M5 = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
    result.M6 = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
    result.M7 = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
    result.M8 = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
    result.M9 = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
    result.M10 = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
    result.M11 = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
    result.M12 = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
    result.M13 = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
    result.M14 = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
    result.M15 = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
    return result;
}

// MatrixIdentity - Returns identity matrix
gfx_Matrix gfx_MatrixIdentity(void) {
    return gfx_NewMatrix(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}

// MatrixNormalize - Normalize provided matrix
gfx_Matrix gfx_MatrixNormalize(gfx_Matrix mat) {
    gfx_Matrix result = {0};
    float det = gfx_MatrixDeterminant(mat);
    result.M0 /= det;
    result.M1 /= det;
    result.M2 /= det;
    result.M3 /= det;
    result.M4 /= det;
    result.M5 /= det;
    result.M6 /= det;
    result.M7 /= det;
    result.M8 /= det;
    result.M9 /= det;
    result.M10 /= det;
    result.M11 /= det;
    result.M12 /= det;
    result.M13 /= det;
    result.M14 /= det;
    result.M15 /= det;
    return result;
}

// MatrixAdd - Add two matrices
gfx_Matrix gfx_MatrixAdd(gfx_Matrix left, gfx_Matrix right) {
    gfx_Matrix result = gfx_MatrixIdentity();
    result.M0 = left.M0 + right.M0;
    result.M1 = left.M1 + right.M1;
    result.M2 = left.M2 + right.M2;
    result.M3 = left.M3 + right.M3;
    result.M4 = left.M4 + right.M4;
    result.M5 = left.M5 + right.M5;
    result.M6 = left.M6 + right.M6;
    result.M7 = left.M7 + right.M7;
    result.M8 = left.M8 + right.M8;
    result.M9 = left.M9 + right.M9;
    result.M10 = left.M10 + right.M10;
    result.M11 = left.M11 + right.M11;
    result.M12 = left.M12 + right.M12;
    result.M13 = left.M13 + right.M13;
    result.M14 = left.M14 + right.M14;
    result.M15 = left.M15 + right.M15;
    return result;
}

// MatrixSubtract - Subtract two matrices (left - right)
gfx_Matrix gfx_MatrixSubtract(gfx_Matrix left, gfx_Matrix right) {
    gfx_Matrix result = gfx_MatrixIdentity();
    result.M0 = left.M0 - right.M0;
    result.M1 = left.M1 - right.M1;
    result.M2 = left.M2 - right.M2;
    result.M3 = left.M3 - right.M3;
    result.M4 = left.M4 - right.M4;
    result.M5 = left.M5 - right.M5;
    result.M6 = left.M6 - right.M6;
    result.M7 = left.M7 - right.M7;
    result.M8 = left.M8 - right.M8;
    result.M9 = left.M9 - right.M9;
    result.M10 = left.M10 - right.M10;
    result.M11 = left.M11 - right.M11;
    result.M12 = left.M12 - right.M12;
    result.M13 = left.M13 - right.M13;
    result.M14 = left.M14 - right.M14;
    result.M15 = left.M15 - right.M15;
    return result;
}

// MatrixMultiply - Returns two matrix multiplication
gfx_Matrix gfx_MatrixMultiply(gfx_Matrix left, gfx_Matrix right) {
    gfx_Matrix result = {0};
    result.M0 = left.M0 * right.M0 + left.M1 * right.M4 + left.M2 * right.M8 + left.M3 * right.M12;
    result.M1 = left.M0 * right.M1 + left.M1 * right.M5 + left.M2 * right.M9 + left.M3 * right.M13;
    result.M2 = left.M0 * right.M2 + left.M1 * right.M6 + left.M2 * right.M10 + left.M3 * right.M14;
    result.M3 = left.M0 * right.M3 + left.M1 * right.M7 + left.M2 * right.M11 + left.M3 * right.M15;
    result.M4 = left.M4 * right.M0 + left.M5 * right.M4 + left.M6 * right.M8 + left.M7 * right.M12;
    result.M5 = left.M4 * right.M1 + left.M5 * right.M5 + left.M6 * right.M9 + left.M7 * right.M13;
    result.M6 = left.M4 * right.M2 + left.M5 * right.M6 + left.M6 * right.M10 + left.M7 * right.M14;
    result.M7 = left.M4 * right.M3 + left.M5 * right.M7 + left.M6 * right.M11 + left.M7 * right.M15;
    result.M8 = left.M8 * right.M0 + left.M9 * right.M4 + left.M10 * right.M8 + left.M11 * right.M12;
    result.M9 = left.M8 * right.M1 + left.M9 * right.M5 + left.M10 * right.M9 + left.M11 * right.M13;
    result.M10 = left.M8 * right.M2 + left.M9 * right.M6 + left.M10 * right.M10 + left.M11 * right.M14;
    result.M11 = left.M8 * right.M3 + left.M9 * right.M7 + left.M10 * right.M11 + left.M11 * right.M15;
    result.M12 = left.M12 * right.M0 + left.M13 * right.M4 + left.M14 * right.M8 + left.M15 * right.M12;
    result.M13 = left.M12 * right.M1 + left.M13 * right.M5 + left.M14 * right.M9 + left.M15 * right.M13;
    result.M14 = left.M12 * right.M2 + left.M13 * right.M6 + left.M14 * right.M10 + left.M15 * right.M14;
    result.M15 = left.M12 * right.M3 + left.M13 * right.M7 + left.M14 * right.M11 + left.M15 * right.M15;
    return result;
}

// MatrixTranslate - Returns translation matrix
gfx_Matrix gfx_MatrixTranslate(float x, float y, float z) {
    return gfx_NewMatrix(1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0, 0, 0, 1.0);
}

// MatrixRotate - Returns rotation matrix for an angle around an specified axis (angle in radians)
gfx_Matrix gfx_MatrixRotate(gfx_Vector3 axis, float angle) {
    gfx_Matrix result = {0};
    gfx_Matrix mat = gfx_MatrixIdentity();
    float x = axis.X;
    float y = axis.Y;
    float z = axis.Z;
    float length = (float)(math_Sqrt((double)(x * x + y * y + z * z)));
    if (length != 1.0 && length != 0.0) {
        length = 1.0 / length;
        x *= length;
        y *= length;
        z *= length;
    }
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    float t = 1.0 - cosres;
    // Cache some matrix values (speed optimization)
    float a00 = mat.M0;
    float a01 = mat.M1;
    float a02 = mat.M2;
    float a03 = mat.M3;
    float a10 = mat.M4;
    float a11 = mat.M5;
    float a12 = mat.M6;
    float a13 = mat.M7;
    float a20 = mat.M8;
    float a21 = mat.M9;
    float a22 = mat.M10;
    float a23 = mat.M11;
    // Construct the elements of the rotation matrix
    float b00 = x * x * t + cosres;
    float b01 = y * x * t + z * sinres;
    float b02 = z * x * t - y * sinres;
    float b10 = x * y * t - z * sinres;
    float b11 = y * y * t + cosres;
    float b12 = z * y * t + x * sinres;
    float b20 = x * z * t + y * sinres;
    float b21 = y * z * t - x * sinres;
    float b22 = z * z * t + cosres;
    // Perform rotation-specific matrix multiplication
    result.M0 = a00 * b00 + a10 * b01 + a20 * b02;
    result.M1 = a01 * b00 + a11 * b01 + a21 * b02;
    result.M2 = a02 * b00 + a12 * b01 + a22 * b02;
    result.M3 = a03 * b00 + a13 * b01 + a23 * b02;
    result.M4 = a00 * b10 + a10 * b11 + a20 * b12;
    result.M5 = a01 * b10 + a11 * b11 + a21 * b12;
    result.M6 = a02 * b10 + a12 * b11 + a22 * b12;
    result.M7 = a03 * b10 + a13 * b11 + a23 * b12;
    result.M8 = a00 * b20 + a10 * b21 + a20 * b22;
    result.M9 = a01 * b20 + a11 * b21 + a21 * b22;
    result.M10 = a02 * b20 + a12 * b21 + a22 * b22;
    result.M11 = a03 * b20 + a13 * b21 + a23 * b22;
    result.M12 = mat.M12;
    result.M13 = mat.M13;
    result.M14 = mat.M14;
    result.M15 = mat.M15;
    return result;
}

// MatrixRotateX - Returns x-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateX(float angle) {
    gfx_Matrix result = gfx_MatrixIdentity();
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    result.M5 = cosres;
    result.M6 = sinres;
    result.M9 = -sinres;
    result.M10 = cosres;
    return result;
}

// MatrixRotateY - Returns y-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateY(float angle) {
    gfx_Matrix result = gfx_MatrixIdentity();
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    result.M0 = cosres;
    result.M2 = -sinres;
    result.M8 = sinres;
    result.M10 = cosres;
    return result;
}

// MatrixRotateZ - Returns z-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateZ(float angle) {
    gfx_Matrix result = gfx_MatrixIdentity();
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    result.M0 = cosres;
    result.M1 = sinres;
    result.M4 = -sinres;
    result.M5 = cosres;
    return result;
}

// MatrixRotateXYZ - Get xyz-rotation matrix (angles in radians)
gfx_Matrix gfx_MatrixRotateXYZ(gfx_Vector3 angle) {
    gfx_Matrix result = gfx_MatrixIdentity();
    so_R_f32_f32 _res1 = gfx_Sincos(-angle.Z);
    float sinz = _res1.val;
    float cosz = _res1.val2;
    so_R_f32_f32 _res2 = gfx_Sincos(-angle.Y);
    float siny = _res2.val;
    float cosy = _res2.val2;
    so_R_f32_f32 _res3 = gfx_Sincos(-angle.X);
    float sinx = _res3.val;
    float cosx = _res3.val2;
    result.M0 = cosz * cosy;
    result.M1 = (cosz * siny * sinx) - (sinz * cosx);
    result.M2 = (cosz * siny * cosx) + (sinz * sinx);
    result.M4 = sinz * cosy;
    result.M5 = (sinz * siny * sinx) + (cosz * cosx);
    result.M6 = (sinz * siny * cosx) - (cosz * sinx);
    result.M8 = -siny;
    result.M9 = cosy * sinx;
    result.M10 = cosy * cosx;
    return result;
}

// MatrixRotateZYX - Get zyx-rotation matrix
// NOTE: Angle must be provided in radians
gfx_Matrix gfx_MatrixRotateZYX(gfx_Vector3 angle) {
    gfx_Matrix result = (gfx_Matrix){};
    so_R_f32_f32 _res1 = gfx_Sincos(angle.Z);
    float sz = _res1.val;
    float cz = _res1.val2;
    so_R_f32_f32 _res2 = gfx_Sincos(angle.Y);
    float sy = _res2.val;
    float cy = _res2.val2;
    so_R_f32_f32 _res3 = gfx_Sincos(angle.X);
    float sx = _res3.val;
    float cx = _res3.val2;
    result.M0 = cz * cy;
    result.M4 = cz * sy * sx - cx * sz;
    result.M8 = sz * sx + cz * cx * sy;
    result.M12 = (float)(0);
    result.M1 = cy * sz;
    result.M5 = cz * cx + sz * sy * sx;
    result.M9 = cx * sz * sy - cz * sx;
    result.M13 = (float)(0);
    result.M2 = -sy;
    result.M6 = cy * sx;
    result.M10 = cy * cx;
    result.M14 = (float)(0);
    result.M3 = (float)(0);
    result.M7 = (float)(0);
    result.M11 = (float)(0);
    result.M15 = (float)(1);
    return result;
}

// MatrixScale - Returns scaling matrix
gfx_Matrix gfx_MatrixScale(float x, float y, float z) {
    gfx_Matrix result = gfx_NewMatrix(x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0);
    return result;
}

// MatrixFrustum - Returns perspective projection matrix
gfx_Matrix gfx_MatrixFrustum(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    gfx_Matrix result = {0};
    float rl = right - left;
    float tb = top - bottom;
    float fn = farPlane - nearPlane;
    result.M0 = (nearPlane * 2.0) / rl;
    result.M1 = 0.0;
    result.M2 = 0.0;
    result.M3 = 0.0;
    result.M4 = 0.0;
    result.M5 = (nearPlane * 2.0) / tb;
    result.M6 = 0.0;
    result.M7 = 0.0;
    result.M8 = (right + left) / rl;
    result.M9 = (top + bottom) / tb;
    result.M10 = -(farPlane + nearPlane) / fn;
    result.M11 = -1.0;
    result.M12 = 0.0;
    result.M13 = 0.0;
    result.M14 = -(farPlane * nearPlane * 2.0) / fn;
    result.M15 = 0.0;
    return result;
}

// MatrixPerspective - Returns perspective projection matrix
// NOTE: Fovy angle must be provided in radians
gfx_Matrix gfx_MatrixPerspective(float fovY, float aspect, float nearPlane, float farPlane) {
    gfx_Matrix result = {0};
    float top = nearPlane * (float)(math_Tan((double)(fovY) * 0.5));
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    // MatrixFrustum(-right, right, -top, top, near, far);
    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(farPlane - nearPlane);
    result.M0 = (nearPlane * 2.0) / rl;
    result.M5 = (nearPlane * 2.0) / tb;
    result.M8 = (right + left) / rl;
    result.M9 = (top + bottom) / tb;
    result.M10 = -(farPlane + nearPlane) / fn;
    result.M11 = -1.0;
    result.M14 = -(farPlane * nearPlane * 2.0) / fn;
    return result;
}

// MatrixOrtho - Returns orthographic projection matrix
gfx_Matrix gfx_MatrixOrtho(float left, float right, float bottom, float top, float near, float far) {
    gfx_Matrix result = {0};
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;
    result.M0 = 2.0 / rl;
    result.M1 = 0.0;
    result.M2 = 0.0;
    result.M3 = 0.0;
    result.M4 = 0.0;
    result.M5 = 2.0 / tb;
    result.M6 = 0.0;
    result.M7 = 0.0;
    result.M8 = 0.0;
    result.M9 = 0.0;
    result.M10 = -2.0 / fn;
    result.M11 = 0.0;
    result.M12 = -(left + right) / rl;
    result.M13 = -(top + bottom) / tb;
    result.M14 = -(far + near) / fn;
    result.M15 = 1.0;
    return result;
}

// MatrixLookAt - Returns camera look-at matrix (view matrix)
gfx_Matrix gfx_MatrixLookAt(gfx_Vector3 eye, gfx_Vector3 target, gfx_Vector3 up) {
    gfx_Matrix result = {0};
    gfx_Vector3 vz = gfx_Vector3Subtract(eye, target);
    vz = gfx_Vector3Normalize(vz);
    gfx_Vector3 vx = gfx_Vector3CrossProduct(up, vz);
    vx = gfx_Vector3Normalize(vx);
    gfx_Vector3 vy = gfx_Vector3CrossProduct(vz, vx);
    result.M0 = vx.X;
    result.M1 = vy.X;
    result.M2 = vz.X;
    result.M3 = 0;
    result.M4 = vx.Y;
    result.M5 = vy.Y;
    result.M6 = vz.Y;
    result.M7 = 0;
    result.M8 = vx.Z;
    result.M9 = vy.Z;
    result.M10 = vz.Z;
    result.M11 = 0;
    result.M12 = -gfx_Vector3DotProduct(vx, eye);
    result.M13 = -gfx_Vector3DotProduct(vy, eye);
    result.M14 = -gfx_Vector3DotProduct(vz, eye);
    result.M15 = 1;
    return result;
}

// MatrixToFloat - Get float array of matrix data
gfx_Float16 gfx_MatrixToFloat(gfx_Matrix mat) {
    gfx_Float16 result = {0};
    result.V[0] = mat.M0;
    result.V[1] = mat.M1;
    result.V[2] = mat.M2;
    result.V[3] = mat.M3;
    result.V[4] = mat.M4;
    result.V[5] = mat.M5;
    result.V[6] = mat.M6;
    result.V[7] = mat.M7;
    result.V[8] = mat.M8;
    result.V[9] = mat.M9;
    result.V[10] = mat.M10;
    result.V[11] = mat.M11;
    result.V[12] = mat.M12;
    result.V[13] = mat.M13;
    result.V[14] = mat.M14;
    result.V[15] = mat.M15;
    return result;
}

// QuaternionAdd - Add two quaternions
gfx_Vector4 gfx_QuaternionAdd(gfx_Vector4 q1, gfx_Vector4 q2) {
    gfx_Vector4 result = (gfx_Vector4){.X = q1.X + q2.X, .Y = q1.Y + q2.Y, .Z = q1.Z + q2.Z, .W = q1.W + q2.W};
    return result;
}

// QuaternionAddValue - Add quaternion and float value
gfx_Vector4 gfx_QuaternionAddValue(gfx_Vector4 q, float add) {
    gfx_Vector4 result = (gfx_Vector4){.X = q.X + add, .Y = q.Y + add, .Z = q.Z + add, .W = q.W + add};
    return result;
}

// QuaternionSubtract - Subtract two quaternions
gfx_Vector4 gfx_QuaternionSubtract(gfx_Vector4 q1, gfx_Vector4 q2) {
    gfx_Vector4 result = (gfx_Vector4){.X = q1.X - q2.X, .Y = q1.Y - q2.Y, .Z = q1.Z - q2.Z, .W = q1.W - q2.W};
    return result;
}

// QuaternionSubtractValue - Subtract quaternion and float value
gfx_Vector4 gfx_QuaternionSubtractValue(gfx_Vector4 q, float sub) {
    gfx_Vector4 result = (gfx_Vector4){.X = q.X - sub, .Y = q.Y - sub, .Z = q.Z - sub, .W = q.W - sub};
    return result;
}

// QuaternionIdentity - Get identity quaternion
gfx_Vector4 gfx_QuaternionIdentity(void) {
    gfx_Vector4 result = (gfx_Vector4){.W = 1.0};
    return result;
}

// QuaternionLength - Compute the length of a quaternion
float gfx_QuaternionLength(gfx_Vector4 quat) {
    return (float)(math_Sqrt((double)(quat.X * quat.X + quat.Y * quat.Y + quat.Z * quat.Z + quat.W * quat.W)));
}

// QuaternionNormalize - Normalize provided quaternion
gfx_Vector4 gfx_QuaternionNormalize(gfx_Vector4 q) {
    gfx_Vector4 result = q;
    float length = gfx_QuaternionLength(q);
    if (length != 0.0) {
        result.X /= length;
        result.Y /= length;
        result.Z /= length;
        result.W /= length;
    }
    return result;
}

// QuaternionInvert - Invert provided quaternion
gfx_Vector4 gfx_QuaternionInvert(gfx_Vector4 quat) {
    gfx_Vector4 result = quat;
    float length = gfx_QuaternionLength(quat);
    float lengthSq = length * length;
    if (lengthSq != 0.0) {
        float i = 1.0 / lengthSq;
        result.X *= -i;
        result.Y *= -i;
        result.Z *= -i;
        result.W *= i;
    }
    return result;
}

// QuaternionMultiply - Calculate two quaternion multiplication
gfx_Vector4 gfx_QuaternionMultiply(gfx_Vector4 q1, gfx_Vector4 q2) {
    gfx_Vector4 result = {0};
    float qax = q1.X;
    float qay = q1.Y;
    float qaz = q1.Z;
    float qaw = q1.W;
    float qbx = q2.X;
    float qby = q2.Y;
    float qbz = q2.Z;
    float qbw = q2.W;
    result.X = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
    result.Y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
    result.Z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
    result.W = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;
    return result;
}

// QuaternionScale - Scale quaternion by float value
gfx_Vector4 gfx_QuaternionScale(gfx_Vector4 q, float mul) {
    gfx_Vector4 result = (gfx_Vector4){};
    result.X = q.X * mul;
    result.Y = q.Y * mul;
    result.Z = q.Z * mul;
    result.W = q.W * mul;
    return result;
}

// QuaternionDivide - Divide two quaternions
gfx_Vector4 gfx_QuaternionDivide(gfx_Vector4 q1, gfx_Vector4 q2) {
    gfx_Vector4 result = (gfx_Vector4){.X = q1.X / q2.X, .Y = q1.Y / q2.Y, .Z = q1.Z / q2.Z, .W = q1.W / q2.W};
    return result;
}

// QuaternionLerp - Calculate linear interpolation between two quaternions
gfx_Vector4 gfx_QuaternionLerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount) {
    gfx_Vector4 result = (gfx_Vector4){};
    result.X = q1.X + amount * (q2.X - q1.X);
    result.Y = q1.Y + amount * (q2.Y - q1.Y);
    result.Z = q1.Z + amount * (q2.Z - q1.Z);
    result.W = q1.W + amount * (q2.W - q1.W);
    return result;
}

// QuaternionNlerp - Calculate slerp-optimized interpolation between two quaternions
gfx_Vector4 gfx_QuaternionNlerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount) {
    gfx_Vector4 result = (gfx_Vector4){};
    // QuaternionLerp(q1, q2, amount)
    result.X = q1.X + amount * (q2.X - q1.X);
    result.Y = q1.Y + amount * (q2.Y - q1.Y);
    result.Z = q1.Z + amount * (q2.Z - q1.Z);
    result.W = q1.W + amount * (q2.W - q1.W);
    // QuaternionNormalize(r);
    gfx_Vector4 r = result;
    float length = (float)(math_Sqrt((double)(r.X * r.X + r.Y * r.Y + r.Z * r.Z + r.W * r.W)));
    if (length == 0.0) {
        length = 1.0;
    }
    float ilength = 1.0 / length;
    result.X = r.X * ilength;
    result.Y = r.Y * ilength;
    result.Z = r.Z * ilength;
    result.W = r.W * ilength;
    return result;
}

// QuaternionSlerp - Calculates spherical linear interpolation between two quaternions
gfx_Vector4 gfx_QuaternionSlerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount) {
    float cosHalfTheta = q1.X * q2.X + q1.Y * q2.Y + q1.Z * q2.Z + q1.W * q2.W;
    if (cosHalfTheta < 0) {
        q2.X = -q2.X;
        q2.Y = -q2.Y;
        q2.Z = -q2.Z;
        q2.W = -q2.W;
        cosHalfTheta = -cosHalfTheta;
    }
    if (math_Abs((double)(cosHalfTheta)) >= 1.0) {
        return q1;
    }
    if (cosHalfTheta > 0.95) {
        return gfx_QuaternionNlerp(q1, q2, amount);
    }
    gfx_Vector4 result = {0};
    float halfTheta = (float)(math_Acos((double)(cosHalfTheta)));
    float sinHalfTheta = (float)(math_Sqrt((double)(1.0 - cosHalfTheta * cosHalfTheta)));
    if (math_Abs((double)(sinHalfTheta)) < epsilon) {
        result.X = (q1.X * 0.5 + q2.X * 0.5);
        result.Y = (q1.Y * 0.5 + q2.Y * 0.5);
        result.Z = (q1.Z * 0.5 + q2.Z * 0.5);
        result.W = (q1.W * 0.5 + q2.W * 0.5);
    } else {
        float ratioA = (float)(math_Sin((double)((1 - amount) * halfTheta))) / sinHalfTheta;
        float ratioB = (float)(math_Sin((double)(amount * halfTheta))) / sinHalfTheta;
        result.X = (q1.X * ratioA + q2.X * ratioB);
        result.Y = (q1.Y * ratioA + q2.Y * ratioB);
        result.Z = (q1.Z * ratioA + q2.Z * ratioB);
        result.W = (q1.W * ratioA + q2.W * ratioB);
    }
    return result;
}

// QuaternionFromVector3ToVector3 - Calculate quaternion based on the rotation from one vector to another
gfx_Vector4 gfx_QuaternionFromVector3ToVector3(gfx_Vector3 from, gfx_Vector3 to) {
    gfx_Vector4 result = (gfx_Vector4){};
    // Vector3DotProduct(from, to)
    float cos2Theta = from.X * to.X + from.Y * to.Y + from.Z * to.Z;
    // Vector3CrossProduct(from, to)
    gfx_Vector3 cross = (gfx_Vector3){.X = from.Y * to.Z - from.Z * to.Y, .Y = from.Z * to.X - from.X * to.Z, .Z = from.X * to.Y - from.Y * to.X};
    result.X = cross.X;
    result.Y = cross.Y;
    result.Z = cross.Z;
    result.W = 1.0 + cos2Theta;
    // QuaternionNormalize(q);
    // NOTE: Normalize to essentially nlerp the original and identity to 0.5
    gfx_Vector4 q = result;
    float length = (float)(math_Sqrt((double)(q.X * q.X + q.Y * q.Y + q.Z * q.Z + q.W * q.W)));
    if (length == 0.0) {
        length = 1.0;
    }
    float ilength = 1.0 / length;
    result.X = q.X * ilength;
    result.Y = q.Y * ilength;
    result.Z = q.Z * ilength;
    result.W = q.W * ilength;
    return result;
}

// QuaternionFromMatrix - Returns a quaternion for a given rotation matrix
gfx_Vector4 gfx_QuaternionFromMatrix(gfx_Matrix mat) {
    gfx_Vector4 result = {0};
    float fourWSquaredMinus1 = mat.M0 + mat.M5 + mat.M10;
    float fourXSquaredMinus1 = mat.M0 - mat.M5 - mat.M10;
    float fourYSquaredMinus1 = mat.M5 - mat.M0 - mat.M10;
    float fourZSquaredMinus1 = mat.M10 - mat.M0 - mat.M5;
    so_int biggestIndex = 0;
    float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if (fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if (fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if (fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }
    float biggestVal = (float)(math_Sqrt((double)(fourBiggestSquaredMinus1) + 1.0) * 0.5);
    float mult = 0.25 / biggestVal;
    if (biggestIndex == (0)) {
        result.W = biggestVal;
        result.X = (mat.M6 - mat.M9) * mult;
        result.Y = (mat.M8 - mat.M2) * mult;
        result.Z = (mat.M1 - mat.M4) * mult;
    } else if (biggestIndex == (1)) {
        result.X = biggestVal;
        result.W = (mat.M6 - mat.M9) * mult;
        result.Y = (mat.M1 + mat.M4) * mult;
        result.Z = (mat.M8 + mat.M2) * mult;
    } else if (biggestIndex == (2)) {
        result.Y = biggestVal;
        result.W = (mat.M8 - mat.M2) * mult;
        result.X = (mat.M1 + mat.M4) * mult;
        result.Z = (mat.M6 + mat.M9) * mult;
    } else if (biggestIndex == (3)) {
        result.Z = biggestVal;
        result.W = (mat.M1 - mat.M4) * mult;
        result.X = (mat.M8 + mat.M2) * mult;
        result.Y = (mat.M6 + mat.M9) * mult;
    }
    return result;
}

// QuaternionToMatrix - Returns a matrix for a given quaternion
gfx_Matrix gfx_QuaternionToMatrix(gfx_Vector4 q) {
    gfx_Matrix result = gfx_MatrixIdentity();
    float a2 = q.X * q.X;
    float b2 = q.Y * q.Y;
    float c2 = q.Z * q.Z;
    float ac = q.X * q.Z;
    float ab = q.X * q.Y;
    float bc = q.Y * q.Z;
    float ad = q.W * q.X;
    float bd = q.W * q.Y;
    float cd = q.W * q.Z;
    result.M0 = 1 - 2 * (b2 + c2);
    result.M1 = 2 * (ab + cd);
    result.M2 = 2 * (ac - bd);
    result.M4 = 2 * (ab - cd);
    result.M5 = 1 - 2 * (a2 + c2);
    result.M6 = 2 * (bc + ad);
    result.M8 = 2 * (ac + bd);
    result.M9 = 2 * (bc - ad);
    result.M10 = 1 - 2 * (a2 + b2);
    return result;
}

// QuaternionFromAxisAngle - Returns rotation quaternion for an angle and axis
gfx_Vector4 gfx_QuaternionFromAxisAngle(gfx_Vector3 axis, float angle) {
    gfx_Vector4 result = gfx_NewQuaternion(0.0, 0.0, 0.0, 1.0);
    if (gfx_Vector3Length(axis) != 0.0) {
        angle *= 0.5;
    }
    axis = gfx_Vector3Normalize(axis);
    so_R_f32_f32 _res1 = gfx_Sincos(angle);
    float sinres = _res1.val;
    float cosres = _res1.val2;
    result.X = axis.X * sinres;
    result.Y = axis.Y * sinres;
    result.Z = axis.Z * sinres;
    result.W = cosres;
    result = gfx_QuaternionNormalize(result);
    return result;
}

// QuaternionToAxisAngle - Returns the rotation angle and axis for a given quaternion
void gfx_QuaternionToAxisAngle(gfx_Vector4 q, gfx_Vector3* outAxis, float* outAngle) {
    if (math_Abs((double)(q.W)) > 1.0) {
        q = gfx_QuaternionNormalize(q);
    }
    gfx_Vector3 resAxis = gfx_NewVector3(0.0, 0.0, 0.0);
    float resAngle = 2.0 * (float)(math_Acos((double)(q.W)));
    float den = (float)(math_Sqrt((double)(1.0 - q.W * q.W)));
    if (den > epsilon) {
        resAxis.X = q.X / den;
        resAxis.Y = q.Y / den;
        resAxis.Z = q.Z / den;
    } else {
        // This occurs when the angle is zero.
        // Not a problem: just set an arbitrary normalized axis.
        resAxis.X = 1.0;
    }
    *outAxis = resAxis;
    *outAngle = resAngle;
}

// QuaternionFromEuler - Get the quaternion equivalent to Euler angles
// NOTE: Rotation order is ZYX
gfx_Vector4 gfx_QuaternionFromEuler(float pitch, float yaw, float roll) {
    gfx_Vector4 result = {0};
    so_R_f32_f32 _res1 = gfx_Sincos(pitch * 0.5);
    float x1 = _res1.val;
    float x0 = _res1.val2;
    so_R_f32_f32 _res2 = gfx_Sincos(yaw * 0.5);
    float y1 = _res2.val;
    float y0 = _res2.val2;
    so_R_f32_f32 _res3 = gfx_Sincos(roll * 0.5);
    float z1 = _res3.val;
    float z0 = _res3.val2;
    result.X = x1 * y0 * z0 - x0 * y1 * z1;
    result.Y = x0 * y1 * z0 + x1 * y0 * z1;
    result.Z = x0 * y0 * z1 - x1 * y1 * z0;
    result.W = x0 * y0 * z0 + x1 * y1 * z1;
    return result;
}

// QuaternionToEuler - Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
gfx_Vector3 gfx_QuaternionToEuler(gfx_Vector4 q) {
    gfx_Vector3 result = {0};
    // Roll (x-axis rotation)
    float x0 = 2.0 * (q.W * q.X + q.Y * q.Z);
    float x1 = 1.0 - 2.0 * (q.X * q.X + q.Y * q.Y);
    result.X = (float)(math_Atan2((double)(x0), (double)(x1)));
    // Pitch (y-axis rotation)
    float y0 = 2.0 * (q.W * q.Y - q.Z * q.X);
    y0 = gfx_Clamp(y0, -1.0, 1.0);
    result.Y = (float)(math_Asin((double)(y0)));
    // Yaw (z-axis rotation)
    float z0 = 2.0 * (q.W * q.Z + q.X * q.Y);
    float z1 = 1.0 - 2.0 * (q.Y * q.Y + q.Z * q.Z);
    result.Z = (float)(math_Atan2((double)(z0), (double)(z1)));
    return result;
}

// QuaternionTransform - Transform a quaternion given a transformation matrix
gfx_Vector4 gfx_QuaternionTransform(gfx_Vector4 q, gfx_Matrix mat) {
    gfx_Vector4 result = {0};
    float x = q.X;
    float y = q.Y;
    float z = q.Z;
    float w = q.W;
    result.X = mat.M0 * x + mat.M4 * y + mat.M8 * z + mat.M12 * w;
    result.Y = mat.M1 * x + mat.M5 * y + mat.M9 * z + mat.M13 * w;
    result.Z = mat.M2 * x + mat.M6 * y + mat.M10 * z + mat.M14 * w;
    result.W = mat.M3 * x + mat.M7 * y + mat.M11 * z + mat.M15 * w;
    return result;
}

// QuaternionEquals - Check whether two given quaternions are almost equal
bool gfx_QuaternionEquals(gfx_Vector4 q, gfx_Vector4 p) {
    return (math_Abs((double)(q.X - p.X)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.X)), math_Abs((double)(p.X)))) && math_Abs((double)(q.Y - p.Y)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.Y)), math_Abs((double)(p.Y)))) && math_Abs((double)(q.Z - p.Z)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.Z)), math_Abs((double)(p.Z)))) && math_Abs((double)(q.W - p.W)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.W)), math_Abs((double)(p.W)))) || math_Abs((double)(q.X + p.X)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.X)), math_Abs((double)(p.X)))) && math_Abs((double)(q.Y + p.Y)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.Y)), math_Abs((double)(p.Y)))) && math_Abs((double)(q.Z + p.Z)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.Z)), math_Abs((double)(p.Z)))) && math_Abs((double)(q.W + p.W)) <= epsilon * math_Max(1.0, math_Max(math_Abs((double)(q.W)), math_Abs((double)(p.W)))));
}

// MatrixDecompose - Decompose a transformation matrix into its rotational, translational and scaling components
void gfx_MatrixDecompose(gfx_Matrix mat, gfx_Vector3* translation, gfx_Vector4* rotation, gfx_Vector3* scale) {
    // Extract translation.
    translation->X = mat.M12;
    translation->Y = mat.M13;
    translation->Z = mat.M14;
    // Extract upper-left for determinant computation
    float a = mat.M0;
    float b = mat.M4;
    float c = mat.M8;
    float d = mat.M1;
    float e = mat.M5;
    float f = mat.M9;
    float g = mat.M2;
    float h = mat.M6;
    float i = mat.M10;
    float A = e * i - f * h;
    float B = f * g - d * i;
    float C = d * h - e * g;
    // Extract scale
    float det = a * A + b * B + c * C;
    gfx_Vector3 abc = gfx_NewVector3(a, b, c);
    gfx_Vector3 def = gfx_NewVector3(d, e, f);
    gfx_Vector3 ghi = gfx_NewVector3(g, h, i);
    float scalex = gfx_Vector3Length(abc);
    float scaley = gfx_Vector3Length(def);
    float scalez = gfx_Vector3Length(ghi);
    gfx_Vector3 s = gfx_NewVector3(scalex, scaley, scalez);
    if (det < 0) {
        s = gfx_Vector3Negate(s);
    }
    *scale = s;
    // Remove scale from the matrix if it is not close to zero
    gfx_Matrix clone = mat;
    if (!gfx_FloatEquals(det, 0)) {
        clone.M0 /= s.X;
        clone.M5 /= s.Y;
        clone.M10 /= s.Z;
        // Extract rotation
        *rotation = gfx_QuaternionFromMatrix(clone);
    } else {
        // Set to identity if close to zero
        *rotation = gfx_QuaternionIdentity();
    }
}

so_R_f32_f32 gfx_Sincos(float angle) {
    double sind = math_Sin((double)(angle));
    double cosd = math_Cos((double)(angle));
    return (so_R_f32_f32){.val = (float)(sind), .val2 = (float)(cosd)};
}

// -- math_methods.go --

// anchor this rectangle's position inside parent rectangle.
// Returns new position
gfx_Rectangle gfx_Rectangle_Anchor(gfx_Rectangle r, gfx_Rectangle parent, float anchorX, float anchorY) {
    return gfx_Rectangle_SetPosition(r, gfx_NewVector2(parent.X + (parent.W - r.W) * anchorX, parent.Y + (parent.H - r.H) * anchorY));
}

gfx_Rectangle gfx_Rectangle_Multiply(gfx_Rectangle r, float v) {
    r.W *= v;
    r.H *= v;
    r.X *= v;
    r.Y *= v;
    return r;
}

// Scale size
gfx_Rectangle gfx_Rectangle_Scale(gfx_Rectangle r, float v) {
    r.W *= v;
    r.H *= v;
    return r;
}

// AddPosition position by adding it
gfx_Rectangle gfx_Rectangle_AddPosition(gfx_Rectangle r, gfx_Vector2 v) {
    return gfx_Rectangle_SetPosition(r, gfx_Vector2_Add(gfx_Rectangle_Position(r), v));
}

gfx_Rectangle gfx_Rectangle_SubractPosition(gfx_Rectangle r, gfx_Vector2 v) {
    return gfx_Rectangle_SetPosition(r, gfx_Vector2_Subtract(gfx_Rectangle_Position(r), v));
}

gfx_Rectangle gfx_Rectangle_SetPosition(gfx_Rectangle r, gfx_Vector2 v) {
    r.X = v.X;
    r.Y = v.Y;
    return r;
}

// Grow equally on all sides (anchored to center).
gfx_Rectangle gfx_Rectangle_Grow(gfx_Rectangle r, float v) {
    r.X -= v;
    r.Y -= v;
    r.W += 2 * v;
    r.H += 2 * v;
    return r;
}

// Shrink equally on all sides (anchored to center).
gfx_Rectangle gfx_Rectangle_Shrink(gfx_Rectangle r, float v) {
    r.X += v;
    r.Y += v;
    r.W -= 2 * v;
    r.H -= 2 * v;
    return r;
}

gfx_Rectangle gfx_Rectangle_SetSize(gfx_Rectangle r, gfx_Vector2 v) {
    r.W = v.X;
    r.H = v.Y;
    return r;
}

gfx_Vector2 gfx_Rectangle_Position(gfx_Rectangle r) {
    return (gfx_Vector2){.X = r.X, .Y = r.Y};
}

gfx_Vector2 gfx_Rectangle_Size(gfx_Rectangle r) {
    return (gfx_Vector2){.X = r.W, .Y = r.H};
}

bool gfx_Rectangle_Contains(gfx_Rectangle r, gfx_Vector2 p) {
    return p.X >= r.X && p.X <= r.X + r.W && p.Y >= r.Y && p.Y <= r.Y + r.H;
}

// MultiplyVector2 - Multiplies a vector by a matrix 2x2
gfx_Vector2 gfx_Mat2_MultiplyVector2(gfx_Mat2 m, gfx_Vector2 vector) {
    return gfx_Mat2MultiplyVector2(m, vector);
}

// Transpose - Returns the transpose of a given matrix 2x2
gfx_Mat2 gfx_Mat2_Transpose(gfx_Mat2 m) {
    return gfx_Mat2Transpose(m);
}

// Add - Add two matrices
gfx_Matrix gfx_Matrix_Add(gfx_Matrix m, gfx_Matrix right) {
    return gfx_MatrixAdd(m, right);
}

// Decompose - Decompose a transformation matrix into its rotational, translational and scaling components
void gfx_Matrix_Decompose(gfx_Matrix m, gfx_Vector3* translation, gfx_Vector4* rotation, gfx_Vector3* scale) {
    gfx_MatrixDecompose(m, translation, rotation, scale);
}

// Determinant - Compute matrix determinant
float gfx_Matrix_Determinant(gfx_Matrix m) {
    return gfx_MatrixDeterminant(m);
}

// Invert - Invert provided matrix
gfx_Matrix gfx_Matrix_Invert(gfx_Matrix m) {
    return gfx_MatrixInvert(m);
}

// Multiply - Returns two matrix multiplication
gfx_Matrix gfx_Matrix_Multiply(gfx_Matrix m, gfx_Matrix right) {
    return gfx_MatrixMultiply(m, right);
}

// Normalize - Normalize provided matrix
gfx_Matrix gfx_Matrix_Normalize(gfx_Matrix m) {
    return gfx_MatrixNormalize(m);
}

// Subtract - Subtract two matrices (left - right)
gfx_Matrix gfx_Matrix_Subtract(gfx_Matrix m, gfx_Matrix right) {
    return gfx_MatrixSubtract(m, right);
}

// ToFloatV - Get float array of matrix data
gfx_Float16 gfx_Matrix_ToFloat(gfx_Matrix m) {
    return gfx_MatrixToFloat(m);
}

// Trace - Returns the trace of the matrix (sum of the values along the diagonal)
float gfx_Matrix_Trace(gfx_Matrix m) {
    return gfx_MatrixTrace(m);
}

// Transpose - Transposes provided matrix
gfx_Matrix gfx_Matrix_Transpose(gfx_Matrix m) {
    return gfx_MatrixTranspose(m);
}

// Add - Add two quaternions
gfx_Vector4 gfx_Vector4_Add(gfx_Vector4 q, gfx_Vector4 q2) {
    return gfx_QuaternionAdd(q, q2);
}

// AddValue - Add quaternion and float value
gfx_Vector4 gfx_Vector4_AddValue(gfx_Vector4 q, float add) {
    return gfx_QuaternionAddValue(q, add);
}

// Divide - Divide two quaternions
gfx_Vector4 gfx_Vector4_Divide(gfx_Vector4 q, gfx_Vector4 q2) {
    return gfx_QuaternionDivide(q, q2);
}

// Equals - Check whether two given quaternions are almost equal
bool gfx_Vector4_Equals(gfx_Vector4 q, gfx_Vector4 p) {
    return gfx_QuaternionEquals(q, p);
}

// Invert - Invert provided quaternion
gfx_Vector4 gfx_Vector4_Invert(gfx_Vector4 q) {
    return gfx_QuaternionInvert(q);
}

// Length - Compute the length of a quaternion
float gfx_Vector4_Length(gfx_Vector4 q) {
    return gfx_QuaternionLength(q);
}

// Lerp - Calculate linear interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Lerp(gfx_Vector4 q, gfx_Vector4 q2, float amount) {
    return gfx_QuaternionLerp(q, q2, amount);
}

// Multiply - Calculate two quaternion multiplication
gfx_Vector4 gfx_Vector4_Multiply(gfx_Vector4 q, gfx_Vector4 q2) {
    return gfx_QuaternionMultiply(q, q2);
}

// Nlerp - Calculate slerp-optimized interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Nlerp(gfx_Vector4 q, gfx_Vector4 q2, float amount) {
    return gfx_QuaternionNlerp(q, q2, amount);
}

// Normalize - Normalize provided quaternion
gfx_Vector4 gfx_Vector4_Normalize(gfx_Vector4 q) {
    return gfx_QuaternionNormalize(q);
}

// Scale - Scale quaternion by float value
gfx_Vector4 gfx_Vector4_Scale(gfx_Vector4 q, float mul) {
    return gfx_QuaternionScale(q, mul);
}

// Slerp - Calculates spherical linear interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Slerp(gfx_Vector4 q, gfx_Vector4 q2, float amount) {
    return gfx_QuaternionSlerp(q, q2, amount);
}

// Subtract - Subtract two quaternions
gfx_Vector4 gfx_Vector4_Subtract(gfx_Vector4 q, gfx_Vector4 q2) {
    return gfx_QuaternionSubtract(q, q2);
}

// SubtractValue - Subtract quaternion and float value
gfx_Vector4 gfx_Vector4_SubtractValue(gfx_Vector4 q, float sub) {
    return gfx_QuaternionSubtractValue(q, sub);
}

// ToAxisAngle - Returns the rotation angle and axis for a given quaternion
void gfx_Vector4_ToAxisAngle(gfx_Vector4 q, gfx_Vector3* outAxis, float* outAngle) {
    gfx_QuaternionToAxisAngle(q, outAxis, outAngle);
}

// ToEuler - Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
gfx_Vector3 gfx_Vector4_ToEuler(gfx_Vector4 q) {
    return gfx_QuaternionToEuler(q);
}

// ToMatrix - Returns a matrix for a given quaternion
gfx_Matrix gfx_Vector4_ToMatrix(gfx_Vector4 q) {
    return gfx_QuaternionToMatrix(q);
}

// Transform - Transform a quaternion given a transformation matrix
gfx_Vector4 gfx_Vector4_Transform(gfx_Vector4 q, gfx_Matrix mat) {
    return gfx_QuaternionTransform(q, mat);
}

// Add - Add two vectors (v1 + v2)
gfx_Vector2 gfx_Vector2_Add(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Add(v, v2);
}

// AddValue - Add vector and float value
gfx_Vector2 gfx_Vector2_AddValue(gfx_Vector2 v, float add) {
    return gfx_Vector2AddValue(v, add);
}

// Angle - Calculate angle from two vectors in radians
// NOTE: Coordinate system convention: positive X right, positive Y down,
// positive angles appear clockwise, and negative angles appear counterclockwise
float gfx_Vector2_Angle(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Angle(v, v2);
}

// Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector2 gfx_Vector2_Clamp(gfx_Vector2 v, gfx_Vector2 min, gfx_Vector2 max) {
    return gfx_Vector2Clamp(v, min, max);
}

// ClampValue - Clamp the magnitude of the vector between two min and max values
gfx_Vector2 gfx_Vector2_ClampValue(gfx_Vector2 v, float min, float max) {
    return gfx_Vector2ClampValue(v, min, max);
}

// CrossProduct - Calculate two vectors cross product
float gfx_Vector2_CrossProduct(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2CrossProduct(v, v2);
}

// Distance - Calculate distance between two vectors
float gfx_Vector2_Distance(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Distance(v, v2);
}

// Intersect - Calculate square distance between two vectors
bool gfx_Vector2_Intersect(gfx_Vector2 v, gfx_Vector2 v2, float radius) {
    return gfx_Vector2DistanceSqr(v, v2) <= radius;
}

// Divide - Divide vector by vector
gfx_Vector2 gfx_Vector2_Divide(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Divide(v, v2);
}

// DotProduct - Calculate two vectors dot product
float gfx_Vector2_DotProduct(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2DotProduct(v, v2);
}

// Equals - Check whether two given vectors are almost equal
bool gfx_Vector2_Equals(gfx_Vector2 v, gfx_Vector2 q) {
    return gfx_Vector2Equals(v, q);
}

// Invert - Invert the given vector
gfx_Vector2 gfx_Vector2_Invert(gfx_Vector2 v) {
    return gfx_Vector2Invert(v);
}

// Length - Calculate vector length
float gfx_Vector2_Length(gfx_Vector2 v) {
    return gfx_Vector2Length(v);
}

// LengthSqr - Calculate vector square length
float gfx_Vector2_LengthSqr(gfx_Vector2 v) {
    return gfx_Vector2LengthSqr(v);
}

// Lerp - Calculate linear interpolation between two vectors
gfx_Vector2 gfx_Vector2_Lerp(gfx_Vector2 v, gfx_Vector2 v2, float amount) {
    return gfx_Vector2Lerp(v, v2, amount);
}

// LineAngle - Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized. Current implementation should be aligned with glm::angle
float gfx_Vector2_LineAngle(gfx_Vector2 v, gfx_Vector2 end) {
    return gfx_Vector2LineAngle(v, end);
}

// MoveTowards - Move Vector towards target
gfx_Vector2 gfx_Vector2_MoveTowards(gfx_Vector2 v, gfx_Vector2 target, float maxDistance) {
    return gfx_Vector2MoveTowards(v, target, maxDistance);
}

// Multiply - Multiply vector by vector
gfx_Vector2 gfx_Vector2_Multiply(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Multiply(v, v2);
}

// Negate - Negate vector
gfx_Vector2 gfx_Vector2_Negate(gfx_Vector2 v) {
    return gfx_Vector2Negate(v);
}

// Normalize - Normalize provided vector
gfx_Vector2 gfx_Vector2_Normalize(gfx_Vector2 v) {
    return gfx_Vector2Normalize(v);
}

// Reflect - Calculate reflected vector to normal
gfx_Vector2 gfx_Vector2_Reflect(gfx_Vector2 v, gfx_Vector2 normal) {
    return gfx_Vector2Reflect(v, normal);
}

// Rotate - Rotate vector by angle
gfx_Vector2 gfx_Vector2_Rotate(gfx_Vector2 v, float angle) {
    return gfx_Vector2Rotate(v, angle);
}

gfx_Vector2 gfx_Vector2_RotateAroundPivot(gfx_Vector2 v, gfx_Vector2 pivot, float angle) {
    return gfx_Vector2_Add(pivot, gfx_Vector2_Rotate(gfx_Vector2_Subtract(v, pivot), angle));
}

gfx_Vector2 gfx_Vector2_Half(gfx_Vector2 v) {
    return gfx_Vector2_Scale(v, .5);
}

// Scale - Scale vector (multiply by value)
gfx_Vector2 gfx_Vector2_Scale(gfx_Vector2 v, float scale) {
    return gfx_Vector2Scale(v, scale);
}

// Subtract - Subtract two vectors (v1 - v2)
gfx_Vector2 gfx_Vector2_Subtract(gfx_Vector2 v, gfx_Vector2 v2) {
    return gfx_Vector2Subtract(v, v2);
}

// SubtractValue - Subtract vector by float value
gfx_Vector2 gfx_Vector2_SubtractValue(gfx_Vector2 v, float sub) {
    return gfx_Vector2SubtractValue(v, sub);
}

// Transform - Transforms a Vector2 by a given Matrix
gfx_Vector2 gfx_Vector2_Transform(gfx_Vector2 v, gfx_Matrix mat) {
    return gfx_Vector2Transform(v, mat);
}

// Transform - Transforms a Vector2 by a given Matrix
so_R_f32_f32 gfx_Vector2_XY(gfx_Vector2 v) {
    return (so_R_f32_f32){.val = v.X, .val2 = v.Y};
}

// Add - Add two vectors
gfx_Vector3 gfx_Vector3_Add(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Add(v, v2);
}

// AddValue - Add vector and float value
gfx_Vector3 gfx_Vector3_AddValue(gfx_Vector3 v, float add) {
    return gfx_Vector3AddValue(v, add);
}

// Angle - Calculate angle between two vectors
float gfx_Vector3_Angle(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Angle(v, v2);
}

// Barycenter - Barycenter coords for p in triangle abc
gfx_Vector3 gfx_Vector3_Barycenter(gfx_Vector3 v, gfx_Vector3 a, gfx_Vector3 b, gfx_Vector3 c) {
    return gfx_Vector3Barycenter(v, a, b, c);
}

// Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector3 gfx_Vector3_Clamp(gfx_Vector3 v, gfx_Vector3 min, gfx_Vector3 max) {
    return gfx_Vector3Clamp(v, min, max);
}

// ClampValue - Clamp the magnitude of the vector between two values
gfx_Vector3 gfx_Vector3_ClampValue(gfx_Vector3 v, float min, float max) {
    return gfx_Vector3ClampValue(v, min, max);
}

// CrossProduct - Calculate two vectors cross product
gfx_Vector3 gfx_Vector3_CrossProduct(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3CrossProduct(v, v2);
}

// Distance - Calculate distance between two vectors
float gfx_Vector3_Distance(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Distance(v, v2);
}

// DistanceSqr - Calculate square distance between two vectors
float gfx_Vector3_DistanceSqr(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3DistanceSqr(v, v2);
}

// Divide - Divide vector by vector
gfx_Vector3 gfx_Vector3_Divide(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Divide(v, v2);
}

// DotProduct - Calculate two vectors dot product
float gfx_Vector3_DotProduct(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3DotProduct(v, v2);
}

// Equals - Check whether two given vectors are almost equal
bool gfx_Vector3_Equals(gfx_Vector3 v, gfx_Vector3 q) {
    return gfx_Vector3Equals(v, q);
}

// Invert - Invert the given vector
gfx_Vector3 gfx_Vector3_Invert(gfx_Vector3 v) {
    return gfx_Vector3Invert(v);
}

// Length - Calculate vector length
float gfx_Vector3_Length(gfx_Vector3 v) {
    return gfx_Vector3Length(v);
}

// LengthSqr - Calculate vector square length
float gfx_Vector3_LengthSqr(gfx_Vector3 v) {
    return gfx_Vector3LengthSqr(v);
}

// Lerp - Calculate linear interpolation between two vectors
gfx_Vector3 gfx_Vector3_Lerp(gfx_Vector3 v, gfx_Vector3 v2, float amount) {
    return gfx_Vector3Lerp(v, v2, amount);
}

// Max - Return max value for each pair of components
gfx_Vector3 gfx_Vector3_Max(gfx_Vector3 v, gfx_Vector3 vec2) {
    return gfx_Vector3Max(v, vec2);
}

// Min - Return min value for each pair of components
gfx_Vector3 gfx_Vector3_Min(gfx_Vector3 v, gfx_Vector3 vec2) {
    return gfx_Vector3Min(v, vec2);
}

// Multiply - Multiply vector by vector
gfx_Vector3 gfx_Vector3_Multiply(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Multiply(v, v2);
}

// Negate - Negate provided vector (invert direction)
gfx_Vector3 gfx_Vector3_Negate(gfx_Vector3 v) {
    return gfx_Vector3Negate(v);
}

// Normalize - Normalize provided vector
gfx_Vector3 gfx_Vector3_Normalize(gfx_Vector3 v) {
    return gfx_Vector3Normalize(v);
}

// Perpendicular - Calculate one vector perpendicular vector
gfx_Vector3 gfx_Vector3_Perpendicular(gfx_Vector3 v) {
    return gfx_Vector3Perpendicular(v);
}

// Project - Calculate the projection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3_Project(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Project(v, v2);
}

// Reflect - Calculate reflected vector to normal
gfx_Vector3 gfx_Vector3_Reflect(gfx_Vector3 v, gfx_Vector3 normal) {
    return gfx_Vector3Reflect(v, normal);
}

// Refract - Compute the direction of a refracted ray
//
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface
gfx_Vector3 gfx_Vector3_Refract(gfx_Vector3 v, gfx_Vector3 n, float r) {
    return gfx_Vector3Refract(v, n, r);
}

// Reject - Calculate the rejection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3_Reject(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Reject(v, v2);
}

// RotateByAxisAngle - Rotates a vector around an axis
gfx_Vector3 gfx_Vector3_RotateByAxisAngle(gfx_Vector3 v, gfx_Vector3 axis, float angle) {
    return gfx_Vector3RotateByAxisAngle(v, axis, angle);
}

// RotateByQuaternion - Transform a vector by quaternion rotation
gfx_Vector3 gfx_Vector3_RotateByQuaternion(gfx_Vector3 v, gfx_Vector4 q) {
    return gfx_Vector3RotateByQuaternion(v, q);
}

// Scale - Scale provided vector
gfx_Vector3 gfx_Vector3_Scale(gfx_Vector3 v, float scale) {
    return gfx_Vector3Scale(v, scale);
}

// Subtract - Subtract two vectors
gfx_Vector3 gfx_Vector3_Subtract(gfx_Vector3 v, gfx_Vector3 v2) {
    return gfx_Vector3Subtract(v, v2);
}

// SubtractValue - Subtract vector by float value
gfx_Vector3 gfx_Vector3_SubtractValue(gfx_Vector3 v, float sub) {
    return gfx_Vector3SubtractValue(v, sub);
}

// ToFloat - Converts Vector3 to float32 slice
gfx_Float3 gfx_Vector3_ToFloat(gfx_Vector3 v) {
    return gfx_Vector3ToFloat(v);
}

// Transform - Transforms a Vector3 by a given Matrix
gfx_Vector3 gfx_Vector3_Transform(gfx_Vector3 v, gfx_Matrix mat) {
    return gfx_Vector3Transform(v, mat);
}

// Unproject - Projects a Vector3 from screen space into object space
// NOTE: We are avoiding calling other raymath functions despite available
gfx_Vector3 gfx_Vector3_Unproject(gfx_Vector3 v, gfx_Matrix projection, gfx_Matrix view) {
    return gfx_Vector3Unproject(v, projection, view);
}

// -- typedefs.go --

// NewVector2 - Returns new Vector2
gfx_Vector2 gfx_NewVector2(float x, float y) {
    return (gfx_Vector2){x, y};
}

// NewVector3 - Returns new Vector3
gfx_Vector3 gfx_NewVector3(float x, float y, float z) {
    return (gfx_Vector3){x, y, z};
}

// NewVector4 - Returns new Vector4
gfx_Vector4 gfx_NewVector4(float x, float y, float z, float w) {
    return (gfx_Vector4){x, y, z, w};
}

// NewMatrix - Returns new Matrix
gfx_Matrix gfx_NewMatrix(float m0, float m4, float m8, float m12, float m1, float m5, float m9, float m13, float m2, float m6, float m10, float m14, float m3, float m7, float m11, float m15) {
    return (gfx_Matrix){m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15};
}

// NewMat2 - Returns new Mat2
gfx_Mat2 gfx_NewMat2(float m0, float m1, float m10, float m11) {
    return (gfx_Mat2){m0, m1, m10, m11};
}

// NewQuaternion - Returns new Quaternion
gfx_Vector4 gfx_NewQuaternion(float x, float y, float z, float w) {
    return (gfx_Vector4){x, y, z, w};
}

gfx_Color gfx_Color_Tint(gfx_Color c, gfx_Color target, so_int percent) {
    float amount = (float)(percent) * 0.01;
    return (gfx_Color){.R = (uint8_t)(gfx_Lerp((float)(c.R), (float)(target.R), amount)), .G = (uint8_t)(gfx_Lerp((float)(c.G), (float)(target.G), amount)), .B = (uint8_t)(gfx_Lerp((float)(c.B), (float)(target.B), amount)), .A = c.A};
}

// NewColor - Returns new Color
gfx_Color gfx_NewColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (gfx_Color){r, g, b, a};
}

// NewRectangle - Returns new Rectangle
gfx_Rectangle gfx_NewRectangle(float x, float y, float width, float height) {
    return (gfx_Rectangle){x, y, width, height};
}

// ToInt32 converts rectangle to int32 variant
gfx_RectangleInt32 gfx_Rectangle_ToInt32(void* self) {
    gfx_Rectangle* r = self;
    gfx_RectangleInt32 rect = (gfx_RectangleInt32){};
    rect.X = (int32_t)(r->X);
    rect.Y = (int32_t)(r->Y);
    rect.Width = (int32_t)(r->W);
    rect.Height = (int32_t)(r->H);
    return rect;
}

// ToFloat32 converts rectangle to float32 variant
gfx_Rectangle gfx_RectangleInt32_ToFloat32(void* self) {
    gfx_RectangleInt32* r = self;
    gfx_Rectangle rect = (gfx_Rectangle){};
    rect.X = (float)(r->X);
    rect.Y = (float)(r->Y);
    rect.W = (float)(r->Width);
    rect.H = (float)(r->Height);
    return rect;
}

// NewCamera3D - Returns new Camera3D
gfx_Camera gfx_NewCamera3D(gfx_Vector3 pos, gfx_Vector3 target, gfx_Vector3 up, float fovy) {
    return (gfx_Camera){pos, target, up, fovy};
}

gfx_Vector2 gfx_Texture_Size(gfx_Texture t) {
    return (gfx_Vector2){(float)(t.Width), (float)(t.Height)};
}
