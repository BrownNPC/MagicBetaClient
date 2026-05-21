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
	return *(*QuitEvent)(unsafe.Pointer(e))
}

type QuitEvent struct {
	/**< SDL_EVENT_QUIT */
	Type     EventType
	reserved uint32
	/**< In nanoseconds, populated using SDL_GetTicksNS() */
	Timestamp time.Duration
}
