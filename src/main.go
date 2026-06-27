package SDL

import (
	"mbc/game"
	"mbc/gfx"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/time"
)

//so:embed sdl/app.h
var _ string

type AppState struct {
	lastTime time.Time
	game     game.State
}

var state AppState

func AppInit(appState *any, argc c.Int, argv **c.Char) sdl.AppResult {
	if !sdl.Init(sdl.INIT_VIDEO | sdl.INIT_GAMEPAD) {
		sdl.Log("SDL init failed %s", sdl.GetError().Error())
		return sdl.APP_FAILURE
	}
	window := sdl.CreateWindow("MagicBetaClient", 480, 272,
		sdl.WINDOW_OPENGL|
			sdl.WINDOW_RESIZABLE|
			sdl.WINDOW_HIGH_PIXEL_DENSITY)
	if window == nil {
		sdl.Log("SDL_CreateWindowFailed %s", sdl.GetError().Error())
		return sdl.APP_FAILURE
	}
	gfx.Init(window)
	mix.Init()
	state.game.TargetFPS = 60
	state.game.Init()
	state.lastTime = time.Now()

	return sdl.APP_CONTINUE
}

func AppIterate(appState any) sdl.AppResult {
	// Enable/ disable text input events.
	if state.game.TextInputActive && !sdl.TextInputActive(gfx.Window) {
		sdl.StartTextInput(gfx.Window)
	} else if !state.game.TextInputActive && sdl.TextInputActive(gfx.Window) {
		sdl.StopTextInput(gfx.Window)
	}

	now := time.Now()
	// Update/render
	if !state.game.Update() {
		return sdl.APP_SUCCESS
	}

	// Delta time
	frameTime := now.Sub(state.lastTime)
	state.game.Dt = float32(frameTime.Seconds())
	state.lastTime = now
	state.game.Inputs = [game.TotalInputs]game.Input{} // clear inputs after they're used.

	// FPS cap
	targetFrameTime := time.Second / time.Duration(state.game.TargetFPS)

	if state.game.TargetFPS != 0 && frameTime < targetFrameTime {
		timeToSleep := targetFrameTime - frameTime
		sdl.Delay(timeToSleep)
	}

	return sdl.APP_CONTINUE
}

func AppEvent(appState any, e *sdl.Event) sdl.AppResult {
	switch e.Type() {
	case sdl.EVENT_QUIT:
		return sdl.APP_SUCCESS

	case sdl.EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		w := e.Window()
		state.game.ScreenWidth, state.game.ScreenHeight = float32(w.Data1), float32(w.Data2)
		gfx.SetupViewport(int(w.Data1), int(w.Data2))

	case sdl.EVENT_MOUSE_MOTION:
		m := e.MouseMotion()
		state.game.Cursor = gfx.Vector2{X: m.X, Y: m.Y}
		state.game.CursorDelta = gfx.Vector2{X: m.Xrel, Y: m.Yrel}

		typ := game.InputLook
		// store the input
		state.game.Inputs[typ] =
			game.Input{
				Type:      typ,
				Direction: state.game.CursorDelta.Normalize(),
			}

	case sdl.EVENT_MOUSE_BUTTON_UP, sdl.EVENT_MOUSE_BUTTON_DOWN:
		m := e.MouseButton()
		typ := game.InputLeftClick
		switch m.Button {
		case sdl.BUTTON_LEFT:
			typ = game.InputLeftClick
		case sdl.BUTTON_RIGHT:
			typ = game.InputRightClick
		}
		state.game.Inputs[typ] = game.Input{
			Type:     typ,
			Pressed:  m.Type == sdl.EVENT_MOUSE_BUTTON_DOWN,
			Released: m.Type == sdl.EVENT_MOUSE_BUTTON_UP,
		}

	case sdl.EVENT_TEXT_INPUT:
		t := e.TextInput()
		typ := game.InputTextInput
		state.game.Inputs[typ] = game.Input{
			Type:    typ,
			Text:    t.Rune(),
			Pressed: true,
		}

	case sdl.EVENT_KEY_DOWN, sdl.EVENT_KEY_UP:
		key := e.Keyboard()
		if key.Key == sdl.KeyESCAPE {
			state.game.Inputs[game.InputClose] = game.Input{
				Type:     game.InputClose,
				Pressed:  key.Type == sdl.EVENT_KEY_DOWN,
				Released: key.Type == sdl.EVENT_KEY_UP,
			}
		}
		if key.Key == sdl.KeyBACKSPACE {
			state.game.Inputs[game.InputBackspace] = game.Input{
				Type:     game.InputClose,
				Pressed:  key.Type == sdl.EVENT_KEY_DOWN,
				Released: key.Type == sdl.EVENT_KEY_UP,
			}
		}
	}

	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {
}
