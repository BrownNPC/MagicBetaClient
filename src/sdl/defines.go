package sdl

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
