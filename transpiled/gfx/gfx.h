#pragma once
#include "so/builtin/builtin.h"
#include "rlgl-master.h"
#include "math_include.h"
#include "gfx/assets/assets.h"
#include "sdl/sdl.h"
#include "so/c/c.h"
#include "so/math/math.h"
#include "so/slices/slices.h"
#include "so/unicode/unicode.h"

// -- Types --

typedef struct gfx_Image gfx_Image;
typedef struct gfx_Float3 gfx_Float3;
typedef struct gfx_Float16 gfx_Float16;
typedef struct gfx_Vector2 gfx_Vector2;
typedef struct gfx_Vector3 gfx_Vector3;
typedef struct gfx_Vector4 gfx_Vector4;
typedef struct gfx_Matrix gfx_Matrix;
typedef struct gfx_Mat2 gfx_Mat2;
typedef struct gfx_Color gfx_Color;
typedef struct gfx_Rectangle gfx_Rectangle;
typedef struct gfx_RectangleInt32 gfx_RectangleInt32;
typedef struct gfx_Camera gfx_Camera;
typedef struct gfx_Camera2D gfx_Camera2D;
typedef struct gfx_Texture gfx_Texture;
typedef struct gfx_Font gfx_Font;

// Image backed by an RGBA32 SDL3 surface.
typedef struct gfx_Image {
    SDL_Surface* Surface;
} gfx_Image;

typedef struct gfx_Float3 {
    float V[3];
} gfx_Float3;

typedef struct gfx_Float16 {
    float V[16];
} gfx_Float16;

// Vector2 type
typedef struct gfx_Vector2 {
    float X;
    float Y;
} gfx_Vector2;

// Vector3 type
typedef struct gfx_Vector3 {
    float X;
    float Y;
    float Z;
} gfx_Vector3;

// Vector4 type
typedef struct gfx_Vector4 {
    float X;
    float Y;
    float Z;
    float W;
} gfx_Vector4;

// Matrix type (OpenGL style 4x4 - right handed, column major)
typedef struct gfx_Matrix {
    float M0;
    float M4;
    float M8;
    float M12;
    float M1;
    float M5;
    float M9;
    float M13;
    float M2;
    float M6;
    float M10;
    float M14;
    float M3;
    float M7;
    float M11;
    float M15;
} gfx_Matrix;

// Mat2 type (used for polygon shape rotation matrix)
typedef struct gfx_Mat2 {
    float M00;
    float M01;
    float M10;
    float M11;
} gfx_Mat2;

// Quaternion, 4 components (Vector4 alias)
typedef gfx_Vector4 gfx_Quaternion;

// Color type, RGBA (32bit)
// TODO remove later, keep type for now to not break code
typedef struct gfx_Color {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;
} gfx_Color;

// Rectangle type
typedef struct gfx_Rectangle {
    float X;
    float Y;
    float W;
    float H;
} gfx_Rectangle;

// RectangleInt32 type
typedef struct gfx_RectangleInt32 {
    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
} gfx_RectangleInt32;

// Camera type, defines a camera position/orientation in 3d space
typedef struct gfx_Camera {
    gfx_Vector3 Position;
    gfx_Vector3 Target;
    gfx_Vector3 Up;
    float Fovy;
} gfx_Camera;

// Camera2D type, defines position/orientation in 2d space
typedef struct gfx_Camera2D {
    gfx_Vector2 Offset;
    gfx_Vector2 Target;
    float Rotation;
    float Zoom;
} gfx_Camera2D;

typedef struct gfx_Texture {
    so_int Width;
    so_int Height;
    so_int ID;
} gfx_Texture;

// Font is a Minecraft bitmap font.
// Minecraft uses Ascii + Code Page 437 character set.
//
// With the exception that index 167 is assumed to be the [SectionSign].
typedef struct gfx_Font {
    gfx_Texture Atlas;
    uint8_t CharWidths[256];
} gfx_Font;

typedef struct gfx_TexturePack {
    void* self;
    so_String (*Description)(void* self);
    void (*Destroy)(void* self);
    gfx_Font* (*Font)(void* self);
    gfx_Texture (*GetTexture)(void* self, assets_ID asset);
    gfx_Texture (*Icon)(void* self);
    so_String (*Name)(void* self);
    void (*Unload)(void* self);
} gfx_TexturePack;

// -- Result types --

typedef struct gfx_ImageResult {
    gfx_Image val;
    so_Error err;
} gfx_ImageResult;

typedef struct gfx_TextureResult {
    gfx_Texture val;
    so_Error err;
} gfx_TextureResult;

typedef struct gfx_FontResult {
    gfx_Font val;
    so_Error err;
} gfx_FontResult;

// -- Variables and constants --
extern double gfx_CameraCullDistanceNear;
extern double gfx_CameraCullDistanceFar;

// Some basic Defines
static const double gfx_Pi = 3.1415927;
static const double gfx_Deg2rad = 0.017453292;
static const double gfx_Rad2deg = 57.295776;

// Java edition chat colors
extern gfx_Color gfx_Black;
extern gfx_Color gfx_DarkBlue;
extern gfx_Color gfx_DarkGreen;
extern gfx_Color gfx_DarkAqua;
extern gfx_Color gfx_DarkRed;
extern gfx_Color gfx_DarkPurple;
extern gfx_Color gfx_Gold;
extern gfx_Color gfx_Gray;
extern gfx_Color gfx_DarkGray;
extern gfx_Color gfx_Blue;
extern gfx_Color gfx_Green;
extern gfx_Color gfx_Aqua;
extern gfx_Color gfx_Red;
extern gfx_Color gfx_LightPurple;
extern gfx_Color gfx_Yellow;
extern gfx_Color gfx_White;
extern SDL_Window* gfx_Window;
extern so_String gfx_AssetsPath;

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
extern so_int gfx_TexturesLoaded;

// Approximately the GPU memory used by textures in bytes.
extern so_int gfx_TextureMemoryUsed;

