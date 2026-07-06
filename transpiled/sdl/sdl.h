#pragma once
#include "so/builtin/builtin.h"
#include <SDL3/SDL.h>
#include "so/c/c.h"
#include "so/io/io.h"
#include "so/mem/mem.h"
#include "so/time/time.h"

// -- Types --

typedef struct sdl_QuitEvent sdl_QuitEvent;
typedef struct sdl_WindowEvent sdl_WindowEvent;
typedef struct sdl_MouseButtonEvent sdl_MouseButtonEvent;
typedef struct sdl_MouseWheelEvent sdl_MouseWheelEvent;
typedef struct sdl_MouseMotionEvent sdl_MouseMotionEvent;
typedef struct sdl_KeyboardEvent sdl_KeyboardEvent;
typedef struct sdl_TextInputEvent sdl_TextInputEvent;
typedef struct sdl_GamepadButtonEvent sdl_GamepadButtonEvent;
typedef struct sdl_GamepadDeviceEvent sdl_GamepadDeviceEvent;
typedef struct sdl_TextEditingEvent sdl_TextEditingEvent;
typedef uint32_t sdl_InitFlags;
typedef uint64_t sdl_WindowFlags;
typedef so_int sdl_PixelFormat;
typedef so_int sdl_IOStatus;

typedef void (*sdl_AudioCallback)(void*, so_byte*, so_int);

typedef struct sdl_QuitEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
} sdl_QuitEvent;

typedef struct sdl_WindowEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    int32_t Data1;
    int32_t Data2;
} sdl_WindowEvent;

typedef struct sdl_MouseButtonEvent {
    SDL_EventType Type;
    int32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    uint32_t Which;
    uint8_t Button;
    bool Down;
    uint8_t Clicks;
    uint8_t Padding;
    float X;
    float Y;
} sdl_MouseButtonEvent;

typedef struct sdl_MouseWheelEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    uint32_t Which;
    float X;
    float Y;
    uint32_t Direction;
    float MouseX;
    float MouseY;
    int32_t IntegerX;
    int32_t IntegerY;
} sdl_MouseWheelEvent;

typedef struct sdl_MouseMotionEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    uint32_t Which;
    uint32_t State;
    float X;
    float Y;
    float Xrel;
    float Yrel;
} sdl_MouseMotionEvent;

typedef struct sdl_KeyboardEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    uint32_t Which;
    uint32_t Scancode;
    uint32_t Key;
    uint32_t Mod;
    uint16_t Raw;
    bool Down;
    bool Repeat;
} sdl_KeyboardEvent;

typedef struct sdl_TextInputEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    const char* text;
} sdl_TextInputEvent;

typedef struct sdl_GamepadButtonEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t Which;
    uint8_t Button;
    bool Down;
    so_byte padding1;
    so_byte padding2;
} sdl_GamepadButtonEvent;

typedef struct sdl_GamepadDeviceEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t Which;
} sdl_GamepadDeviceEvent;

typedef struct sdl_TextEditingEvent {
    SDL_EventType Type;
    uint32_t reserved;
    time_Duration Timestamp;
    uint32_t WindowID;
    const char* text;
    int32_t Start;
    int32_t End;
} sdl_TextEditingEvent;

