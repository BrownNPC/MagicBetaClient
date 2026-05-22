//so:include <SDL3/SDL.h>
package sdl

import (
	"solod.dev/so/c"
	"solod.dev/so/mem"
	"solod.dev/so/time"
)

//so:extern SDL_Window
type Window struct{}

//so:extern SDL_Renderer
type Renderer struct{}

//so:extern int
type Cint int

//so:extern SDL_SetAppMetadata
func SetAppMetadata(appname, appversion, appidentifier string)

//so:extern SDL_Init
func Init(InitFlags) bool

//so:extern SDL_Quit
func Quit()

//so:extern SDL_CreateWindowAndRenderer
func CreateWindowAndRenderer(title string, width, height int, windowFlags WindowFlags, window **Window, renderer **Renderer) bool

//so:extern SDL_CreateWindow
func CreateWindow(title string, width, height int, windowFlags WindowFlags) *Window

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

func (e *sdlError) Error() string { return c.String(e.str) }

func GetError() error {
	e := mem.Alloc[sdlError](mem.System)
	e.str = getError()
	return e
}

//so:extern SDL_GL_CreateContext
func GLCreateContext(*Window)

//so:extern SDL_GetWindowSizeInPixels
func GetWindowSizeInPixels(win *Window, w, h *Cint)

//so:extern SDL_GL_SwapWindow
func GLSwapWindow(*Window)

//so:extern SDL_Surface
type Surface struct {
	w, h   int
	pixels any
}

func (s Surface) Width() int  { return s.w }
func (s Surface) Height() int { return s.h }
func (s Surface) Pixels() any { return s.pixels }

//so:extern SDL_LoadSurface
func LoadSurface(path string) *Surface

//so:extern SDL_DestroySurface
func DestroySurface(*Surface)

//so:extern SDL_ConvertSurface
func ConvertSurface(src *Surface, format PixelFormat) *Surface

//so:extern SDL_IOStream
type IOStream struct{}

//so:extern SDL_IOFromFile
func IOFromFile(file string, mode string) *IOStream

//so:extern SDL_WriteIO
func WriteIO(ctx *IOStream, ptr *byte, size int) int

//so:extern SDL_CloseIO
func CloseIO(ctx *IOStream) bool