// https://minecraft.wiki/w/Formatting_codes
//
// NOTE: only color formatting codes are supported in beta 1.7.3
static const so_rune gfx_SectionSign = U'§';

// OpenGL version
static const int64_t gfx_RL_OPENGL_SOFTWARE = 0;
static const int64_t gfx_RL_OPENGL_11 = 1;
static const int64_t gfx_RL_OPENGL_21 = 2;
static const int64_t gfx_RL_OPENGL_33 = 3;
static const int64_t gfx_RL_OPENGL_43 = 4;
static const int64_t gfx_RL_OPENGL_ES_20 = 5;
static const int64_t gfx_RL_OPENGL_ES_30 = 6;

// -- Functions and methods --

// GetForward - Returns the cameras forward vector (normalized)
gfx_Vector3 gfx_Camera_GetForward(void* self);

// GetUp - Returns the cameras up vector (normalized)
// Note: The up vector might not be perpendicular to the forward vector
gfx_Vector3 gfx_Camera_GetUp(void* self);

// GetRight - Returns the cameras right vector (normalized)
gfx_Vector3 gfx_Camera_GetRight(void* self);

// MoveForward - Moves the camera in its forward direction
void gfx_Camera_MoveForward(void* self, float distance, bool moveInWorldPlane);

// MoveUp - Moves the camera in its up direction
void gfx_Camera_MoveUp(void* self, float distance);

// MoveRight - Moves the camera target in its current right direction
void gfx_Camera_MoveRight(void* self, float distance, bool moveInWorldPlane);

// MoveToTarget - Moves the camera position closer/farther to/from the camera target
void gfx_Camera_MoveToTarget(void* self, float delta);

// Yaw - Rotates the camera around its up vector
// Yaw is "looking left and right"
// If rotateAroundTarget is false, the camera rotates around its position
// Note: angle must be provided in radians
void gfx_Camera_Yaw(void* self, float angle, bool rotateAroundTarget);

// Pitch - Rotates the camera around its right vector, pitch is "looking up and down"
//   - lockView prevents camera overrotation (aka "somersaults")
//   - rotateAroundTarget defines if rotation is around target or around its position
//   - rotateUp rotates the up direction as well (typically only useful in CAMERA_FREE)
//
// NOTE: angle must be provided in radians
void gfx_Camera_Pitch(void* self, float angle, bool lockView, bool rotateAroundTarget, bool rotateUp);

// Roll - Rotates the camera around its forward vector
// Roll is "turning your head sideways to the left or right"
// Note: angle must be provided in radians
void gfx_Camera_Roll(void* self, float angle);

// ViewMatrix - Returns the camera view matrix
gfx_Matrix gfx_Camera_ViewMatrix(void* self);

// ProjectionMatrix - Returns the camera projection matrix
gfx_Matrix gfx_Camera_ProjectionMatrix(void* self, float aspect);

// Update - Update camera movement, movement/rotation values should be provided by user
// Required values
// movement.X - Move forward/backward
// movement.Y - Move right/left
// movement.Z - Move up/down
// rotation.X - yaw
// rotation.Y - pitch
// rotation.Z - roll
// zoom - Move towards target
void gfx_Camera_Update(void* self, gfx_Vector3 movement, gfx_Vector3 rotation, float zoom);
gfx_Matrix gfx_GetCameraMatrix2D(gfx_Camera2D cam);

// Set viewport for a provided width and height
void gfx_SetupViewport(so_int width, so_int height);
void gfx_EnableTexture(gfx_Texture t);
void gfx_DisableTexture(void);
void gfx_Init(SDL_Window* win);
so_R_int_int gfx_GetWindowSize(void);
void gfx_BeginDrawing(void);
void gfx_EndDrawing(void);
void gfx_ClearBackground(gfx_Color c);
void gfx_BeginMode3D(gfx_Camera cam);
void gfx_EndMode3D(void);
void gfx_BeginMode2D(gfx_Camera2D cam);
void gfx_EndMode2D(void);

// Get the screen space position for a 2d camera world space position
gfx_Vector2 gfx_GetWorldToScreen2D(gfx_Vector2 position, gfx_Camera2D camera);

// Get the world space position for a 2d camera screen space position
gfx_Vector2 gfx_GetScreenToWorld2D(gfx_Vector2 position, gfx_Camera2D camera);
gfx_ImageResult gfx_LoadImage(so_String path);
void gfx_Image_Destroy(void* self);
so_R_int_int gfx_Image_Size(void* self);

// Get a pixel from the image.
gfx_Color gfx_Image_Get(void* self, so_int x, so_int y);
so_Slice gfx_Image_Pixels(void* self);
gfx_TextureResult gfx_LoadTextureFromImage(gfx_Image img);
gfx_TextureResult gfx_LoadTexture(so_String path);
void gfx_SetTextureConfig(gfx_Texture t, bool blur, bool clamp);
void gfx_UnloadTexture(gfx_Texture texture);
void gfx_DrawTexture(gfx_Texture texture, gfx_Vector2 pos);
void gfx_DrawTextureEx(gfx_Texture texture, gfx_Rectangle src, gfx_Rectangle dst);
void gfx_DrawTextureRec(gfx_Texture texture, gfx_Rectangle src, gfx_Rectangle dst);
void gfx_DrawTextureTiled(gfx_Texture texture, gfx_Rectangle dest, float scale, gfx_Color tint);

// DrawTexturePro draws a portion of a texture into a destination rectangle,
// optionally rotated around origin.
//
// origin is relative to dest's size, matching raylib-style semantics.
void gfx_DrawTexturePro(gfx_Texture texture, gfx_Rectangle source, gfx_Rectangle dest, gfx_Vector2 origin, float rotation, gfx_Color tint);

// These are all the characters allowed by Minecraft.
bool gfx_IsRuneAllowed(so_rune r);

// Load Minecraft bitmap font
gfx_FontResult gfx_LoadFont(so_String path);