// -- Variables and constants --
static const sdl_InitFlags sdl_INIT_AUDIO = 0x00000010;
static const sdl_InitFlags sdl_INIT_VIDEO = 0x00000020;
static const sdl_InitFlags sdl_INIT_JOYSTICK = 0x00000200;
static const sdl_InitFlags sdl_INIT_HAPTIC = 0x00001000;
static const sdl_InitFlags sdl_INIT_GAMEPAD = 0x00002000;
static const sdl_InitFlags sdl_INIT_EVENTS = 0x00004000;
static const sdl_InitFlags sdl_INIT_SENSOR = 0x00008000;
static const sdl_InitFlags sdl_INIT_CAMERA = 0x00010000;
static const sdl_WindowFlags sdl_WINDOW_FULLSCREEN = (0x0000000000000001);
static const sdl_WindowFlags sdl_WINDOW_OPENGL = (0x0000000000000002);
static const sdl_WindowFlags sdl_WINDOW_OCCLUDED = (0x0000000000000004);
static const sdl_WindowFlags sdl_WINDOW_HIDDEN = (0x0000000000000008);
static const sdl_WindowFlags sdl_WINDOW_BORDERLESS = (0x0000000000000010);
static const sdl_WindowFlags sdl_WINDOW_RESIZABLE = (0x0000000000000020);
static const sdl_WindowFlags sdl_WINDOW_MINIMIZED = (0x0000000000000040);
static const sdl_WindowFlags sdl_WINDOW_MAXIMIZED = (0x0000000000000080);
static const sdl_WindowFlags sdl_WINDOW_MOUSE_GRABBED = (0x0000000000000100);
static const sdl_WindowFlags sdl_WINDOW_INPUT_FOCUS = (0x0000000000000200);
static const sdl_WindowFlags sdl_WINDOW_MOUSE_FOCUS = (0x0000000000000400);
static const sdl_WindowFlags sdl_WINDOW_EXTERNAL = (0x0000000000000800);
static const sdl_WindowFlags sdl_WINDOW_MODAL = (0x0000000000001000);
static const sdl_WindowFlags sdl_WINDOW_HIGH_PIXEL_DENSITY = (0x0000000000002000);
static const sdl_WindowFlags sdl_WINDOW_MOUSE_CAPTURE = (0x0000000000004000);
static const sdl_WindowFlags sdl_WINDOW_MOUSE_RELATIVE_MODE = (0x0000000000008000);
static const sdl_WindowFlags sdl_WINDOW_ALWAYS_ON_TOP = (0x0000000000010000);
static const sdl_WindowFlags sdl_WINDOW_UTILITY = (0x0000000000020000);
static const sdl_WindowFlags sdl_WINDOW_TOOLTIP = (0x0000000000040000);
static const sdl_WindowFlags sdl_WINDOW_POPUP_MENU = (0x0000000000080000);
static const sdl_WindowFlags sdl_WINDOW_KEYBOARD_GRABBED = (0x0000000000100000);
static const sdl_WindowFlags sdl_WINDOW_FILL_DOCUMENT = (0x0000000000200000);
static const sdl_WindowFlags sdl_WINDOW_VULKAN = (0x0000000010000000);
static const sdl_WindowFlags sdl_WINDOW_METAL = (0x0000000020000000);
static const sdl_WindowFlags sdl_WINDOW_TRANSPARENT = (0x0000000040000000);
static const sdl_WindowFlags sdl_WINDOW_NOT_FOCUSABLE = (0x0000000080000000);
static const int64_t sdl_BUTTON_LEFT = 1;
static const int64_t sdl_BUTTON_MIDDLE = 2;
static const int64_t sdl_BUTTON_RIGHT = 3;
static const int64_t sdl_BUTTON_X1 = 4;
static const int64_t sdl_BUTTON_X2 = 5;
static const SDL_AppResult sdl_APP_CONTINUE = 0;
static const SDL_AppResult sdl_APP_SUCCESS = 1;
static const SDL_AppResult sdl_APP_FAILURE = 2;
static const int64_t sdl_GL_CONTEXT_PROFILE_CORE = 0x0001;
static const int64_t sdl_GL_CONTEXT_PROFILE_COMPATIBILITY = 0x0002;
static const int64_t sdl_GL_CONTEXT_PROFILE_ES = 0x0004;
static const int64_t sdl_GL_RED_SIZE = 0;
static const int64_t sdl_GL_GREEN_SIZE = 1;
static const int64_t sdl_GL_BLUE_SIZE = 2;
static const int64_t sdl_GL_ALPHA_SIZE = 3;
static const int64_t sdl_GL_BUFFER_SIZE = 4;
static const int64_t sdl_GL_DOUBLEBUFFER = 5;
static const int64_t sdl_GL_DEPTH_SIZE = 6;
static const int64_t sdl_GL_STENCIL_SIZE = 7;
static const int64_t sdl_GL_ACCUM_RED_SIZE = 8;
static const int64_t sdl_GL_ACCUM_GREEN_SIZE = 9;
static const int64_t sdl_GL_ACCUM_BLUE_SIZE = 10;
static const int64_t sdl_GL_ACCUM_ALPHA_SIZE = 11;
static const int64_t sdl_GL_STEREO = 12;
static const int64_t sdl_GL_MULTISAMPLEBUFFERS = 13;
static const int64_t sdl_GL_MULTISAMPLESAMPLES = 14;
static const int64_t sdl_GL_ACCELERATED_VISUAL = 15;
static const int64_t sdl_GL_RETAINED_BACKING = 16;
static const int64_t sdl_GL_CONTEXT_MAJOR_VERSION = 17;
static const int64_t sdl_GL_CONTEXT_MINOR_VERSION = 18;
static const int64_t sdl_GL_CONTEXT_FLAGS = 19;
static const int64_t sdl_GL_CONTEXT_PROFILE_MASK = 20;
static const int64_t sdl_GL_SHARE_WITH_CURRENT_CONTEXT = 21;
static const int64_t sdl_GL_FRAMEBUFFER_SRGB_CAPABLE = 22;
static const int64_t sdl_GL_CONTEXT_RELEASE_BEHAVIOR = 23;
static const int64_t sdl_GL_CONTEXT_RESET_NOTIFICATION = 24;
static const int64_t sdl_GL_CONTEXT_NO_ERROR = 25;
static const int64_t sdl_GL_FLOATBUFFERS = 26;
static const int64_t sdl_GL_EGL_PLATFORM = 27;

