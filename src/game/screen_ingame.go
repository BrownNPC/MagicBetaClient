package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net/mc"

	"solod.dev/so/fmt"
)

func (state *ScreenInGameState) Init(s *State) {
	state.ErrMsgBuf = fmt.BufferFrom(state.__ErrorMessageBufMemory[:])
	state.Cam = gfx.Camera{
		Position: gfx.Vector3{Y: 2},
		Target:   gfx.Vector3{Z: 1},
		Up:       gfx.Vector3{Y: 1},
		Fovy:     70,
	}
}
func (s *State) Screen_InGame(state *ScreenInGameState, screen gfx.Rectangle) {
	if !state.Initialized {
		*state = ScreenInGameState{}
		state.Init(s)
		state.Initialized = true
	}
	if state.Disconnected {
		state.OnDisconnect(s, screen)
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
func (state *ScreenInGameState) OnDisconnect(s *State, screen gfx.Rectangle) {
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	fnt := gui.ActivePack.Font()
	runes := []rune(state.ErrorMessage)
	size := fnt.TextSize(runes).Scale(gui.Scale)
	bbox := gfx.Rectangle{W: size.X, H: size.Y}.
		Anchor(screen, .5, .5)
	fnt.DrawRunes(runes, bbox.Position(), gui.Scale, 0, gfx.White, false)
	// draw back button
	bbox.W = gui.ButtonSize.W * gui.Scale
	bbox.H = gui.ButtonSize.H * gui.Scale
	bbox = bbox.Anchor(screen, .5, .5)
	bbox.Y += bbox.H
	bbox.Y += 4 * gui.Scale
	clicked := s.Inputs[InputTap].Released
	hovered := bbox.Contains(s.Cursor)
	if clicked && hovered {
		s.PlaySoundEffect(assets.Newsound_random_click)
		s.CurrentScreeen = SCREEN_MENU_SELECT_SERVER
		return
	}
	gui.Button("Back", bbox, hovered, true)

}
func (state *ScreenInGameState) OnPacket(id mc.PacketID) bool {
	switch id {
	}
	return state.setErrorMessage(
		fmt.Sprintf(state.ErrMsgBuf, "Cannot handle %s", mc.PacketIDString(id)),
	)
}