// TextHeight is the same as the full glyph bounding box in the Atlas.
so_int gfx_Font_TextHeight(void* self);
gfx_Vector2 gfx_Font_TextSize(void* self, so_Slice text);
gfx_Vector2 gfx_Font_GlyphSize(void* self, so_rune charCode);

// Get text width.
so_int gfx_Font_TextWidth(void* self, so_Slice text);
void gfx_Font_Destroy(void* self);
void gfx_Font_DrawRunes(void* self, so_Slice text, gfx_Vector2 position, float scale, float rotation, gfx_Color color, bool darken);
void gfx_DrawRectangle(gfx_Rectangle rectangle, gfx_Color color);

// Draw a color-filled rectangle with pro parameters
// DrawRectanglePro draws a color-filled rectangle with rotation and origin.
//
// origin is relative to rectangle size, matching raylib semantics.
void gfx_DrawRectanglePro(gfx_Rectangle rectangle, gfx_Vector2 origin, float rotation, gfx_Color color);

// Clamp - Clamp float value
//
float gfx_Clamp(float value, float min, float max);

// Lerp - Calculate linear interpolation between two floats
float gfx_Lerp(float start, float end, float amount);

// Normalize - Normalize input value within input range
float gfx_Normalize(float value, float start, float end);

// Remap - Remap input value within input range to output range
float gfx_Remap(float value, float inputStart, float inputEnd, float outputStart, float outputEnd);

// Wrap - Wrap input value from min to max
float gfx_Wrap(float value, float min, float max);

// FloatEquals - Check whether two given floats are almost equal
bool gfx_FloatEquals(float x, float y);

// Vector2Zero - Vector with components value 0.0
gfx_Vector2 gfx_Vector2Zero(void);

// Vector2One - Vector with components value 1.0
gfx_Vector2 gfx_Vector2One(void);

