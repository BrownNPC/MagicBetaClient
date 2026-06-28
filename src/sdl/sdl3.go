//so:include <SDL3/SDL.h>
package sdl

import (
	"solod.dev/so/c"
	"solod.dev/so/io"
	"solod.dev/so/mem"
	"solod.dev/so/time"
)

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

//so:extern SDL_LogError
func LogError(int, string, ...any)

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
func GetWindowSizeInPixels(win *Window, w, h *c.Int)

//so:extern SDL_GL_GetProcAddress
func GLGetProcAddress(proc string) any

//so:extern SDL_GL_SwapWindow
func GLSwapWindow(*Window)


func (s Surface) Width() int     { return s.w }
func (s Surface) Height() int    { return s.h }
func (s Surface) Pitch() int     { return s.pitch }
func (s Surface) Pixels() *uint8 { return s.pixels }

//so:extern SDL_LoadSurface
func LoadSurface(path string) *Surface

//so:extern SDL_DestroySurface
func DestroySurface(*Surface)

//so:extern SDL_ConvertSurface
func ConvertSurface(src *Surface, format PixelFormat) *Surface


func (ctx *IOStream) Read(b []byte) (int, error) {
	if len(b) == 0 {
		return 0, nil
	}
	n := ReadIO(ctx, &b[0], len(b))
	if n == 0 {
		return 0, io.EOF
	}
	return n, nil
}
func (ctx *IOStream) Write(b []byte) (int, error) {
	if len(b) == 0 {
		return 0, nil
	}
	n := WriteIO(ctx, &b[0], len(b))
	if n != len(b) {
		return n, io.EOF
	}
	return n, nil
}
func (ctx *IOStream) Close() error {
	if !CloseIO(ctx) {
		return GetError()
	}
	return nil
}

//so:extern SDL_IOFromFile
func IOFromFile(file string, mode string) *IOStream

//so:extern SDL_WriteIO
func WriteIO(ctx *IOStream, ptr *byte, size int) int

//so:extern SDL_ReadIO
func ReadIO(ctx *IOStream, ptr *byte, size int) int

//so:extern SDL_CloseIO
func CloseIO(ctx *IOStream) bool

//so:extern SDL_GetIOStatus
func GetIOStatus(ctx *IOStream) IOStatus

//so:extern SDL_StartTextInput
func StartTextInput(*Window) bool

//so:extern SDL_StopTextInput
func StopTextInput(*Window) bool

//so:extern SDL_GetBasePath
func getBasePath() *c.ConstChar

func GetBasePath() string {
	return c.String(getBasePath())
}

//so:extern SDL_GetPlatform
func getPlatform() *c.ConstChar

func GetPlatform() string {
	return c.String(getPlatform())
}

//so:extern SDL_TextInputActive
func TextInputActive(*Window) bool

//so:extern SDL_OpenTitleStorage
func OpenTitleStorage(override string, proprs uint32) *Storage

//so:extern SDL_OpenUserStorage
func OpenUserStorage(org, app string, proprs uint32) *Storage

//so:extern SDL_StorageReady
func storageReady(*Storage) bool
func (storage *Storage) Ready() bool { return storageReady(storage) }

//so:extern SDL_GetStorageFileSize
func storageFileSize(s *Storage, path string, length *uint64) bool

func (storage *Storage) FileSize(path string) (int, error) {
	var size uint64
	ok := storageFileSize(storage, path, &size)
	if ok {
		return int(size), nil
	}
	return int(size), GetError()
}

// Returns nil byte slice if there's an error
func (storage *Storage) ReadFile(a mem.Allocator, path string) ([]byte, error) {
	size, err := storage.FileSize(path)
	if err != nil {
		return nil, err
	}
	// allocate file memory
	fileMem := mem.AllocSlice[byte](a, size, size)
	// read the file
	if ok := storage.readFile(path, fileMem); ok {
		return fileMem, nil
	}
	// free file memory if we can
	if fileMem != nil {
		mem.FreeSlice(a, fileMem)
	}
	return nil, GetError()

}

//so:extern SDL_ReadStorageFile
func readStorageFile(s *Storage, path string, dst any, len uint64) bool

// dst must be long enough to hold the file
func (s *Storage) readFile(path string, dst []byte) bool {
	return readStorageFile(s, path, &dst[0], uint64(len(dst)))
}

//so:extern SDL_WriteStorageFile
func writeStorageFile(s *Storage, path string, src any, len uint64) bool

func (storage *Storage) WriteFile(path string, src []byte) error {
	if !writeStorageFile(storage, path, &src[0], uint64(len(src))) {
		return GetError()
	}
	return nil
}

//so:extern SDL_CloseStorage
func closeStorage(*Storage) bool

func (s *Storage) Close() bool {
	return closeStorage(s)
}

//so:extern SDL_GL_SetAttribute
func GLSetAttribute(attr int, value int) bool

//so:extern SDL_OpenGamepad
func OpenGamepad(id uint32) *Gamepad
