package SDL

import (
	"mbc/net/curl"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/mem"
)

//so:embed sdl/app.h
var _ string

type AppState struct {
	window   *sdl.Window
	renderer *sdl.Renderer
}
//so:extern int
type CINT int
func AppInit(appState *any, argc CINT, argv **c.Char) sdl.AppResult {
	if !curl.Init() {
		panic("failed to init curl.")
	}

	var s = mem.Alloc[AppState](nil)

	*appState = s

	return sdl.APP_CONTINUE
}

func AppIterate(appState any) sdl.AppResult {
	return sdl.APP_CONTINUE
}

func AppEvent(appState any, e *sdl.Event) sdl.AppResult {
	if e.Type() == sdl.EVENT_QUIT {
		return sdl.APP_SUCCESS
	}
	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {
	curl.DeInit()
}
