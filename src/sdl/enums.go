package sdl

/**
 * The types of events that can be delivered.
 *
 * \since This enum is available since SDL 3.2.0.
 */

//so:include <SDL3/SDL.h>

//so:extern SDL_EventType
type EventType uint32

const (
	EVENT_FIRST = 0 /**< Unused (do not remove) */

	/* Application events */
	EVENT_QUIT = 0x100 /**< User-requested quit */

	/* These application events have special meaning on iOS and Android see README-ios.md and README-android.md for details */
	EVENT_TERMINATING /**< The application is being terminated by the OS. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationWillTerminate()
	  Called on Android in onDestroy()
	*/
	EVENT_LOW_MEMORY /**< The application is low on memory free memory if possible. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationDidReceiveMemoryWarning()
	  Called on Android in onTrimMemory()
	*/
	EVENT_WILL_ENTER_BACKGROUND /**< The application is about to enter the background. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationWillResignActive()
	  Called on Android in onPause()
	*/
	EVENT_DID_ENTER_BACKGROUND /**< The application did enter the background and may not get CPU for some time. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationDidEnterBackground()
	  Called on Android in onPause()
	*/
	EVENT_WILL_ENTER_FOREGROUND /**< The application is about to enter the foreground. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationWillEnterForeground()
	  Called on Android in onResume()
	*/
	EVENT_DID_ENTER_FOREGROUND /**< The application is now interactive. This event must be handled in a callback set with SDL_AddEventWatch().
	  Called on iOS in applicationDidBecomeActive()
	  Called on Android in onResume()
	*/

	EVENT_LOCALE_CHANGED /**< The user's locale preferences have changed. */

	EVENT_SYSTEM_THEME_CHANGED /**< The system theme changed */

	/* Display events */
	/* 0x150 was SDL_DISPLAYEVENT reserve the number for sdl2-compat */
	EVENT_DISPLAY_ORIENTATION           = 0x151 /**< Display orientation has changed to data1 */
	EVENT_DISPLAY_ADDED                         /**< Display has been added to the system */
	EVENT_DISPLAY_REMOVED                       /**< Display has been removed from the system */
	EVENT_DISPLAY_MOVED                         /**< Display has changed position */
	EVENT_DISPLAY_DESKTOP_MODE_CHANGED          /**< Display has changed desktop mode */
	EVENT_DISPLAY_CURRENT_MODE_CHANGED          /**< Display has changed current mode */
	EVENT_DISPLAY_CONTENT_SCALE_CHANGED         /**< Display has changed content scale */
	EVENT_DISPLAY_USABLE_BOUNDS_CHANGED         /**< Display has changed usable bounds */
	EVENT_DISPLAY_FIRST                 = EVENT_DISPLAY_ORIENTATION
	EVENT_DISPLAY_LAST                  = EVENT_DISPLAY_USABLE_BOUNDS_CHANGED

	/* Window events */
	/* 0x200 was SDL_WINDOWEVENT reserve the number for sdl2-compat */
	/* 0x201 was SDL_SYSWMEVENT reserve the number for sdl2-compat */
	EVENT_WINDOW_SHOWN   = 0x202 /**< Window has been shown */
	EVENT_WINDOW_HIDDEN          /**< Window has been hidden */
	EVENT_WINDOW_EXPOSED         /**< Window has been exposed and should be redrawn and can be redrawn directly from event watchers for this event.
	  data1 is 1 for live-resize expose events 0 otherwise. */
	EVENT_WINDOW_MOVED                 /**< Window has been moved to data1 data2 */
	EVENT_WINDOW_RESIZED               /**< Window has been resized to data1xdata2 */
	EVENT_WINDOW_PIXEL_SIZE_CHANGED    /**< The pixel size of the window has changed to data1xdata2 */
	EVENT_WINDOW_METAL_VIEW_RESIZED    /**< The pixel size of a Metal view associated with the window has changed */
	EVENT_WINDOW_MINIMIZED             /**< Window has been minimized */
	EVENT_WINDOW_MAXIMIZED             /**< Window has been maximized */
	EVENT_WINDOW_RESTORED              /**< Window has been restored to normal size and position */
	EVENT_WINDOW_MOUSE_ENTER           /**< Window has gained mouse focus */
	EVENT_WINDOW_MOUSE_LEAVE           /**< Window has lost mouse focus */
	EVENT_WINDOW_FOCUS_GAINED          /**< Window has gained keyboard focus */
	EVENT_WINDOW_FOCUS_LOST            /**< Window has lost keyboard focus */
	EVENT_WINDOW_CLOSE_REQUESTED       /**< The window manager requests that the window be closed */
	EVENT_WINDOW_HIT_TEST              /**< Window had a hit test that wasn't SDL_HITTEST_NORMAL */
	EVENT_WINDOW_ICCPROF_CHANGED       /**< The ICC profile of the window's display has changed */
	EVENT_WINDOW_DISPLAY_CHANGED       /**< Window has been moved to display data1 */
	EVENT_WINDOW_DISPLAY_SCALE_CHANGED /**< Window display scale has been changed */
	EVENT_WINDOW_SAFE_AREA_CHANGED     /**< The window safe area has been changed */
	EVENT_WINDOW_OCCLUDED              /**< The window has been occluded */
	EVENT_WINDOW_ENTER_FULLSCREEN      /**< The window has entered fullscreen mode */
	EVENT_WINDOW_LEAVE_FULLSCREEN      /**< The window has left fullscreen mode */
	EVENT_WINDOW_DESTROYED             /**< The window with the associated ID is being or has been destroyed. If this message is being handled
	  in an event watcher the window handle is still valid and can still be used to retrieve any properties
	  associated with the window. Otherwise the handle has already been destroyed and all resources
	  associated with it are invalid */
	EVENT_WINDOW_HDR_STATE_CHANGED /**< Window HDR properties have changed */
	EVENT_WINDOW_FIRST             = EVENT_WINDOW_SHOWN
	EVENT_WINDOW_LAST              = EVENT_WINDOW_HDR_STATE_CHANGED

	/* Keyboard events */
	EVENT_KEY_DOWN       = 0x300 /**< Key pressed */
	EVENT_KEY_UP                 /**< Key released */
	EVENT_TEXT_EDITING           /**< Keyboard text editing (composition) */
	EVENT_TEXT_INPUT             /**< Keyboard text input */
	EVENT_KEYMAP_CHANGED         /**< Keymap changed due to a system event such as an
	  input language or keyboard layout change. */
	EVENT_KEYBOARD_ADDED          /**< A new keyboard has been inserted into the system */
	EVENT_KEYBOARD_REMOVED        /**< A keyboard has been removed */
	EVENT_TEXT_EDITING_CANDIDATES /**< Keyboard text editing candidates */
	EVENT_SCREEN_KEYBOARD_SHOWN   /**< The on-screen keyboard has been shown */
	EVENT_SCREEN_KEYBOARD_HIDDEN  /**< The on-screen keyboard has been hidden */

	/* Mouse events */
	EVENT_MOUSE_MOTION      = 0x400 /**< Mouse moved */
	EVENT_MOUSE_BUTTON_DOWN         /**< Mouse button pressed */
	EVENT_MOUSE_BUTTON_UP           /**< Mouse button released */
	EVENT_MOUSE_WHEEL               /**< Mouse wheel motion */
	EVENT_MOUSE_ADDED               /**< A new mouse has been inserted into the system */
	EVENT_MOUSE_REMOVED             /**< A mouse has been removed */

	/* Joystick events */
	EVENT_JOYSTICK_AXIS_MOTION     = 0x600 /**< Joystick axis motion */
	EVENT_JOYSTICK_BALL_MOTION             /**< Joystick trackball motion */
	EVENT_JOYSTICK_HAT_MOTION              /**< Joystick hat position change */
	EVENT_JOYSTICK_BUTTON_DOWN             /**< Joystick button pressed */
	EVENT_JOYSTICK_BUTTON_UP               /**< Joystick button released */
	EVENT_JOYSTICK_ADDED                   /**< A new joystick has been inserted into the system */
	EVENT_JOYSTICK_REMOVED                 /**< An opened joystick has been removed */
	EVENT_JOYSTICK_BATTERY_UPDATED         /**< Joystick battery level change */
	EVENT_JOYSTICK_UPDATE_COMPLETE         /**< Joystick update is complete */

	/* Gamepad events */
	EVENT_GAMEPAD_AXIS_MOTION          = 0x650 /**< Gamepad axis motion */
	EVENT_GAMEPAD_BUTTON_DOWN                  /**< Gamepad button pressed */
	EVENT_GAMEPAD_BUTTON_UP                    /**< Gamepad button released */
	EVENT_GAMEPAD_ADDED                        /**< A new gamepad has been inserted into the system */
	EVENT_GAMEPAD_REMOVED                      /**< A gamepad has been removed */
	EVENT_GAMEPAD_REMAPPED                     /**< The gamepad mapping was updated */
	EVENT_GAMEPAD_TOUCHPAD_DOWN                /**< Gamepad touchpad was touched */
	EVENT_GAMEPAD_TOUCHPAD_MOTION              /**< Gamepad touchpad finger was moved */
	EVENT_GAMEPAD_TOUCHPAD_UP                  /**< Gamepad touchpad finger was lifted */
	EVENT_GAMEPAD_SENSOR_UPDATE                /**< Gamepad sensor was updated */
	EVENT_GAMEPAD_UPDATE_COMPLETE              /**< Gamepad update is complete */
	EVENT_GAMEPAD_STEAM_HANDLE_UPDATED         /**< Gamepad Steam handle has changed */

	/* Touch events */
	EVENT_FINGER_DOWN = 0x700
	EVENT_FINGER_UP
	EVENT_FINGER_MOTION
	EVENT_FINGER_CANCELED

	/* Pinch events */
	EVENT_PINCH_BEGIN  = 0x710 /**< Pinch gesture started */
	EVENT_PINCH_UPDATE         /**< Pinch gesture updated */
	EVENT_PINCH_END            /**< Pinch gesture ended */

	/* 0x800 0x801 and 0x802 were the Gesture events from SDL2. Do not reuse these values! sdl2-compat needs them! */

	/* Clipboard events */
	EVENT_CLIPBOARD_UPDATE = 0x900 /**< The clipboard changed */

	/* Drag and drop events */
	EVENT_DROP_FILE     = 0x1000 /**< The system requests a file open */
	EVENT_DROP_TEXT              /**< text/plain drag-and-drop event */
	EVENT_DROP_BEGIN             /**< A new set of drops is beginning (NULL filename) */
	EVENT_DROP_COMPLETE          /**< Current set of drops is now complete (NULL filename) */
	EVENT_DROP_POSITION          /**< Position while moving over the window */

	/* Audio hotplug events */
	EVENT_AUDIO_DEVICE_ADDED          = 0x1100 /**< A new audio device is available */
	EVENT_AUDIO_DEVICE_REMOVED                 /**< An audio device has been removed. */
	EVENT_AUDIO_DEVICE_FORMAT_CHANGED          /**< An audio device's format has been changed by the system. */

	/* Sensor events */
	EVENT_SENSOR_UPDATE = 0x1200 /**< A sensor was updated */

	/* Pressure-sensitive pen events */
	EVENT_PEN_PROXIMITY_IN  = 0x1300 /**< Pressure-sensitive pen has become available */
	EVENT_PEN_PROXIMITY_OUT          /**< Pressure-sensitive pen has become unavailable */
	EVENT_PEN_DOWN                   /**< Pressure-sensitive pen touched drawing surface */
	EVENT_PEN_UP                     /**< Pressure-sensitive pen stopped touching drawing surface */
	EVENT_PEN_BUTTON_DOWN            /**< Pressure-sensitive pen button pressed */
	EVENT_PEN_BUTTON_UP              /**< Pressure-sensitive pen button released */
	EVENT_PEN_MOTION                 /**< Pressure-sensitive pen is moving on the tablet */
	EVENT_PEN_AXIS                   /**< Pressure-sensitive pen angle/pressure/etc changed */

	/* Camera hotplug events */
	EVENT_CAMERA_DEVICE_ADDED    = 0x1400 /**< A new camera device is available */
	EVENT_CAMERA_DEVICE_REMOVED           /**< A camera device has been removed. */
	EVENT_CAMERA_DEVICE_APPROVED          /**< A camera device has been approved for use by the user. */
	EVENT_CAMERA_DEVICE_DENIED            /**< A camera device has been denied for use by the user. */

	/* Render events */
	EVENT_RENDER_TARGETS_RESET = 0x2000 /**< The render targets have been reset and their contents need to be updated */
	EVENT_RENDER_DEVICE_RESET           /**< The device has been reset and all textures need to be recreated */
	EVENT_RENDER_DEVICE_LOST            /**< The device has been lost and can't be recovered. */

	/* Reserved events for private platforms */
	EVENT_PRIVATE0 = 0x4000
	EVENT_PRIVATE1
	EVENT_PRIVATE2
	EVENT_PRIVATE3

	/* Internal events */
	EVENT_POLL_SENTINEL = 0x7F00 /**< Signals the end of an event poll cycle */

	/** Events EVENT_USER through EVENT_LAST are for your use
	 *  and should be allocated with SDL_RegisterEvents()
	 */
	EVENT_USER = 0x8000

	/**
	 *  This last event is only for bounding internal arrays
	 */
	EVENT_LAST = 0xFFFF

	/* This just makes sure the enum is the size of Uint32 */
	EVENT_ENUM_PADDING = 0x7FFFFFFF
)

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
