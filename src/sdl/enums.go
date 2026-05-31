package sdl

/**
 * The types of events that can be delivered.
 *
 * \since This enum is available since SDL 3.2.0.
 */

//so:include <SDL3/SDL.h>

//so:extern SDL_EventType
type EventType uint32

//so:extern SDL_AppResult
type AppResult uint32

const (
	APP_CONTINUE AppResult = iota /**< Value that requests that the app continue from the main callbacks. */
	APP_SUCCESS                   /**< Value that requests termination with success from the main callbacks. */
	APP_FAILURE                   /**< Value that requests termination with error from the main callbacks. */
)

type PixelFormat int

//so:extern SDL_PIXELFORMAT_RGBA32
const PIXELFORMAT_RGBA32 PixelFormat = iota
//so:extern SDL_PIXELFORMAT_ABGR4444 
const PIXELFORMAT_ABGR4444 PixelFormat = iota

type IOStatus int

//so:extern SDL_IO_STATUS_READY
const IO_STATUS_READY IOStatus = iota /**< Everything is ready (no errors and not EOF). */
//so:extern SDL_IO_STATUS_ERROR
const IO_STATUS_ERROR IOStatus = iota /**< Read or write I/O error */
//so:extern SDL_IO_STATUS_EOF
const IO_STATUS_EOF IOStatus = iota /**< End of file */
//so:extern SDL_IO_STATUS_NOT_READY
const IO_STATUS_NOT_READY IOStatus = iota /**< Non blocking I/O, not ready */
//so:extern SDL_IO_STATUS_READONLY
const IO_STATUS_READONLY IOStatus = iota /**< Tried to write a read-only buffer */
//so:extern SDL_IO_STATUS_WRITEONLY
const IO_STATUS_WRITEONLY IOStatus = iota /**< Tried to read a write-only buffer */
