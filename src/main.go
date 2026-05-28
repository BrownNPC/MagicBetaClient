package SDL

import (
	"mbc/game"
	"mbc/gfx"
	"mbc/net/curl"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/mem"
	"solod.dev/so/time"
)

//so:embed sdl/app.h
var _ string

type AppState struct {
	lastTime time.Time
	g        game.State
}

func AppInit(appState *any, argc sdl.Cint, argv **c.Char) sdl.AppResult {
	var state = mem.Alloc[AppState](nil)
	*appState = state
	sdl.Init(sdl.INIT_VIDEO)

	window := sdl.CreateWindow("MagicBetaClient", 640, 480, sdl.WINDOW_OPENGL|sdl.WINDOW_RESIZABLE)

	gfx.Init(window)
	return sdl.APP_CONTINUE
}

func AppIterate(appState any) sdl.AppResult {
	state := appState.(*AppState)
	{ // calculate delta time
		currentTime := time.Now()
		state.g.Dt = float32(time.Since(currentTime).Seconds())
		state.lastTime = currentTime
	}

	if !state.g.Update() {
		return sdl.APP_SUCCESS
	}
	return sdl.APP_CONTINUE
}

func AppEvent(appState any, e *sdl.Event) sdl.AppResult {
	if e.Type() == sdl.EVENT_QUIT {
		return sdl.APP_SUCCESS
	}
	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {
	state := appState.(*AppState)
	mem.Free(mem.System, state)
	curl.DeInit()
}
