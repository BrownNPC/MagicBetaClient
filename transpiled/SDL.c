#include "SDL.h"

// -- Variables and constants --
static SDL_AppState state = {0};

// -- Implementation --

SDL_AppResult SDL_AppInit(void** appState, int argc, char** argv) {
    if (!SDL_Init((sdl_INIT_VIDEO | sdl_INIT_GAMEPAD))) {
        SDL_Log("SDL init failed %s", so_cstr(sdl_GetError().Error(sdl_GetError().self)));
        return sdl_APP_FAILURE;
    }
    SDL_Window* window = SDL_CreateWindow("MagicBetaClient", 480, 272, ((sdl_WINDOW_OPENGL | sdl_WINDOW_RESIZABLE) | sdl_WINDOW_HIGH_PIXEL_DENSITY));
    if (window == NULL) {
        SDL_Log("SDL_CreateWindowFailed %s", so_cstr(sdl_GetError().Error(sdl_GetError().self)));
        return sdl_APP_FAILURE;
    }
    gfx_Init(window);
    MIX_Init();
    state.game.TargetFPS = 60;
    game_State_Init(&state.game);
    state.lastTime = time_Now();
    return sdl_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appState) {
    // Enable/ disable text input events.
    if (state.game.TextInputActive && !SDL_TextInputActive(gfx_Window)) {
        SDL_StartTextInput(gfx_Window);
    } else if (!state.game.TextInputActive && SDL_TextInputActive(gfx_Window)) {
        SDL_StopTextInput(gfx_Window);
    }
    time_Time now = time_Now();
    // Update/render
    // reset before frame
    state.game.InteractingWithUI = false;
    if (!game_State_Update(&state.game)) {
        return sdl_APP_SUCCESS;
    }
    // Delta time
    time_Duration frameTime = time_Time_Sub(now, state.lastTime);
    state.game.Dt = (float)(time_Duration_Seconds(frameTime));
    state.lastTime = now;
    // clear inputs after they're used.
    memcpy(state.game.Inputs, (game_Input[13]){}, sizeof(state.game.Inputs));
    // FPS cap
    time_Duration targetFrameTime = time_Second / (time_Duration)(state.game.TargetFPS);
    if (state.game.TargetFPS != 0 && frameTime < targetFrameTime) {
        time_Duration timeToSleep = targetFrameTime - frameTime;
        SDL_DelayNS(timeToSleep);
    }
    return sdl_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* e) {
    if (SDL_Event_Type(e) == (SDL_EVENT_QUIT)) {
        return sdl_APP_SUCCESS;
    } else if (SDL_Event_Type(e) == (SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)) {
        sdl_WindowEvent w = SDL_Event_Window(e);
        state.game.ScreenWidth = (float)(w.Data1);
        state.game.ScreenHeight = (float)(w.Data2);
        gfx_SetupViewport((so_int)(w.Data1), (so_int)(w.Data2));
    } else if (SDL_Event_Type(e) == (SDL_EVENT_MOUSE_MOTION)) {
        sdl_MouseMotionEvent m = SDL_Event_MouseMotion(e);
        state.game.Cursor = (gfx_Vector2){.X = m.X, .Y = m.Y};
        state.game.CursorDelta = (gfx_Vector2){.X = m.Xrel, .Y = m.Yrel};
        game_InputType typ = game_InputLook;
        // store the input
        state.game.Inputs[typ] = (game_Input){.Direction = gfx_Vector2_Normalize(state.game.CursorDelta)};
    } else if (SDL_Event_Type(e) == (SDL_EVENT_MOUSE_BUTTON_UP) || SDL_Event_Type(e) == (SDL_EVENT_MOUSE_BUTTON_DOWN)) {
        sdl_MouseButtonEvent m = SDL_Event_MouseButton(e);
        game_InputType typ = game_InputTap;
        if (m.Button == (sdl_BUTTON_LEFT)) {
            typ = game_InputTap;
            if (m.Type == SDL_EVENT_MOUSE_BUTTON_DOWN && state.game.InteractingWithUI) {
                state.game.UIDpadMode = false;
            }
        } else if (m.Button == (sdl_BUTTON_RIGHT)) {
            typ = game_InputRightClick;
        }
        state.game.Inputs[typ] = (game_Input){.Pressed = m.Type == SDL_EVENT_MOUSE_BUTTON_DOWN, .Released = m.Type == SDL_EVENT_MOUSE_BUTTON_UP};
    } else if (SDL_Event_Type(e) == (SDL_EVENT_TEXT_EDITING)) {
    } else if (SDL_Event_Type(e) == (SDL_EVENT_TEXT_INPUT)) {
        sdl_TextInputEvent t = SDL_Event_TextInput(e);
        game_InputType typ = game_InputTextInput;
        state.game.Inputs[typ] = (game_Input){.Text = sdl_TextInputEvent_Rune(t), .Pressed = true};
    } else if (SDL_Event_Type(e) == (SDL_EVENT_KEY_DOWN) || SDL_Event_Type(e) == (SDL_EVENT_KEY_UP)) {
        sdl_KeyboardEvent key = SDL_Event_Keyboard(e);
        game_InputType typ = game_InputNone;
        if (key.Key == (SDLK_F3)) {
            fmt_Printf("CurrentScreen=%d", state.game.CurrentScreeen);
        } else if (key.Key == (SDLK_ESCAPE)) {
            typ = game_InputClose;
        } else if (key.Key == (SDLK_BACKSPACE)) {
            typ = game_InputBackspace;
        } else if (key.Key == (SDLK_RIGHT) || key.Key == (SDLK_LEFT) || key.Key == (SDLK_DOWN) || key.Key == (SDLK_UP)) {
            if (state.game.InteractingWithUI) {
                state.game.UIDpadMode = true;
            }
            typ = (game_InputRight) + (game_InputType)(SDLK_RIGHT - key.Key);
        } else if (key.Key == (SDLK_RETURN)) {
            typ = game_InputReturn;
        }
        state.game.Inputs[typ] = (game_Input){.Pressed = key.Type == SDL_EVENT_KEY_DOWN, .Released = key.Type == SDL_EVENT_KEY_UP};
    } else if (SDL_Event_Type(e) == (SDL_EVENT_GAMEPAD_ADDED)) {
        sdl_GamepadDeviceEvent added = SDL_Event_GamepadDevice(e);
        SDL_OpenGamepad(added.Which);
        state.game.UIDpadMode = true;
    } else if (SDL_Event_Type(e) == (SDL_EVENT_GAMEPAD_BUTTON_UP) || SDL_Event_Type(e) == (SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        game_InputType typ = game_InputNone;
        sdl_GamepadButtonEvent btn = SDL_Event_GamepadButton(e);
        if (btn.Button == (SDL_GAMEPAD_BUTTON_DPAD_LEFT)) {
            typ = game_InputLeft;
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) {
            typ = game_InputRight;
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_DPAD_UP)) {
            typ = game_InputUp;
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_DPAD_DOWN)) {
            typ = game_InputDown;
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_SOUTH)) {
            if (state.game.InteractingWithUI) {
                typ = game_InputReturn;
            }
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_START)) {
            if (state.game.InteractingWithUI) {
                typ = game_InputReturn;
            }
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_BACK)) {
            if (state.game.InteractingWithUI) {
                typ = game_InputClose;
            }
        } else if (btn.Button == (SDL_GAMEPAD_BUTTON_EAST)) {
            if (state.game.InteractingWithUI) {
                typ = game_InputClose;
            }
        }
        if (state.game.InteractingWithUI) {
            state.game.UIDpadMode = true;
        } else {
            state.game.UIDpadMode = true;
        }
        state.game.Inputs[typ] = (game_Input){.Pressed = btn.Type == SDL_EVENT_GAMEPAD_BUTTON_DOWN, .Released = btn.Type == SDL_EVENT_GAMEPAD_BUTTON_UP};
    }
    return sdl_APP_CONTINUE;
}

void SDL_AppQuit(void* appState, SDL_AppResult result) {
}
