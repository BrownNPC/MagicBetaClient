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

func (state *ScreenInGameState) OnSetSpawnPosition(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetSpawnPosition)
	state.SpawnPosition = gfx.NewVector3(float32(pkt.X), float32(pkt.Y), float32(pkt.Z))
}

func (state *ScreenInGameState) Init(s *State) {
	state.Cam = gfx.Camera{
		Position: gfx.Vector3{Y: 2},
		Target:   gfx.Vector3{Z: 1},
		Up:       gfx.Vector3{Y: 1},
		Fovy:     70,
	}
	state.PacketDecodeArena = mem.NewArena(state.__PacketDecodeArenaMemory[:])
	state.PersistentArena = mem.NewArena(state.__PersistentMemory[:])

	// register packet handlers
	// use this code when https://github.com/solod-dev/solod/issues/83 is fixed
	// state.PacketHandlers[mc.PKT_SetSpawnPosition] = state.OnSetSpawnPosition
}
func (state *ScreenInGameState) dispatchPacketHandler(id mc.PacketID, data mc.Decoder) {
	// use this code when https://github.com/solod-dev/solod/issues/83 is fixed
	// if handler := state.PacketHandlers[state.PacketID]; handler != nil {
	// 	handler(state.Decoder)
	// 	state.DecodeState = DECODE_WAITING
	// } else {
	// 	sdl.Log("No handler registered for %s", mc.PacketIDString(state.PacketID))
	// }
	switch id {
	case mc.PKT_SetSpawnPosition:
		state.OnSetSpawnPosition(data)
	default:
		state.DecodeState = DECODE_HANDLING
		sdl.Log("No handler registered for %s", mc.PacketIDString(state.PacketID))
		return
	}
	state.DecodeState = DECODE_WAITING
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
	if state.Error = state.DecodePackets(s); state.Error != nil {
		state.Disconnected = true
		s.Conn.Close()
	}
}
func (state *ScreenInGameState) DecodePackets(s *State) error {
	switch state.DecodeState {
	case DECODE_WAITING:
		id, err := s.ClientBound.ReadByte()
		if err != nil {
			return err
		}
		state.PacketID = id
		state.PacketDecodeArena.Reset()
		state.Decoder = mc.NewDecoder(&state.PacketDecodeArena, id)
		if state.Decoder == nil {
			return NoDecoderForPacketErr
		}
		state.DecodeState = DECODE_DECODING
	case DECODE_DECODING:
		if state.Decoder == nil {
			panic("Decoder should not be nil at this stage")
		}
		ok, err := state.Decoder.Step(&state.PacketDecodeArena, &s.ClientBound)
		if err != nil {
			return err
		}
		if ok {
			state.DecodeState = DECODE_HANDLING
			return nil
		}
	case DECODE_HANDLING:
		state.dispatchPacketHandler(state.PacketID, state.Decoder)
	}
	return nil
}

var NoDecoderForPacketErr = errors.New("No handler implemented for packet")

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
		s.CurrentScreeen = SCREEN_MENU_SELECT_SERVER
		return
	}
	gui.Button("Back", bbox, hovered, true)

}
