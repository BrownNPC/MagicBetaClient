package sdl

import (
	"unsafe"

	"solod.dev/so/time"
)

//so:extern SDL_Event
type Event struct{}

func (e *Event) Type() EventType {
	return *(*EventType)(unsafe.Pointer(e))

}
func (e *Event) Quit() QuitEvent {
	return *any(e).(*QuitEvent)
}
func (e *Event) Window() WindowEvent {
	return *any(e).(*WindowEvent)
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