// Vector2Add - Add two vectors (v1 + v2)
gfx_Vector2 gfx_Vector2Add(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2AddValue - Add vector and float value
gfx_Vector2 gfx_Vector2AddValue(gfx_Vector2 v, float add);

// Vector2Subtract - Subtract two vectors (v1 - v2)
gfx_Vector2 gfx_Vector2Subtract(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2SubtractValue - Subtract vector by float value
gfx_Vector2 gfx_Vector2SubtractValue(gfx_Vector2 v, float sub);

// Vector2Length - Calculate vector length
float gfx_Vector2Length(gfx_Vector2 v);

// Vector2LengthSqr - Calculate vector square length
float gfx_Vector2LengthSqr(gfx_Vector2 v);

// Vector2DotProduct - Calculate two vectors dot product
float gfx_Vector2DotProduct(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2Distance - Calculate distance between two vectors
float gfx_Vector2Distance(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2DistanceSqr - Calculate square distance between two vectors
float gfx_Vector2DistanceSqr(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2Angle - Calculate angle from two vectors in radians
// NOTE: Coordinate system convention: positive X right, positive Y down,
// positive angles appear clockwise, and negative angles appear counterclockwise
float gfx_Vector2Angle(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2LineAngle - Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized. Current implementation should be aligned with glm::angle
float gfx_Vector2LineAngle(gfx_Vector2 start, gfx_Vector2 end);

// Vector2Scale - Scale vector (multiply by value)
gfx_Vector2 gfx_Vector2Scale(gfx_Vector2 v, float scale);

// Vector2Multiply - Multiply vector by vector
gfx_Vector2 gfx_Vector2Multiply(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2Negate - Negate vector
gfx_Vector2 gfx_Vector2Negate(gfx_Vector2 v);

// Vector2Divide - Divide vector by vector
gfx_Vector2 gfx_Vector2Divide(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2Normalize - Normalize provided vector
gfx_Vector2 gfx_Vector2Normalize(gfx_Vector2 v);

// Vector2Transform - Transforms a Vector2 by a given Matrix
gfx_Vector2 gfx_Vector2Transform(gfx_Vector2 v, gfx_Matrix mat);

// Vector2Lerp - Calculate linear interpolation between two vectors
gfx_Vector2 gfx_Vector2Lerp(gfx_Vector2 v1, gfx_Vector2 v2, float amount);

// Vector2Reflect - Calculate reflected vector to normal
gfx_Vector2 gfx_Vector2Reflect(gfx_Vector2 v, gfx_Vector2 normal);

// Vector2Rotate - Rotate vector by angle
gfx_Vector2 gfx_Vector2Rotate(gfx_Vector2 v, float angle);

// Vector2MoveTowards - Move Vector towards target
gfx_Vector2 gfx_Vector2MoveTowards(gfx_Vector2 v, gfx_Vector2 target, float maxDistance);

// Vector2Invert - Invert the given vector
gfx_Vector2 gfx_Vector2Invert(gfx_Vector2 v);

// Vector2Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector2 gfx_Vector2Clamp(gfx_Vector2 v, gfx_Vector2 min, gfx_Vector2 max);

// Vector2ClampValue - Clamp the magnitude of the vector between two min and max values
gfx_Vector2 gfx_Vector2ClampValue(gfx_Vector2 v, float min, float max);

// Vector2Equals - Check whether two given vectors are almost equal
bool gfx_Vector2Equals(gfx_Vector2 p, gfx_Vector2 q);

// Vector2CrossProduct - Calculate two vectors cross product
float gfx_Vector2CrossProduct(gfx_Vector2 v1, gfx_Vector2 v2);

// Vector2Cross - Calculate the cross product of a vector and a value
gfx_Vector2 gfx_Vector2Cross(float value, gfx_Vector2 vector);

// Vector3Zero - Vector with components value 0.0
gfx_Vector3 gfx_Vector3Zero(void);

// Vector3One - Vector with components value 1.0
gfx_Vector3 gfx_Vector3One(void);

// Vector3Add - Add two vectors
gfx_Vector3 gfx_Vector3Add(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3AddValue - Add vector and float value
gfx_Vector3 gfx_Vector3AddValue(gfx_Vector3 v, float add);

// Vector3Subtract - Subtract two vectors
gfx_Vector3 gfx_Vector3Subtract(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3SubtractValue - Subtract vector by float value
gfx_Vector3 gfx_Vector3SubtractValue(gfx_Vector3 v, float sub);

// Vector3Scale - Scale provided vector
gfx_Vector3 gfx_Vector3Scale(gfx_Vector3 v, float scale);

// Vector3Multiply - Multiply vector by vector
gfx_Vector3 gfx_Vector3Multiply(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3CrossProduct - Calculate two vectors cross product
gfx_Vector3 gfx_Vector3CrossProduct(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Perpendicular - Calculate one vector perpendicular vector
gfx_Vector3 gfx_Vector3Perpendicular(gfx_Vector3 v);

// Vector3Length - Calculate vector length
float gfx_Vector3Length(gfx_Vector3 v);

// Vector3LengthSqr - Calculate vector square length
float gfx_Vector3LengthSqr(gfx_Vector3 v);

// Vector3DotProduct - Calculate two vectors dot product
float gfx_Vector3DotProduct(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Distance - Calculate distance between two vectors
float gfx_Vector3Distance(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3DistanceSqr - Calculate square distance between two vectors
float gfx_Vector3DistanceSqr(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Angle - Calculate angle between two vectors
float gfx_Vector3Angle(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Negate - Negate provided vector (invert direction)
gfx_Vector3 gfx_Vector3Negate(gfx_Vector3 v);

// Vector3Divide - Divide vector by vector
gfx_Vector3 gfx_Vector3Divide(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Normalize - Normalize provided vector
gfx_Vector3 gfx_Vector3Normalize(gfx_Vector3 v);

// Vector3Project - Calculate the projection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3Project(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3Reject - Calculate the rejection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3Reject(gfx_Vector3 v1, gfx_Vector3 v2);

// Vector3OrthoNormalize - Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
void gfx_Vector3OrthoNormalize(gfx_Vector3* v1, gfx_Vector3* v2);

// Vector3Transform - Transforms a Vector3 by a given Matrix
gfx_Vector3 gfx_Vector3Transform(gfx_Vector3 v, gfx_Matrix mat);

// Vector3RotateByQuaternion - Transform a vector by quaternion rotation
gfx_Vector3 gfx_Vector3RotateByQuaternion(gfx_Vector3 v, gfx_Vector4 q);

// Vector3RotateByAxisAngle - Rotates a vector around an axis
gfx_Vector3 gfx_Vector3RotateByAxisAngle(gfx_Vector3 v, gfx_Vector3 axis, float angle);

// Vector3Lerp - Calculate linear interpolation between two vectors
gfx_Vector3 gfx_Vector3Lerp(gfx_Vector3 v1, gfx_Vector3 v2, float amount);

// Vector3Reflect - Calculate reflected vector to normal
gfx_Vector3 gfx_Vector3Reflect(gfx_Vector3 vector, gfx_Vector3 normal);

// Vector3Min - Return min value for each pair of components
gfx_Vector3 gfx_Vector3Min(gfx_Vector3 vec1, gfx_Vector3 vec2);

// Vector3Max - Return max value for each pair of components
gfx_Vector3 gfx_Vector3Max(gfx_Vector3 vec1, gfx_Vector3 vec2);

// Vector3Barycenter - Barycenter coords for p in triangle abc
gfx_Vector3 gfx_Vector3Barycenter(gfx_Vector3 p, gfx_Vector3 a, gfx_Vector3 b, gfx_Vector3 c);

// Vector3Unproject - Projects a Vector3 from screen space into object space
// NOTE: We are avoiding calling other raymath functions despite available
gfx_Vector3 gfx_Vector3Unproject(gfx_Vector3 source, gfx_Matrix projection, gfx_Matrix view);

// Vector3ToFloatV - Get Vector3 as float array
gfx_Float3 gfx_Vector3ToFloat(gfx_Vector3 v);

// Vector3Invert - Invert the given vector
gfx_Vector3 gfx_Vector3Invert(gfx_Vector3 v);

// Vector3Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector3 gfx_Vector3Clamp(gfx_Vector3 v, gfx_Vector3 min, gfx_Vector3 max);

// Vector3ClampValue - Clamp the magnitude of the vector between two values
gfx_Vector3 gfx_Vector3ClampValue(gfx_Vector3 v, float min, float max);

// Vector3Equals - Check whether two given vectors are almost equal
bool gfx_Vector3Equals(gfx_Vector3 p, gfx_Vector3 q);

// Vector3Refract - Compute the direction of a refracted ray
//
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface
gfx_Vector3 gfx_Vector3Refract(gfx_Vector3 v, gfx_Vector3 n, float r);

// Mat2Radians - Creates a matrix 2x2 from a given radians value
gfx_Mat2 gfx_Mat2Radians(float radians);

// Mat2Set - Set values from radians to a created matrix 2x2
void gfx_Mat2Set(gfx_Mat2* matrix, float radians);

// Mat2Transpose - Returns the transpose of a given matrix 2x2
gfx_Mat2 gfx_Mat2Transpose(gfx_Mat2 matrix);

// Mat2MultiplyVector2 - Multiplies a vector by a matrix 2x2
gfx_Vector2 gfx_Mat2MultiplyVector2(gfx_Mat2 matrix, gfx_Vector2 vector);

// MatrixDeterminant - Compute matrix determinant
float gfx_MatrixDeterminant(gfx_Matrix mat);

// MatrixTrace - Returns the trace of the matrix (sum of the values along the diagonal)
float gfx_MatrixTrace(gfx_Matrix mat);

// MatrixTranspose - Transposes provided matrix
gfx_Matrix gfx_MatrixTranspose(gfx_Matrix mat);

// MatrixInvert - Invert provided matrix
gfx_Matrix gfx_MatrixInvert(gfx_Matrix mat);

// MatrixIdentity - Returns identity matrix
gfx_Matrix gfx_MatrixIdentity(void);

// MatrixNormalize - Normalize provided matrix
gfx_Matrix gfx_MatrixNormalize(gfx_Matrix mat);

// MatrixAdd - Add two matrices
gfx_Matrix gfx_MatrixAdd(gfx_Matrix left, gfx_Matrix right);

// MatrixSubtract - Subtract two matrices (left - right)
gfx_Matrix gfx_MatrixSubtract(gfx_Matrix left, gfx_Matrix right);

// MatrixMultiply - Returns two matrix multiplication
gfx_Matrix gfx_MatrixMultiply(gfx_Matrix left, gfx_Matrix right);

// MatrixTranslate - Returns translation matrix
gfx_Matrix gfx_MatrixTranslate(float x, float y, float z);

// MatrixRotate - Returns rotation matrix for an angle around an specified axis (angle in radians)
gfx_Matrix gfx_MatrixRotate(gfx_Vector3 axis, float angle);

// MatrixRotateX - Returns x-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateX(float angle);

// MatrixRotateY - Returns y-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateY(float angle);

// MatrixRotateZ - Returns z-rotation matrix (angle in radians)
gfx_Matrix gfx_MatrixRotateZ(float angle);

// MatrixRotateXYZ - Get xyz-rotation matrix (angles in radians)
gfx_Matrix gfx_MatrixRotateXYZ(gfx_Vector3 angle);

// MatrixRotateZYX - Get zyx-rotation matrix
// NOTE: Angle must be provided in radians
gfx_Matrix gfx_MatrixRotateZYX(gfx_Vector3 angle);

// MatrixScale - Returns scaling matrix
gfx_Matrix gfx_MatrixScale(float x, float y, float z);

// MatrixFrustum - Returns perspective projection matrix
gfx_Matrix gfx_MatrixFrustum(float left, float right, float bottom, float top, float nearPlane, float farPlane);

// MatrixPerspective - Returns perspective projection matrix
// NOTE: Fovy angle must be provided in radians
gfx_Matrix gfx_MatrixPerspective(float fovY, float aspect, float nearPlane, float farPlane);

// MatrixOrtho - Returns orthographic projection matrix
gfx_Matrix gfx_MatrixOrtho(float left, float right, float bottom, float top, float near, float far);

// MatrixLookAt - Returns camera look-at matrix (view matrix)
gfx_Matrix gfx_MatrixLookAt(gfx_Vector3 eye, gfx_Vector3 target, gfx_Vector3 up);

// MatrixToFloat - Get float array of matrix data
gfx_Float16 gfx_MatrixToFloat(gfx_Matrix mat);

// QuaternionAdd - Add two quaternions
gfx_Vector4 gfx_QuaternionAdd(gfx_Vector4 q1, gfx_Vector4 q2);

// QuaternionAddValue - Add quaternion and float value
gfx_Vector4 gfx_QuaternionAddValue(gfx_Vector4 q, float add);

// QuaternionSubtract - Subtract two quaternions
gfx_Vector4 gfx_QuaternionSubtract(gfx_Vector4 q1, gfx_Vector4 q2);

// QuaternionSubtractValue - Subtract quaternion and float value
gfx_Vector4 gfx_QuaternionSubtractValue(gfx_Vector4 q, float sub);

// QuaternionIdentity - Get identity quaternion
gfx_Vector4 gfx_QuaternionIdentity(void);

// QuaternionLength - Compute the length of a quaternion
float gfx_QuaternionLength(gfx_Vector4 quat);

// QuaternionNormalize - Normalize provided quaternion
gfx_Vector4 gfx_QuaternionNormalize(gfx_Vector4 q);

// QuaternionInvert - Invert provided quaternion
gfx_Vector4 gfx_QuaternionInvert(gfx_Vector4 quat);

// QuaternionMultiply - Calculate two quaternion multiplication
gfx_Vector4 gfx_QuaternionMultiply(gfx_Vector4 q1, gfx_Vector4 q2);

// QuaternionScale - Scale quaternion by float value
gfx_Vector4 gfx_QuaternionScale(gfx_Vector4 q, float mul);

// QuaternionDivide - Divide two quaternions
gfx_Vector4 gfx_QuaternionDivide(gfx_Vector4 q1, gfx_Vector4 q2);

// QuaternionLerp - Calculate linear interpolation between two quaternions
gfx_Vector4 gfx_QuaternionLerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount);

// QuaternionNlerp - Calculate slerp-optimized interpolation between two quaternions
gfx_Vector4 gfx_QuaternionNlerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount);

// QuaternionSlerp - Calculates spherical linear interpolation between two quaternions
gfx_Vector4 gfx_QuaternionSlerp(gfx_Vector4 q1, gfx_Vector4 q2, float amount);

// QuaternionFromVector3ToVector3 - Calculate quaternion based on the rotation from one vector to another
gfx_Vector4 gfx_QuaternionFromVector3ToVector3(gfx_Vector3 from, gfx_Vector3 to);

// QuaternionFromMatrix - Returns a quaternion for a given rotation matrix
gfx_Vector4 gfx_QuaternionFromMatrix(gfx_Matrix mat);

// QuaternionToMatrix - Returns a matrix for a given quaternion
gfx_Matrix gfx_QuaternionToMatrix(gfx_Vector4 q);

// QuaternionFromAxisAngle - Returns rotation quaternion for an angle and axis
gfx_Vector4 gfx_QuaternionFromAxisAngle(gfx_Vector3 axis, float angle);

// QuaternionToAxisAngle - Returns the rotation angle and axis for a given quaternion
void gfx_QuaternionToAxisAngle(gfx_Vector4 q, gfx_Vector3* outAxis, float* outAngle);

// QuaternionFromEuler - Get the quaternion equivalent to Euler angles
// NOTE: Rotation order is ZYX
gfx_Vector4 gfx_QuaternionFromEuler(float pitch, float yaw, float roll);

// QuaternionToEuler - Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
gfx_Vector3 gfx_QuaternionToEuler(gfx_Vector4 q);

// QuaternionTransform - Transform a quaternion given a transformation matrix
gfx_Vector4 gfx_QuaternionTransform(gfx_Vector4 q, gfx_Matrix mat);

// QuaternionEquals - Check whether two given quaternions are almost equal
bool gfx_QuaternionEquals(gfx_Vector4 q, gfx_Vector4 p);

// MatrixDecompose - Decompose a transformation matrix into its rotational, translational and scaling components
void gfx_MatrixDecompose(gfx_Matrix mat, gfx_Vector3* translation, gfx_Vector4* rotation, gfx_Vector3* scale);
so_R_f32_f32 gfx_Sincos(float angle);

// anchor this rectangle's position inside parent rectangle.
// Returns new position
gfx_Rectangle gfx_Rectangle_Anchor(gfx_Rectangle r, gfx_Rectangle parent, float anchorX, float anchorY);
gfx_Rectangle gfx_Rectangle_Multiply(gfx_Rectangle r, float v);

// Scale size
gfx_Rectangle gfx_Rectangle_Scale(gfx_Rectangle r, float v);

// AddPosition position by adding it
gfx_Rectangle gfx_Rectangle_AddPosition(gfx_Rectangle r, gfx_Vector2 v);
gfx_Rectangle gfx_Rectangle_SubractPosition(gfx_Rectangle r, gfx_Vector2 v);
gfx_Rectangle gfx_Rectangle_SetPosition(gfx_Rectangle r, gfx_Vector2 v);

// Grow equally on all sides (anchored to center).
gfx_Rectangle gfx_Rectangle_Grow(gfx_Rectangle r, float v);

// Shrink equally on all sides (anchored to center).
gfx_Rectangle gfx_Rectangle_Shrink(gfx_Rectangle r, float v);
gfx_Rectangle gfx_Rectangle_SetSize(gfx_Rectangle r, gfx_Vector2 v);
gfx_Vector2 gfx_Rectangle_Position(gfx_Rectangle r);
gfx_Vector2 gfx_Rectangle_Size(gfx_Rectangle r);
bool gfx_Rectangle_Contains(gfx_Rectangle r, gfx_Vector2 p);

// MultiplyVector2 - Multiplies a vector by a matrix 2x2
gfx_Vector2 gfx_Mat2_MultiplyVector2(gfx_Mat2 m, gfx_Vector2 vector);

// Transpose - Returns the transpose of a given matrix 2x2
gfx_Mat2 gfx_Mat2_Transpose(gfx_Mat2 m);

// Add - Add two matrices
gfx_Matrix gfx_Matrix_Add(gfx_Matrix m, gfx_Matrix right);

// Decompose - Decompose a transformation matrix into its rotational, translational and scaling components
void gfx_Matrix_Decompose(gfx_Matrix m, gfx_Vector3* translation, gfx_Vector4* rotation, gfx_Vector3* scale);

// Determinant - Compute matrix determinant
float gfx_Matrix_Determinant(gfx_Matrix m);

// Invert - Invert provided matrix
gfx_Matrix gfx_Matrix_Invert(gfx_Matrix m);

// Multiply - Returns two matrix multiplication
gfx_Matrix gfx_Matrix_Multiply(gfx_Matrix m, gfx_Matrix right);

// Normalize - Normalize provided matrix
gfx_Matrix gfx_Matrix_Normalize(gfx_Matrix m);

// Subtract - Subtract two matrices (left - right)
gfx_Matrix gfx_Matrix_Subtract(gfx_Matrix m, gfx_Matrix right);

// ToFloatV - Get float array of matrix data
gfx_Float16 gfx_Matrix_ToFloat(gfx_Matrix m);

// Trace - Returns the trace of the matrix (sum of the values along the diagonal)
float gfx_Matrix_Trace(gfx_Matrix m);

// Transpose - Transposes provided matrix
gfx_Matrix gfx_Matrix_Transpose(gfx_Matrix m);

// Add - Add two quaternions
gfx_Vector4 gfx_Vector4_Add(gfx_Vector4 q, gfx_Vector4 q2);

// AddValue - Add quaternion and float value
gfx_Vector4 gfx_Vector4_AddValue(gfx_Vector4 q, float add);

// Divide - Divide two quaternions
gfx_Vector4 gfx_Vector4_Divide(gfx_Vector4 q, gfx_Vector4 q2);

// Equals - Check whether two given quaternions are almost equal
bool gfx_Vector4_Equals(gfx_Vector4 q, gfx_Vector4 p);

// Invert - Invert provided quaternion
gfx_Vector4 gfx_Vector4_Invert(gfx_Vector4 q);

// Length - Compute the length of a quaternion
float gfx_Vector4_Length(gfx_Vector4 q);

// Lerp - Calculate linear interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Lerp(gfx_Vector4 q, gfx_Vector4 q2, float amount);

// Multiply - Calculate two quaternion multiplication
gfx_Vector4 gfx_Vector4_Multiply(gfx_Vector4 q, gfx_Vector4 q2);

// Nlerp - Calculate slerp-optimized interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Nlerp(gfx_Vector4 q, gfx_Vector4 q2, float amount);

// Normalize - Normalize provided quaternion
gfx_Vector4 gfx_Vector4_Normalize(gfx_Vector4 q);

// Scale - Scale quaternion by float value
gfx_Vector4 gfx_Vector4_Scale(gfx_Vector4 q, float mul);

// Slerp - Calculates spherical linear interpolation between two quaternions
gfx_Vector4 gfx_Vector4_Slerp(gfx_Vector4 q, gfx_Vector4 q2, float amount);

// Subtract - Subtract two quaternions
gfx_Vector4 gfx_Vector4_Subtract(gfx_Vector4 q, gfx_Vector4 q2);

// SubtractValue - Subtract quaternion and float value
gfx_Vector4 gfx_Vector4_SubtractValue(gfx_Vector4 q, float sub);

// ToAxisAngle - Returns the rotation angle and axis for a given quaternion
void gfx_Vector4_ToAxisAngle(gfx_Vector4 q, gfx_Vector3* outAxis, float* outAngle);

// ToEuler - Get the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in radians
gfx_Vector3 gfx_Vector4_ToEuler(gfx_Vector4 q);

// ToMatrix - Returns a matrix for a given quaternion
gfx_Matrix gfx_Vector4_ToMatrix(gfx_Vector4 q);

// Transform - Transform a quaternion given a transformation matrix
gfx_Vector4 gfx_Vector4_Transform(gfx_Vector4 q, gfx_Matrix mat);

// Add - Add two vectors (v1 + v2)
gfx_Vector2 gfx_Vector2_Add(gfx_Vector2 v, gfx_Vector2 v2);

// AddValue - Add vector and float value
gfx_Vector2 gfx_Vector2_AddValue(gfx_Vector2 v, float add);

// Angle - Calculate angle from two vectors in radians
// NOTE: Coordinate system convention: positive X right, positive Y down,
// positive angles appear clockwise, and negative angles appear counterclockwise
float gfx_Vector2_Angle(gfx_Vector2 v, gfx_Vector2 v2);

// Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector2 gfx_Vector2_Clamp(gfx_Vector2 v, gfx_Vector2 min, gfx_Vector2 max);

// ClampValue - Clamp the magnitude of the vector between two min and max values
gfx_Vector2 gfx_Vector2_ClampValue(gfx_Vector2 v, float min, float max);

// CrossProduct - Calculate two vectors cross product
float gfx_Vector2_CrossProduct(gfx_Vector2 v, gfx_Vector2 v2);

// Distance - Calculate distance between two vectors
float gfx_Vector2_Distance(gfx_Vector2 v, gfx_Vector2 v2);

// Intersect - Calculate square distance between two vectors
bool gfx_Vector2_Intersect(gfx_Vector2 v, gfx_Vector2 v2, float radius);

// Divide - Divide vector by vector
gfx_Vector2 gfx_Vector2_Divide(gfx_Vector2 v, gfx_Vector2 v2);

// DotProduct - Calculate two vectors dot product
float gfx_Vector2_DotProduct(gfx_Vector2 v, gfx_Vector2 v2);

// Equals - Check whether two given vectors are almost equal
bool gfx_Vector2_Equals(gfx_Vector2 v, gfx_Vector2 q);

// Invert - Invert the given vector
gfx_Vector2 gfx_Vector2_Invert(gfx_Vector2 v);

// Length - Calculate vector length
float gfx_Vector2_Length(gfx_Vector2 v);

// LengthSqr - Calculate vector square length
float gfx_Vector2_LengthSqr(gfx_Vector2 v);

// Lerp - Calculate linear interpolation between two vectors
gfx_Vector2 gfx_Vector2_Lerp(gfx_Vector2 v, gfx_Vector2 v2, float amount);

// LineAngle - Calculate angle defined by a two vectors line
// NOTE: Parameters need to be normalized. Current implementation should be aligned with glm::angle
float gfx_Vector2_LineAngle(gfx_Vector2 v, gfx_Vector2 end);

// MoveTowards - Move Vector towards target
gfx_Vector2 gfx_Vector2_MoveTowards(gfx_Vector2 v, gfx_Vector2 target, float maxDistance);

// Multiply - Multiply vector by vector
gfx_Vector2 gfx_Vector2_Multiply(gfx_Vector2 v, gfx_Vector2 v2);

// Negate - Negate vector
gfx_Vector2 gfx_Vector2_Negate(gfx_Vector2 v);

// Normalize - Normalize provided vector
gfx_Vector2 gfx_Vector2_Normalize(gfx_Vector2 v);

// Reflect - Calculate reflected vector to normal
gfx_Vector2 gfx_Vector2_Reflect(gfx_Vector2 v, gfx_Vector2 normal);

// Rotate - Rotate vector by angle
gfx_Vector2 gfx_Vector2_Rotate(gfx_Vector2 v, float angle);
gfx_Vector2 gfx_Vector2_RotateAroundPivot(gfx_Vector2 v, gfx_Vector2 pivot, float angle);
gfx_Vector2 gfx_Vector2_Half(gfx_Vector2 v);

// Scale - Scale vector (multiply by value)
gfx_Vector2 gfx_Vector2_Scale(gfx_Vector2 v, float scale);

// Subtract - Subtract two vectors (v1 - v2)
gfx_Vector2 gfx_Vector2_Subtract(gfx_Vector2 v, gfx_Vector2 v2);

// SubtractValue - Subtract vector by float value
gfx_Vector2 gfx_Vector2_SubtractValue(gfx_Vector2 v, float sub);

// Transform - Transforms a Vector2 by a given Matrix
gfx_Vector2 gfx_Vector2_Transform(gfx_Vector2 v, gfx_Matrix mat);

// Transform - Transforms a Vector2 by a given Matrix
so_R_f32_f32 gfx_Vector2_XY(gfx_Vector2 v);

// Add - Add two vectors
gfx_Vector3 gfx_Vector3_Add(gfx_Vector3 v, gfx_Vector3 v2);

// AddValue - Add vector and float value
gfx_Vector3 gfx_Vector3_AddValue(gfx_Vector3 v, float add);

// Angle - Calculate angle between two vectors
float gfx_Vector3_Angle(gfx_Vector3 v, gfx_Vector3 v2);

// Barycenter - Barycenter coords for p in triangle abc
gfx_Vector3 gfx_Vector3_Barycenter(gfx_Vector3 v, gfx_Vector3 a, gfx_Vector3 b, gfx_Vector3 c);

// Clamp - Clamp the components of the vector between min and max values specified by the given vectors
gfx_Vector3 gfx_Vector3_Clamp(gfx_Vector3 v, gfx_Vector3 min, gfx_Vector3 max);

// ClampValue - Clamp the magnitude of the vector between two values
gfx_Vector3 gfx_Vector3_ClampValue(gfx_Vector3 v, float min, float max);

// CrossProduct - Calculate two vectors cross product
gfx_Vector3 gfx_Vector3_CrossProduct(gfx_Vector3 v, gfx_Vector3 v2);

// Distance - Calculate distance between two vectors
float gfx_Vector3_Distance(gfx_Vector3 v, gfx_Vector3 v2);

// DistanceSqr - Calculate square distance between two vectors
float gfx_Vector3_DistanceSqr(gfx_Vector3 v, gfx_Vector3 v2);

// Divide - Divide vector by vector
gfx_Vector3 gfx_Vector3_Divide(gfx_Vector3 v, gfx_Vector3 v2);

// DotProduct - Calculate two vectors dot product
float gfx_Vector3_DotProduct(gfx_Vector3 v, gfx_Vector3 v2);

// Equals - Check whether two given vectors are almost equal
bool gfx_Vector3_Equals(gfx_Vector3 v, gfx_Vector3 q);

// Invert - Invert the given vector
gfx_Vector3 gfx_Vector3_Invert(gfx_Vector3 v);

// Length - Calculate vector length
float gfx_Vector3_Length(gfx_Vector3 v);

// LengthSqr - Calculate vector square length
float gfx_Vector3_LengthSqr(gfx_Vector3 v);

// Lerp - Calculate linear interpolation between two vectors
gfx_Vector3 gfx_Vector3_Lerp(gfx_Vector3 v, gfx_Vector3 v2, float amount);

// Max - Return max value for each pair of components
gfx_Vector3 gfx_Vector3_Max(gfx_Vector3 v, gfx_Vector3 vec2);

// Min - Return min value for each pair of components
gfx_Vector3 gfx_Vector3_Min(gfx_Vector3 v, gfx_Vector3 vec2);

// Multiply - Multiply vector by vector
gfx_Vector3 gfx_Vector3_Multiply(gfx_Vector3 v, gfx_Vector3 v2);

// Negate - Negate provided vector (invert direction)
gfx_Vector3 gfx_Vector3_Negate(gfx_Vector3 v);

// Normalize - Normalize provided vector
gfx_Vector3 gfx_Vector3_Normalize(gfx_Vector3 v);

// Perpendicular - Calculate one vector perpendicular vector
gfx_Vector3 gfx_Vector3_Perpendicular(gfx_Vector3 v);

// Project - Calculate the projection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3_Project(gfx_Vector3 v, gfx_Vector3 v2);

// Reflect - Calculate reflected vector to normal
gfx_Vector3 gfx_Vector3_Reflect(gfx_Vector3 v, gfx_Vector3 normal);

// Refract - Compute the direction of a refracted ray
//
// v: normalized direction of the incoming ray
// n: normalized normal vector of the interface of two optical media
// r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface
gfx_Vector3 gfx_Vector3_Refract(gfx_Vector3 v, gfx_Vector3 n, float r);

// Reject - Calculate the rejection of the vector v1 on to v2
gfx_Vector3 gfx_Vector3_Reject(gfx_Vector3 v, gfx_Vector3 v2);

// RotateByAxisAngle - Rotates a vector around an axis
gfx_Vector3 gfx_Vector3_RotateByAxisAngle(gfx_Vector3 v, gfx_Vector3 axis, float angle);

// RotateByQuaternion - Transform a vector by quaternion rotation
gfx_Vector3 gfx_Vector3_RotateByQuaternion(gfx_Vector3 v, gfx_Vector4 q);

// Scale - Scale provided vector
gfx_Vector3 gfx_Vector3_Scale(gfx_Vector3 v, float scale);

// Subtract - Subtract two vectors
gfx_Vector3 gfx_Vector3_Subtract(gfx_Vector3 v, gfx_Vector3 v2);

// SubtractValue - Subtract vector by float value
gfx_Vector3 gfx_Vector3_SubtractValue(gfx_Vector3 v, float sub);

// ToFloat - Converts Vector3 to float32 slice
gfx_Float3 gfx_Vector3_ToFloat(gfx_Vector3 v);

// Transform - Transforms a Vector3 by a given Matrix
gfx_Vector3 gfx_Vector3_Transform(gfx_Vector3 v, gfx_Matrix mat);

// Unproject - Projects a Vector3 from screen space into object space
// NOTE: We are avoiding calling other raymath functions despite available
gfx_Vector3 gfx_Vector3_Unproject(gfx_Vector3 v, gfx_Matrix projection, gfx_Matrix view);

// NewVector2 - Returns new Vector2
gfx_Vector2 gfx_NewVector2(float x, float y);

// NewVector3 - Returns new Vector3
gfx_Vector3 gfx_NewVector3(float x, float y, float z);

// NewVector4 - Returns new Vector4
gfx_Vector4 gfx_NewVector4(float x, float y, float z, float w);

// NewMatrix - Returns new Matrix
gfx_Matrix gfx_NewMatrix(float m0, float m4, float m8, float m12, float m1, float m5, float m9, float m13, float m2, float m6, float m10, float m14, float m3, float m7, float m11, float m15);

// NewMat2 - Returns new Mat2
gfx_Mat2 gfx_NewMat2(float m0, float m1, float m10, float m11);

// NewQuaternion - Returns new Quaternion
gfx_Vector4 gfx_NewQuaternion(float x, float y, float z, float w);
gfx_Color gfx_Color_Tint(gfx_Color c, gfx_Color target, so_int percent);

// NewColor - Returns new Color
gfx_Color gfx_NewColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// NewRectangle - Returns new Rectangle
gfx_Rectangle gfx_NewRectangle(float x, float y, float width, float height);

// ToInt32 converts rectangle to int32 variant
gfx_RectangleInt32 gfx_Rectangle_ToInt32(void* self);

// ToFloat32 converts rectangle to float32 variant
gfx_Rectangle gfx_RectangleInt32_ToFloat32(void* self);

// NewCamera3D - Returns new Camera3D
gfx_Camera gfx_NewCamera3D(gfx_Vector3 pos, gfx_Vector3 target, gfx_Vector3 up, float fovy);
gfx_Vector2 gfx_Texture_Size(gfx_Texture t);
