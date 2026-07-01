package game

import (
	"mbc/gfx"
	"mbc/net/mc"
	"solod.dev/so/fmt"
)

func (s *State) Screen_InGame(state *ScreenInGameState, screen gfx.Rectangle) {
	if !state.Initialized {
		*state = ScreenInGameState{}
		state.ErrMsgBuf = fmt.BufferFrom(state.__ErrorMessageBufMemory[:])
		// reset arenas and stuff
	}
	if state.Disconnected {
		state.OnDisconnect(s)
		return
	}

	packetID, err := s.ClientBound.ReadByte()
	if !state.handleError(err) {
		return
	}

	if packetID != 0 {
		if !state.OnPacket(packetID) {
			return
		}
	}
}

// sets error message and always returns false
func (state *ScreenInGameState) setErrorMessage(msg string) bool {
	state.Disconnected = true
	state.ErrorMessage = msg
	return false
}
func (state *ScreenInGameState) handleError(err error) bool {
	if err != nil {
		state.setErrorMessage(err.Error())
		return false
	}
	return true
}
func (state *ScreenInGameState) OnDisconnect(s *State) {
	s.ScreenConnectServerState.stage = -1
	// cannot reset here because error message needs to persist
	s.ScreenConnectServerState.Text = state.ErrorMessage
	s.CurrentScreeen = SCREEN_CONNECT_SERVER
	state.Initialized = false
}
func (state *ScreenInGameState) OnPacket(id mc.PacketID) bool {
	switch id {
	}
	return state.setErrorMessage(
		fmt.Sprintf(state.ErrMsgBuf, "Cannot handle %s", mc.PacketIDString(id)),
	)
}
