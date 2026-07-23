package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net/mc"
	"mbc/sdl"

	"solod.dev/so/errors"
	"solod.dev/so/fmt"
)

func (s *State) Screen_InGame(state *ScreenInGameState, screen gfx.Rectangle) {
	if !state.Initialized {
		*state = ScreenInGameState{}
		state.Init(s)
		state.Initialized = true
	}

	s.MouseLock = true // mouse lock is explicitly disabled if needed.
	s.ProcessDpadUIInput(2, &state.selected)
	switch state.CurrentScreen {
	case SCREEN_INGAME_DISCONNECTED_SCREEN:
		state.ScreenDisconnected(s, screen)
	case SCREEN_INGAME_PAUSED_SCREEN:
		state.ScreenPaused(s, screen)
	case SCREEN_INGAME:
		state.ScreenInGame(s)
	}
}
func (state *ScreenInGameState) SendQuittingPacket(s *State) {
	dc := mc.PacketDisconnect{
		Reason: []rune("Quitting"),
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
			state.SendQuittingPacket(s)
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
func (state *ScreenInGameState) ScreenDisconnected(s *State, screen gfx.Rectangle) {
	s.InteractingWithUI = true
	state.selected = 0
	s.MouseLock = false
	clicked := s.Inputs[InputTap].Up
	if s.UIDpadMode {
		clicked = s.Inputs[InputReturn].Up
	}
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	fnt := gui.ActivePack.Font()
	var runes []rune
	if state.Error == nil {
		runes = []rune("Disconnected")
	} else if state.Error == NoDecoderForPacketErr {
		runes = []rune(
			fmt.Sprintf(fmt.NewBuffer(100), "Cannot decode %s", mc.PacketIDString(state.PacketID)),
		)
	} else if state.Error != nil {
		runes = []rune(state.Error.Error())
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

	hovered := bbox.Contains(s.Cursor)
	if s.UIDpadMode {
		hovered = state.selected == 0
	}
	if clicked && hovered {
		s.PlaySoundEffect(assets.Newsound_random_click)
		s.CurrentScreeen = SCREEN_CONNECT_SERVER
		state.Initialized = false
		s.ScreenConnectServerState = ScreenConnectServerState{}
		s.ScreenConnectServerState.ShouldTransision = true
		s.ScreenConnectServerState.TransisionTo = SCREEN_MENU_SELECT_SERVER
		state.Unload()
		return
	}
	gui.Button("Back", bbox, hovered, true)
}

func (state *ScreenInGameState) ScreenPaused(s *State, screen gfx.Rectangle) {
	if s.Inputs[InputClose].Up {
		state.CurrentScreen = SCREEN_INGAME
		return
	}
	s.InteractingWithUI = true
	s.MouseLock = false
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	fnt := gui.ActivePack.Font()
	runes := []rune("Paused")
	size := fnt.TextSize(runes).Scale(gui.Scale)
	bbox := gfx.Rectangle{W: size.X, H: size.Y}.
		Anchor(screen, .5, .35)

	fnt.DrawRunes(runes, bbox.Position(), gui.Scale, 0, gfx.White, false)

	buttons := gui.ButtonSize.Scale(gui.Scale).Anchor(screen, .5, .5)

	resumeButton := gfx.Rectangle{W: buttons.W, H: buttons.H, X: buttons.X, Y: buttons.Y}
	clicked := s.Inputs[InputTap].Up
	if s.UIDpadMode {
		clicked = s.Inputs[InputReturn].Up
	}
	buttons.Y += buttons.H + 1
	{ // resume button
		hovered := resumeButton.Contains(s.Cursor)
		if s.UIDpadMode {
			hovered = state.selected == 0
		}
		gui.Button("Resume", resumeButton, hovered, true)
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.CurrentScreen = SCREEN_INGAME
		}
	}
	disconnectButton := gfx.Rectangle{W: buttons.W, H: buttons.H, X: buttons.X, Y: buttons.Y}
	buttons.Y += buttons.H + 1
	{ // disconect button
		hovered := disconnectButton.Contains(s.Cursor)
		if s.UIDpadMode {
			hovered = state.selected == 1
		}
		gui.Button("Disconnect", disconnectButton, hovered, true)
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.SendQuittingPacket(s)
			state.CurrentScreen = SCREEN_INGAME_DISCONNECTED_SCREEN
		}
	}
}
