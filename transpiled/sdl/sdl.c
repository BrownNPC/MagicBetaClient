#include "sdl.h"

// -- Types --

typedef struct sdlError sdlError;

typedef struct sdlError {
    const char* str;
} sdlError;

// -- Forward declarations --
static so_String sdlError_Error(void* self);
static bool SDL_Storage_readFile(void* self, so_String path, so_Slice dst);

// -- Variables and constants --

// -- defines.go --

// -- event.go --

SDL_EventType SDL_Event_Type(void* self) {
    SDL_Event* e = self;
    return *(SDL_EventType*)e;
}

sdl_QuitEvent SDL_Event_Quit(void* self) {
    SDL_Event* e = self;
    return *(sdl_QuitEvent*)e;
}

sdl_WindowEvent SDL_Event_Window(void* self) {
    SDL_Event* e = self;
    return *(sdl_WindowEvent*)e;
}

sdl_MouseButtonEvent SDL_Event_MouseButton(void* self) {
    SDL_Event* e = self;
    return *(sdl_MouseButtonEvent*)e;
}

sdl_MouseWheelEvent SDL_Event_MouseWheel(void* self) {
    SDL_Event* e = self;
    return *(sdl_MouseWheelEvent*)e;
}

sdl_MouseMotionEvent SDL_Event_MouseMotion(void* self) {
    SDL_Event* e = self;
    return *(sdl_MouseMotionEvent*)e;
}

sdl_KeyboardEvent SDL_Event_Keyboard(void* self) {
    SDL_Event* e = self;
    return *(sdl_KeyboardEvent*)e;
}

sdl_TextInputEvent SDL_Event_TextInput(void* self) {
    SDL_Event* e = self;
    return *(sdl_TextInputEvent*)e;
}

sdl_TextEditingEvent SDL_Event_TextEditing(void* self) {
    SDL_Event* e = self;
    return *(sdl_TextEditingEvent*)e;
}

sdl_GamepadButtonEvent SDL_Event_GamepadButton(void* self) {
    SDL_Event* e = self;
    return *(sdl_GamepadButtonEvent*)e;
}

sdl_GamepadDeviceEvent SDL_Event_GamepadDevice(void* self) {
    SDL_Event* e = self;
    return *(sdl_GamepadDeviceEvent*)e;
}

so_rune sdl_TextInputEvent_Rune(sdl_TextInputEvent e) {
    return so_at(so_rune, so_string_runes(c_String(const char, (e.text))), 0);
}

so_String sdl_TextEditingEvent_Text(sdl_TextEditingEvent e) {
    return c_String(const char, (e.text));
}

// -- sdl3.go --

static so_String sdlError_Error(void* self) {
    sdlError* e = self;
    return c_String(const char, (e->str));
}

so_Error sdl_GetError(void) {
    sdlError* e = mem_Alloc(sdlError, (mem_System));
    e->str = SDL_GetError();
    return (so_Error){.self = e, .Error = sdlError_Error};
}

so_int SDL_Surface_Width(SDL_Surface s) {
    return s.w;
}

so_int SDL_Surface_Height(SDL_Surface s) {
    return s.h;
}

so_int SDL_Surface_Pitch(SDL_Surface s) {
    return s.pitch;
}

uint8_t* SDL_Surface_Pixels(SDL_Surface s) {
    return s.pixels;
}

so_R_int_err SDL_IOStream_Read(void* self, so_Slice b) {
    SDL_IOStream* ctx = self;
    if (so_len(b) == 0) {
        return (so_R_int_err){.val = 0, .err = (so_Error){0}};
    }
    so_int n = SDL_ReadIO(ctx, &so_at(so_byte, b, 0), so_len(b));
    if (n == 0) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

so_R_int_err SDL_IOStream_Write(void* self, so_Slice b) {
    SDL_IOStream* ctx = self;
    if (so_len(b) == 0) {
        return (so_R_int_err){.val = 0, .err = (so_Error){0}};
    }
    so_int n = SDL_WriteIO(ctx, &so_at(so_byte, b, 0), so_len(b));
    if (n != so_len(b)) {
        return (so_R_int_err){.val = n, .err = io_EOF};
    }
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

so_Error SDL_IOStream_Close(void* self) {
    SDL_IOStream* ctx = self;
    if (!SDL_CloseIO(ctx)) {
        return sdl_GetError();
    }
    return (so_Error){0};
}

so_String sdl_GetBasePath(void) {
    return c_String(const char, (SDL_GetBasePath()));
}

so_String sdl_GetPlatform(void) {
    return c_String(const char, (SDL_GetPlatform()));
}

bool SDL_Storage_Ready(void* self) {
    SDL_Storage* storage = self;
    return SDL_StorageReady(storage);
}

so_R_int_err SDL_Storage_FileSize(void* self, so_String path) {
    SDL_Storage* storage = self;
    uint64_t size = 0;
    bool ok = SDL_GetStorageFileSize(storage, so_cstr(path), &size);
    if (ok) {
        return (so_R_int_err){.val = (so_int)(size), .err = (so_Error){0}};
    }
    return (so_R_int_err){.val = (so_int)(size), .err = sdl_GetError()};
}

// Returns nil byte slice if there's an error
so_R_slice_err SDL_Storage_ReadFile(void* self, mem_Allocator a, so_String path) {
    SDL_Storage* storage = self;
    so_R_int_err _res1 = SDL_Storage_FileSize(storage, path);
    so_int size = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (so_R_slice_err){.val = NULL, .err = err};
    }
    // allocate file memory
    so_Slice fileMem = mem_AllocSlice(so_byte, (a), (size), (size));
    // read the file
    {
        bool ok = SDL_Storage_readFile(storage, path, fileMem);
        if (ok) {
            return (so_R_slice_err){.val = fileMem, .err = (so_Error){0}};
        }
    }
    // free file memory if we can
    if (fileMem.ptr != NULL) {
        mem_FreeSlice(so_byte, (a), (fileMem));
    }
    return (so_R_slice_err){.val = NULL, .err = sdl_GetError()};
}

// dst must be long enough to hold the file
static bool SDL_Storage_readFile(void* self, so_String path, so_Slice dst) {
    SDL_Storage* s = self;
    return SDL_ReadStorageFile(s, so_cstr(path), &so_at(so_byte, dst, 0), (uint64_t)(so_len(dst)));
}

so_Error SDL_Storage_WriteFile(void* self, so_String path, so_Slice src) {
    SDL_Storage* storage = self;
    if (!SDL_WriteStorageFile(storage, so_cstr(path), &so_at(so_byte, src, 0), (uint64_t)(so_len(src)))) {
        return sdl_GetError();
    }
    return (so_Error){0};
}

bool SDL_Storage_Close(void* self) {
    SDL_Storage* s = self;
    return SDL_CloseStorage(s);
}

// -- sdl_enums_events.go --

// -- sdl_enums_gamepad_button.go --

// -- sdl_enums_glattr.go --

// -- sdl_enums_keycodes.go --

// -- sdl_enums_scancodes.go --
