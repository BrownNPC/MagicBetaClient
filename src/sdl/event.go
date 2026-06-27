package sdl

import (
	"solod.dev/so/c"
	"solod.dev/so/time"
)

//so:extern SDL_Event
type Event struct{}

func (e *Event) Type() EventType {
	return *any(e).(*EventType)

}
func (e *Event) Quit() QuitEvent {
	return *any(e).(*QuitEvent)
}
func (e *Event) Window() WindowEvent {
	return *any(e).(*WindowEvent)
}
func (e *Event) MouseButton() MouseButtonEvent {
	return *any(e).(*MouseButtonEvent)
}
func (e *Event) MouseWheel() MouseWheelEvent {
	return *any(e).(*MouseWheelEvent)
}
func (e *Event) MouseMotion() MouseMotionEvent {
	return *any(e).(*MouseMotionEvent)
}
func (e *Event) Keyboard() KeyboardEvent {
	return *any(e).(*KeyboardEvent)
}
func (e *Event) TextInput() TextInputEvent {
	return *any(e).(*TextInputEvent)
}
func (e *Event) TextEditing() TextEditingEvent {
	return *any(e).(*TextEditingEvent)
}

type QuitEvent struct {
	/**< SDL_EVENT_QUIT */
	Type     EventType
	reserved uint32
	/**< In nanoseconds, populated using SDL_GetTicksNS() */
	Timestamp time.Duration
}
type WindowEvent struct {
	Type     EventType
	reserved uint32
	/**< In nanoseconds, populated using SDL_GetTicksNS() */
	Timestamp    time.Duration
	WindowID     uint32
	Data1, Data2 int32
}

type MouseButtonEvent struct {
	Type      EventType /**< SDL_EVENT_MOUSE_BUTTON_DOWN or SDL_EVENT_MOUSE_BUTTON_UP */
	reserved  int32
	Timestamp time.Duration /**< In nanoseconds, populated using SDL_GetTicksNS() */
	WindowID  uint32        /**< The window with mouse focus, if any */
	Which     uint32        /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, or 0 */
	Button    uint8         /**< The mouse button index */
	Down      bool          /**< true if the button is pressed */
	Clicks    uint8         /**< 1 for single-click, 2 for double-click, etc. */
	Padding   uint8
	X         float32 /**< X coordinate, relative to window */
	Y         float32 /**< Y coordinate, relative to window */
}
type MouseWheelEvent struct {
	Type      EventType /**< SDL_EVENT_MOUSE_WHEEL */
	reserved  uint32
	Timestamp time.Duration /**< In nanoseconds, populated using SDL_GetTicksNS() */
	WindowID  uint32        /**< The window with mouse focus, if any */
	Which     uint32        /**< The mouse instance id in relative mode or 0 */
	X         float32       /**< The amount scrolled horizontally, positive to the right and negative to the left */
	Y         float32       /**< The amount scrolled vertically, positive away from the user and negative toward the user */
	Direction uint32        /**< Set to one of the SDL_MOUSEWHEEL_* defines. When FLIPPED the values in X and Y will be opposite. Multiply by -1 to change them back */
	MouseX    float32       /**< X coordinate, relative to window */
	MouseY    float32       /**< Y coordinate, relative to window */
	IntegerX  int32         /**< The amount scrolled horizontally, accumulated to whole scroll "ticks" (added in 3.2.12) */
	IntegerY  int32         /**< The amount scrolled vertically, accumulated to whole scroll "ticks" (added in 3.2.12) */
}
type MouseMotionEvent struct {
	Type      EventType /**< SDL_EVENT_MOUSE_MOTION */
	reserved  uint32
	Timestamp time.Duration /**< In nanoseconds, populated using SDL_GetTicksNS() */
	WindowID  uint32        /**< The window with mouse focus, if any */
	Which     uint32        /**< The mouse instance id in relative mode, SDL_TOUCH_MOUSEID for touch events, SDL_PEN_MOUSEID for pen events, or 0 */
	State     uint32        /**< The current button state */
	X         float32       /**< X coordinate, relative to window */
	Y         float32       /**< Y coordinate, relative to window */
	Xrel      float32       /**< The relative motion in the X direction */
	Yrel      float32       /**< The relative motion in the Y direction */
}
type KeyboardEvent struct {
	Type      EventType /**< SDL_EVENT_KEY_DOWN or SDL_EVENT_KEY_UP */
	reserved  uint32
	Timestamp time.Duration /**< In nanoseconds, populated using SDL_GetTicksNS() */
	WindowID  uint32        /**< The window with keyboard focus, if any */
	Which     uint32        /**< The keyboard instance id, or 0 if unknown or virtual */
	Scancode  uint32        /**< SDL physical key code */
	Key       uint32        /**< SDL virtual key code */
	Mod       uint32        /**< current key modifiers */
	Raw       uint16        /**< The platform dependent scancode for this event */
	Down      bool          /**< true if the key is pressed */
	Repeat    bool          /**< true if this is a key repeat */
}
type TextInputEvent struct {
	Type      EventType
	reserved  uint32
	Timestamp time.Duration
	WindowID  uint32
	text      *c.ConstChar
}

func (e TextInputEvent) Rune() rune {
	return []rune(c.String(e.text))[0]
}

type TextEditingEvent struct {
	Type      EventType
	reserved  uint32
	Timestamp time.Duration
	WindowID  uint32
	text      *c.ConstChar
	Start     int32
	End       int32
}

func (e TextEditingEvent) Text() string {
	return c.String(e.text)
}
