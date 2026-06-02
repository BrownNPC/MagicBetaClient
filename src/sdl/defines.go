package sdl

import "solod.dev/so/c"

type InitFlags uint32

const (
	INIT_AUDIO    InitFlags = 0x00000010 /**< `SDL_INIT_AUDIO` implies `SDL_INIT_EVENTS` */
	INIT_VIDEO    InitFlags = 0x00000020 /**< `SDL_INIT_VIDEO` implies `SDL_INIT_EVENTS`, should be initialized on the main thread */
	INIT_JOYSTICK InitFlags = 0x00000200 /**< `SDL_INIT_JOYSTICK` implies `SDL_INIT_EVENTS` */
	INIT_HAPTIC   InitFlags = 0x00001000
	INIT_GAMEPAD  InitFlags = 0x00002000 /**< `SDL_INIT_GAMEPAD` implies `SDL_INIT_JOYSTICK` */
	INIT_EVENTS   InitFlags = 0x00004000
	INIT_SENSOR   InitFlags = 0x00008000 /**< `SDL_INIT_SENSOR` implies `SDL_INIT_EVENTS` */
	INIT_CAMERA   InitFlags = 0x00010000 /**< `SDL_INIT_CAMERA` implies `SDL_INIT_EVENTS` */
)

type WindowFlags uint64

const (
	WINDOW_FULLSCREEN          WindowFlags = (0x0000000000000001) /**< window is in fullscreen mode */
	WINDOW_OPENGL              WindowFlags = (0x0000000000000002) /**< window usable with OpenGL context */
	WINDOW_OCCLUDED            WindowFlags = (0x0000000000000004) /**< window is occluded */
	WINDOW_HIDDEN              WindowFlags = (0x0000000000000008) /**< window is neither mapped onto the desktop nor shown in the taskbar/dock/window list; SDL_ShowWindow() is required for it to become visible */
	WINDOW_BORDERLESS          WindowFlags = (0x0000000000000010) /**< no window decoration */
	WINDOW_RESIZABLE           WindowFlags = (0x0000000000000020) /**< window can be resized */
	WINDOW_MINIMIZED           WindowFlags = (0x0000000000000040) /**< window is minimized */
	WINDOW_MAXIMIZED           WindowFlags = (0x0000000000000080) /**< window is maximized */
	WINDOW_MOUSE_GRABBED       WindowFlags = (0x0000000000000100) /**< window has grabbed mouse input */
	WINDOW_INPUT_FOCUS         WindowFlags = (0x0000000000000200) /**< window has input focus */
	WINDOW_MOUSE_FOCUS         WindowFlags = (0x0000000000000400) /**< window has mouse focus */
	WINDOW_EXTERNAL            WindowFlags = (0x0000000000000800) /**< window not created by SDL */
	WINDOW_MODAL               WindowFlags = (0x0000000000001000) /**< window is modal */
	WINDOW_HIGH_PIXEL_DENSITY  WindowFlags = (0x0000000000002000) /**< window uses high pixel density back buffer if possible */
	WINDOW_MOUSE_CAPTURE       WindowFlags = (0x0000000000004000) /**< window has mouse captured (unrelated to MOUSE_GRABBED) */
	WINDOW_MOUSE_RELATIVE_MODE WindowFlags = (0x0000000000008000) /**< window has relative mode enabled */
	WINDOW_ALWAYS_ON_TOP       WindowFlags = (0x0000000000010000) /**< window should always be above others */
	WINDOW_UTILITY             WindowFlags = (0x0000000000020000) /**< window should be treated as a utility window, not showing in the task bar and window list */
	WINDOW_TOOLTIP             WindowFlags = (0x0000000000040000) /**< window should be treated as a tooltip and does not get mouse or keyboard focus, requires a parent window */
	WINDOW_POPUP_MENU          WindowFlags = (0x0000000000080000) /**< window should be treated as a popup menu, requires a parent window */
	WINDOW_KEYBOARD_GRABBED    WindowFlags = (0x0000000000100000) /**< window has grabbed keyboard input */
	WINDOW_FILL_DOCUMENT       WindowFlags = (0x0000000000200000) /**< window is in fill-document mode (Emscripten only), since SDL 3.4.0 */
	WINDOW_VULKAN              WindowFlags = (0x0000000010000000) /**< window usable for Vulkan surface */
	WINDOW_METAL               WindowFlags = (0x0000000020000000) /**< window usable for Metal view */
	WINDOW_TRANSPARENT         WindowFlags = (0x0000000040000000) /**< window with transparent buffer */
	WINDOW_NOT_FOCUSABLE       WindowFlags = (0x0000000080000000) /**< window should not be focusable */

)
const (
	BUTTON_LEFT   = 1
	BUTTON_MIDDLE = 2
	BUTTON_RIGHT  = 3
	BUTTON_X1     = 4
	BUTTON_X2     = 5
)

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

//so:extern SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
const AUDIO_DEVICE_DEFAULT_PLAYBACK uint32 = 0xFFFFFFFF

//so:extern SDL_AUDIO_UNKNOWN
const AUDIO_UNKNOWN = 0x0000 /**< Unspecified audio format */
//so:extern SDL_AUDIO_U8
const AUDIO_U8 = 0x0008 /**< Unsigned 8-bit samples */
/* SDL_DEFINE_AUDIO_FORMAT(0, 0, 0, 8), */
//so:extern SDL_AUDIO_S8
const AUDIO_S8 = 0x8008 /**< Signed 8-bit samples */
/* SDL_DEFINE_AUDIO_FORMAT(1, 0, 0, 8), */
//so:extern SDL_AUDIO_S16LE
const AUDIO_S16LE = 0x8010 /**< Signed 16-bit samples */
/* SDL_DEFINE_AUDIO_FORMAT(1, 0, 0, 16), */
//so:extern SDL_AUDIO_S16BE
const AUDIO_S16BE = 0x9010 /**< As above, but big-endian byte order */
/* SDL_DEFINE_AUDIO_FORMAT(1, 1, 0, 16), */
//so:extern SDL_AUDIO_S32LE
const AUDIO_S32LE = 0x8020 /**< 32-bit integer samples */
/* SDL_DEFINE_AUDIO_FORMAT(1, 0, 0, 32), */
//so:extern SDL_AUDIO_S32BE
const AUDIO_S32BE = 0x9020 /**< As above, but big-endian byte order */
/* SDL_DEFINE_AUDIO_FORMAT(1, 1, 0, 32), */
//so:extern SDL_AUDIO_F32LE
const AUDIO_F32LE = 0x8120 /**< 32-bit floating point samples */
/* SDL_DEFINE_AUDIO_FORMAT(1, 0, 1, 32), */
//so:extern SDL_AUDIO_F32BE
const AUDIO_F32BE = 0x9120 /**< As above, but big-endian byte order */
/* SDL_DEFINE_AUDIO_FORMAT(1, 1, 1, 32), */

type AudioCallback func(userdata any, stream *byte, length int)

//so:extern SDL3_AudioSpec
type AudioSpec struct {
	freq              c.Int
	format            uint32
	channels, silence uint8
	samples           uint16
	size              uint32
	callback          AudioCallback
	userdata          any
}
//so:extern SDL_AudioStream
type AudioStream struct{}
