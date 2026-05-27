package SDL

import (
	"mbc/gfx"
	"mbc/net/curl"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/mem"
)

//so:embed sdl/app.h
var _ string

type AppState struct {
	window *sdl.Window
	Tex    gfx.Texture
}

func AppInit(appState *any, argc sdl.Cint, argv **c.Char) sdl.AppResult {
	var state = mem.Alloc[AppState](nil)
	*appState = state
	sdl.Init(sdl.INIT_VIDEO)
	state.window = sdl.CreateWindow("MagicBetaClient", 640, 480, sdl.WINDOW_OPENGL|sdl.WINDOW_RESIZABLE)

	gfx.Init(state.window)
	icon, err := gfx.LoadTexture("assets/icons/icon_16x16.png")
	if err != nil {
		f := sdl.IOFromFile("error.log", "w")
		if f == nil {
			panic(sdl.GetError())
		}

		_, err := f.Write([]byte(err.Error()))
		if err != nil {
			panic(err)
		}
		f.Close()
		return sdl.APP_FAILURE

	}
	state.Tex = icon
	return sdl.APP_CONTINUE
}

func AppIterate(appState any) sdl.AppResult {
	state := appState.(*AppState)
	_ = state
	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Red)
	gfx.DrawTexturePro(
		state.Tex,
		gfx.NewRectangle(0, 0, float32(state.Tex.Width), float32(state.Tex.Height)),
		gfx.NewRectangle(0, 0, float32(state.Tex.Width), float32(state.Tex.Height)),
		gfx.Vector2{},
		0, gfx.White,
	)
	gfx.EndDrawing()
	return sdl.APP_CONTINUE
}

func AppEvent(appState any, e *sdl.Event) sdl.AppResult {
	if e.Type() == sdl.EVENT_QUIT {
		return sdl.APP_SUCCESS
	}
	return sdl.APP_CONTINUE
}

func AppQuit(appState any, result sdl.AppResult) {
	mem.Free(mem.System, appState.(*AppState))
	curl.DeInit()
}
