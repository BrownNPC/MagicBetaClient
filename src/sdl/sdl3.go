//so:include <SDL3/SDL.h>
package sdl

import (
	"solod.dev/so/c"
	"solod.dev/so/time"
)

//so:extern SDL_Window
type Window struct{}

//so:extern SDL_Renderer
type Renderer struct{}

//so:extern SDL_SetAppMetadata
func SetAppMetadata(appname, appversion, appidentifier string)

//so:extern SDL_Init
func Init(InitFlags) bool

//so:extern SDL_Quit
func Quit()

//so:extern SDL_CreateWindowAndRenderer
func CreateWindowAndRenderer(title string, width, height int, windowFlags WindowFlags, window **Window, renderer **Renderer) bool

//so:extern SDL_Log
func Log(string, ...any)

// Delay pauses the calling thread.
//
//so:extern SDL_DelayNS
func Delay(t time.Duration)

//so:extern SDL_GetError
//so:decay
func getError() *c.ConstChar

type sdlError struct{ str *c.ConstChar }

func (e sdlError) Error() string { return c.String(e.str) }

func GetError() error { return sdlError{str: getError()} }

//so:extern SDL_GL_CreateContext
func GLCreateContext(*Window)
//so:extern SDL_GetWindowSizeInPixels
func GetWindowSizeInPixels(win *Window,w,h *int)
//so:extern SDL_GL_SwapWindow
func GLSwapWindow(*Window)
