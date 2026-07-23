package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net"
	"mbc/net/mc"

	"solod.dev/so/bufio"
	"solod.dev/so/errors"
	"solod.dev/so/mem"
)

func (s *State) Screen_ConnectServer(state *ScreenConnectServerState, screen gfx.Rectangle) {
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	const NInteractables = 1
	s.ProcessDpadUIInput(NInteractables, &state.selected)
	if state.ShouldTransision {
		s.CurrentScreeen = state.TransisionTo
		s.Conn.Close()
		*state = ScreenConnectServerState{}
		return
	}

	// Drawing code

	// draw status text
	fnt := gui.ActivePack.Font()
	var runes []rune
	if state.Err == nil {
		runes = []rune("Connecting")
	} else {
		runes = []rune(state.Err.Error())
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
	clicked := s.Inputs[InputTap].Up
	hovered := bbox.Contains(s.Cursor)
	if s.UIDpadMode {
		clicked = s.Inputs[InputReturn].Up
		hovered = state.selected == 0
	}
	if clicked && hovered {
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_MENU_SELECT_SERVER
		s.PlaySoundEffect(assets.Newsound_random_click)
		return
	}
	gui.Button("Back", bbox, hovered, true)

	// Update logic code

	// get selected server from config file
	var srv *cfg.ServerCfg = &s.Config.Servers[min(s.SelectedServer, cfg.MAX_SERVERS-1)]
	// start dialing
	if !state.Dialed {
		state.Arena = mem.NewArena(state.__ArenaBuf[:])
		state.Dialed = true
		// blocks
		s.Conn, state.Err = net.Dial(srv.Host)
		if state.Err != nil {
			print("Failed to dial")
			state.stage = -1
			return
		} else {
			s.__arenaForServerbound = mem.NewArena(s.__bufioWriterBuffer[:])
			s.ServerBound = bufio.NewWriter(&s.__arenaForServerbound, &s.Conn)
			s.__arenaForClientbound = mem.NewArena(s.__bufioReaderBuffer[:])
			s.ClientBound = net.NewBufferedReader(&s.__arenaForClientbound, &s.Conn)
		}
	}
	// go back if back is pressed
	if s.Inputs[InputClose].Down {
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_JOIN_SERVER
		return
	}
	if state.Err == nil {
		if state.Err = state.Connect(s); state.Err != nil {
			state.stage = -1 // COMPLETED
		} else if state.stage == -1 {
			// do not go through ShouldTransition because it closes the connection.
			s.CurrentScreeen = SCREEN_INGAME
			return
		}
	}
}

var ErrOnlyOfflineModeSupported = errors.New("Only offline mode servers are supported for now.")
var ErrDisconnectedByServer = errors.New("Disconnected by server.")

func (state *ScreenConnectServerState) Connect(s *State) error {
	const (
		SEND_PRE_LOGIN = iota
		RECV_PRE_LOGIN
		SEND_LOGIN
		RECV_LOGIN
		COMPLETED = -1
	)
	switch state.stage {
	case COMPLETED:
		return state.Err
	case SEND_PRE_LOGIN:
		p := mc.ServerboundPreLogin{
			Username: []rune("magicbeta"),
		}
		if err := p.Write(&s.ServerBound); err != nil {
			return err
		} else {
			s.ServerBound.Flush()
			state.stage = RECV_PRE_LOGIN
			state.Arena.Reset()
			state.Decoder = mc.NewDecoder(&state.Arena, mc.PKT_PreLogin)
		}
	case RECV_PRE_LOGIN:
		if state.packetID == mc.PKT_Disconnect {
			return ErrDisconnectedByServer
		}
		if state.packetID == mc.PKT_PreLogin {
			if ok, err := state.Decoder.Step(&state.Arena, &s.ClientBound); !ok {
				return err
			}
			pkt := state.Decoder.(*mc.ClientboundPreLogin)
			if pkt.ConnectionHash[0] != '-' {
				return ErrOnlyOfflineModeSupported
			}
			state.stage = SEND_LOGIN
		} else { // read packet id
			if !s.ClientBound.ReadUint8(&state.packetID) {
				return s.ClientBound.Err()
			}
		}
	case SEND_LOGIN:
		pkt := mc.ServerboundLogin{
			ProtocolVersion: 14,
			Username:        mc.String16("magicbeta"),
		}
		if err := pkt.Write(&s.ServerBound); err != nil {
			return err
		} else {
			s.ServerBound.Flush()
			state.stage = RECV_LOGIN
			state.Arena.Reset()
			state.Decoder = mc.NewDecoder(&state.Arena, mc.PKT_Login)
		}
	case RECV_LOGIN:
		if state.packetID == mc.PKT_Disconnect {
			return ErrDisconnectedByServer
		}
		if state.packetID == mc.PKT_Login {
			if ok, err := state.Decoder.Step(&state.Arena, &s.ClientBound); !ok {
				return err
			}
			pkt := state.Decoder.(*mc.ClientboundLogin)
			_ = pkt // TODO: spawn the player entity
			state.stage = COMPLETED
		} else { // read packet id
			if !s.ClientBound.ReadUint8(&state.packetID) {
				return s.ClientBound.Err()
			}
		}
	}
	return state.Err
}
