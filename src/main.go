package SDL

import (
	"mbc/game"
	"mbc/gfx"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/fmt"
	"solod.dev/so/time"
)

//so:embed sdl/app.h
var _ string

type AppState struct {
	lastTime time.Time
	game     game.State

	wireframeMode bool

	// The PSP does not have a right analog stick. or triggers.
	// This boolean selects between two control schemes
	//
	// Modes should be toggled with left bumper.
	//
	// False:
	//  Left Stick: Move
	//  Right Bumper: Attack
	// True:
	//  Left Stick: Look
	//  Right Bumper: Use
	LeftStickFunctionToggled bool
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
	state.game.InteractingWithUI = false // reset before frame
	gfx.SetMouseLock(state.game.MouseLock)

	// use gamepad axis to set Move inputs.
	if pad := sdl.GetGamepadFromPlayerIndex(0); pad != nil {
		const deadzone = 8 / 100.0 // 8% deadzone
		move := gfx.NewVector2(
			float32(sdl.GetGamepadAxis(pad, sdl.GAMEPAD_AXIS_LEFTX))/32768,
			float32(sdl.GetGamepadAxis(pad, sdl.GAMEPAD_AXIS_LEFTY))/32768,
		)
		if length := move.Length(); length < deadzone {
			move = gfx.NewVector2(0, 0)
		} else {
			// scale it to 0-1
			scale := (length - deadzone) / (1.0 - deadzone)
			move = move.Normalize().Scale(scale)
			// sdl uses negatives for up/left
			// so we flip it before passing to game
			state.game.GamepadMovement = move.Scale(-1)
			state.game.IsMovingWithGamepad = true
		}
	}
	if !state.game.Update() {
		return sdl.APP_SUCCESS
	}
	state.game.GamepadMovement = gfx.NewVector2(0, 0)
	state.game.IsMovingWithGamepad = false

	// Delta time
	frameTime := now.Sub(state.lastTime)
	state.game.Dt = float32(frameTime.Seconds())
	state.game.FrameTime = frameTime
	state.lastTime = now
	// update input state
	state.game.InputsPrevFrame = state.game.Inputs
	for i := range state.game.Inputs {
		state.game.Inputs[i].Text = 0
		state.game.Inputs[i].Direction = gfx.Vector2{}
	}
	state.game.Inputs[game.InputLook].Down = false
	state.game.Inputs[game.InputTextInput].Down = false
	// FPS cap
	targetFrameTime := time.Second / time.Duration(state.game.TargetFPS)
	state.game.TargetFrameTime = targetFrameTime

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

		typ := game.InputLook
		state.game.Inputs[typ].Direction = gfx.Vector2{X: m.Xrel, Y: m.Yrel}
		state.game.Inputs[typ].Down = true

	case sdl.EVENT_MOUSE_BUTTON_UP, sdl.EVENT_MOUSE_BUTTON_DOWN:
		m := e.MouseButton()
		typ := game.InputTap
		switch m.Button {
		case sdl.BUTTON_LEFT:
			typ = game.InputTap
			if m.Type == sdl.EVENT_MOUSE_BUTTON_DOWN && state.game.InteractingWithUI {
				state.game.UIDpadMode = false
			}
		case sdl.BUTTON_RIGHT:
			typ = game.InputRightClick
		}
		state.game.Inputs[typ].Down = m.Type == sdl.EVENT_MOUSE_BUTTON_DOWN
		state.game.Inputs[typ].Up = m.Type == sdl.EVENT_MOUSE_BUTTON_UP
	case sdl.EVENT_TEXT_EDITING:
	case sdl.EVENT_TEXT_INPUT:
		t := e.TextInput()
		typ := game.InputTextInput
		state.game.Inputs[typ].Text = t.Rune()
		state.game.Inputs[typ].Down = true

	case sdl.EVENT_KEY_DOWN, sdl.EVENT_KEY_UP:
		key := e.Keyboard()
		typ := game.InputNone
		switch key.Key {
		case sdl.KeyF3:
			if key.Type == sdl.EVENT_KEY_UP {
				state.wireframeMode = !state.wireframeMode
				gfx.SetWireframeMode(state.wireframeMode)
				fmt.Printf("CurrentScreen=%d", state.game.CurrentScreeen)
			}
		case sdl.KeyESCAPE:
			typ = game.InputClose
		case sdl.KeyBACKSPACE:
			typ = game.InputBackspace
		case sdl.KeyRIGHT, sdl.KeyLEFT, sdl.KeyDOWN, sdl.KeyUP:
			if state.game.InteractingWithUI {
				state.game.UIDpadMode = true
			}
			typ = (game.InputRight) + game.InputType(sdl.KeyRIGHT-key.Key)
		case sdl.KeyW, sdl.KeyA, sdl.KeyS, sdl.KeyD:
			switch key.Key {
			case sdl.KeyW:
				typ = game.InputMoveForward
			case sdl.KeyS:
				typ = game.InputMoveBackward
			case sdl.KeyD:
				typ = game.InputMoveRight
			case sdl.KeyA:
				typ = game.InputMoveLeft
			}
		case sdl.KeySPACE:
			typ = game.InputJump
		case sdl.KeyLSHIFT:
			typ = game.InputSneak
		case sdl.KeyRETURN:
			typ = game.InputReturn
		}

		if typ != game.InputNone {
			state.game.Inputs[typ].Down = key.Type == sdl.EVENT_KEY_DOWN
			state.game.Inputs[typ].Up = key.Type == sdl.EVENT_KEY_UP
		}
	case sdl.EVENT_GAMEPAD_ADDED:
		added := e.GamepadDevice()
		sdl.OpenGamepad(added.Which)
		state.game.UIDpadMode = true

	case sdl.EVENT_GAMEPAD_BUTTON_UP, sdl.EVENT_GAMEPAD_BUTTON_DOWN:
		typ := game.InputNone
		btn := e.GamepadButton()
		switch btn.Button {
		case sdl.GAMEPAD_BUTTON_DPAD_LEFT:
			typ = game.InputLeft
		case sdl.GAMEPAD_BUTTON_DPAD_RIGHT:
			typ = game.InputRight
		case sdl.GAMEPAD_BUTTON_DPAD_UP:
			typ = game.InputUp
		case sdl.GAMEPAD_BUTTON_DPAD_DOWN:
			typ = game.InputDown
		case sdl.GAMEPAD_BUTTON_SOUTH:
			if state.game.InteractingWithUI {
				typ = game.InputReturn
			}
		case sdl.GAMEPAD_BUTTON_START:
			if state.game.InteractingWithUI {
				typ = game.InputReturn
			}
		case sdl.GAMEPAD_BUTTON_BACK:
			if state.game.InteractingWithUI {
				typ = game.InputClose
			}
		case sdl.GAMEPAD_BUTTON_EAST:
			if state.game.InteractingWithUI {
				typ = game.InputClose
			}
		case sdl.GAMEPAD_BUTTON_LEFT_SHOULDER:
			if sdl.IsPlatformPSP() && btn.Type == sdl.EVENT_GAMEPAD_BUTTON_UP {
				// should toggle control scheme for bumpers.
				state.LeftStickFunctionToggled = !state.LeftStickFunctionToggled
				// play some sound or vibrate or something to indicate the
				// mode switch
			}
		}
		if typ != game.InputNone {
			state.game.UIDpadMode = state.game.InteractingWithUI
			state.game.Inputs[typ].Down = btn.Type == sdl.EVENT_GAMEPAD_BUTTON_DOWN
			state.game.Inputs[typ].Up = btn.Type == sdl.EVENT_GAMEPAD_BUTTON_UP
		}
	}
	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {}
