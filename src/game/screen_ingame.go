package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net/mc"
	"mbc/sdl"

	"solod.dev/so/errors"
	"solod.dev/so/fmt"
	"solod.dev/so/mem"
)

func (state *ScreenInGameState) Init(s *State) {
	state.Cam = gfx.Camera{
		Position: gfx.Vector3{Y: 2},
		Target:   gfx.Vector3{Z: 1},
		Up:       gfx.Vector3{Y: 1},
		Fovy:     70,
	}
	state.PacketDecodeArena = mem.NewArena(state.__PacketDecodeArenaMemory[:])
	state.PersistentArena = mem.NewArena(state.__PersistentMemory[:])
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
	state.NetworkSystem(s)

	if err := s.ServerBound.Flush(); err != nil {
		state.Disconnected = true
		state.Error = err
		s.Conn.Close()
		return
	}

}
func (state *ScreenInGameState) NetworkSystem(s *State) {
	// drain packets from buffer
	for {
		ok, err := state.TickPacketDecoder(s)
		if err != nil {
			state.Disconnected = true
			state.Error = err
			s.Conn.Close()
			return
		}
		if !ok {
			return
		}
	}
}
func (state *ScreenInGameState) SendDisconnect(s *State) {
	dc := mc.PacketDisconnect{
		Reason: []rune("Client error"),
	}
	dc.Write(&s.ServerBound)
	s.ServerBound.Flush()
	s.Conn.Close()
}

// Tick the packet decoder state machine / coroutine
func (state *ScreenInGameState) TickPacketDecoder(s *State) (bool, error) {
	const (
		WAITING_PACKET = iota
		DECODING_PACKET
		HANDLING_PACKET
	)
	switch state.DecodeState {
	case WAITING_PACKET:
		ok := s.ClientBound.ReadUint8(&state.PacketID)
		if !ok {
			return false, s.ClientBound.Err()
		}
		if !mc.IsValidClientboundPacket(state.PacketID) {
			sdl.Log("invalid packet %s", mc.PacketIDString(state.PacketID))
			panic("The clientbound stream is corrupted.")
		}
		// Got a real packet.
		state.PacketDecodeArena.Reset()
		if state.PacketID == mc.PKT_SetChunkVisibility {
			state.scv = mc.ClientboundSetChunkVisibility{}
			state.Decoder = &state.scv
		} else {
			state.Decoder = mc.NewDecoder(&state.PacketDecodeArena, state.PacketID)
		}
		if state.Decoder == nil {
			state.SendDisconnect(s)
			return false, NoDecoderForPacketErr
		}
		state.DecodeState = DECODING_PACKET
	case DECODING_PACKET:
		if state.Decoder == nil {
			panic("Decoder should not be nil at this stage")
		}
		ok, err := state.Decoder.Step(&state.PacketDecodeArena, &s.ClientBound)
		if !ok {
			return false, err
		}
		state.DecodeState = HANDLING_PACKET
	case HANDLING_PACKET:
		state.dispatchPacketHandler(state.PacketID, state.Decoder)
		state.DecodeState = WAITING_PACKET
	}
	return true, nil
}

var NoDecoderForPacketErr = errors.New("No decoder implemented for packet")

// Show disconnected screen.
func (state *ScreenInGameState) OnDisconnect(s *State, screen gfx.Rectangle) {
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	fnt := gui.ActivePack.Font()
	runes := []rune(state.Error.Error())
	if state.Error == NoDecoderForPacketErr {
		runes = []rune(
			fmt.Sprintf(fmt.NewBuffer(100), "Cannot decode %s", mc.PacketIDString(state.PacketID)),
		)
	}
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
		s.CurrentScreeen = SCREEN_CONNECT_SERVER
		state.Initialized = false
		s.ScreenConnectServerState = ScreenConnectServerState{}
		s.ScreenConnectServerState.ShouldTransision = true
		s.ScreenConnectServerState.TransisionTo = SCREEN_MENU_SELECT_SERVER
		return
	}
	gui.Button("Back", bbox, hovered, true)

}
