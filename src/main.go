package SDL

import (
	"mbc/game"
	"mbc/gfx"
	"mbc/net/curl"
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

func AppInit(appState *any, argc sdl.Cint, argv **c.Char) sdl.AppResult {
	sdl.Init(sdl.INIT_VIDEO)

	window := sdl.CreateWindow("MagicBetaClient", 480, 272, sdl.WINDOW_OPENGL|
		sdl.WINDOW_RESIZABLE|
		sdl.WINDOW_HIGH_PIXEL_DENSITY)
	gfx.Init(window)
	state.game.Init()

	return sdl.APP_CONTINUE
}

func AppIterate(appState any) sdl.AppResult {
	{ // calculate delta time
		currentTime := time.Now()
		state.game.Dt = float32(time.Since(currentTime).Seconds())
		state.lastTime = currentTime
	}

	if !state.game.Update() {
		return sdl.APP_SUCCESS
	}
	return sdl.APP_CONTINUE
}

func AppEvent(appState any, e *sdl.Event) sdl.AppResult {
	switch e.Type() {
	case sdl.EVENT_QUIT:
		return sdl.APP_SUCCESS
	case sdl.EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		w := e.Window()
		state.game.ScreenWidth, state.game.ScreenHeight = int(w.Data1), int(w.Data2)
		gfx.SetupViewport(int(w.Data1), int(w.Data2))
	}
	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {
	curl.DeInit()
}