// -- Functions and methods --
SDL_EventType SDL_Event_Type(void* self);
sdl_QuitEvent SDL_Event_Quit(void* self);
sdl_WindowEvent SDL_Event_Window(void* self);
sdl_MouseButtonEvent SDL_Event_MouseButton(void* self);
sdl_MouseWheelEvent SDL_Event_MouseWheel(void* self);
sdl_MouseMotionEvent SDL_Event_MouseMotion(void* self);
sdl_KeyboardEvent SDL_Event_Keyboard(void* self);
sdl_TextInputEvent SDL_Event_TextInput(void* self);
sdl_TextEditingEvent SDL_Event_TextEditing(void* self);
sdl_GamepadButtonEvent SDL_Event_GamepadButton(void* self);
sdl_GamepadDeviceEvent SDL_Event_GamepadDevice(void* self);
so_rune sdl_TextInputEvent_Rune(sdl_TextInputEvent e);
so_String sdl_TextEditingEvent_Text(sdl_TextEditingEvent e);
so_Error sdl_GetError(void);
so_int SDL_Surface_Width(SDL_Surface s);
so_int SDL_Surface_Height(SDL_Surface s);
so_int SDL_Surface_Pitch(SDL_Surface s);
uint8_t* SDL_Surface_Pixels(SDL_Surface s);
so_R_int_err SDL_IOStream_Read(void* self, so_Slice b);
so_R_int_err SDL_IOStream_Write(void* self, so_Slice b);
so_Error SDL_IOStream_Close(void* self);
so_String sdl_GetBasePath(void);
so_String sdl_GetPlatform(void);
bool SDL_Storage_Ready(void* self);
so_R_int_err SDL_Storage_FileSize(void* self, so_String path);

// Returns nil byte slice if there's an error
so_R_slice_err SDL_Storage_ReadFile(void* self, mem_Allocator a, so_String path);
so_Error SDL_Storage_WriteFile(void* self, so_String path, so_Slice src);
bool SDL_Storage_Close(void* self);
