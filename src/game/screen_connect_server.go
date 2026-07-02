package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net"
	"mbc/net/mc"

	"solod.dev/so/bufio"
	"solod.dev/so/mem"
)

func (s *State) Screen_ConnectServer(state *ScreenConnectServerState, screen gfx.Rectangle) {
	if state.ShouldTransision {
		s.CurrentScreeen = state.TransisionTo
		s.Conn.Close()
		*state = ScreenConnectServerState{}
		return
	}
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)

	// Update logic code

	// get selected server from config file
	var srv *cfg.ServerCfg = &s.Config.Servers[min(s.SelectedServer, cfg.MAX_SERVERS-1)]
	// start dialing
	if !state.Dialed {
		state.Arena = mem.NewArena(state.__ArenaBuf[:])
		state.Dialed = true
		conn, err := net.Dial(srv.Host)
		if err != nil {
			state.Text = err.Error()
			state.stage = -1
		} else {
			s.Conn = conn
			s.__arenaForServerbound = mem.NewArena(s.__bufioWriterBuffer[:])
			s.ServerBound = bufio.NewWriter(&s.__arenaForServerbound, &s.Conn)
			s.__arenaForClientbound = mem.NewArena(s.__bufioReaderBuffer[:])
			s.ClientBound = bufio.NewReader(&s.__arenaForClientbound, &s.Conn)
		}
	}
	// go back if back is pressed
	if s.Inputs[InputClose].Pressed {
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_JOIN_SERVER
		return
	}

	switch state.stage {
	case -1:
	case 0:
		// C -> S pre login
		state.Text = "Authenticating"
		s.ServerBound.WriteByte(mc.PKT_PreLogin)
		// prep payload
		state.serverbound_prelogin.Username = []rune("magicbeta")
		// write payload
		err := state.serverbound_prelogin.Write(&s.ServerBound)
		if err != nil {
			state.Text = err.Error()
		} else {
			state.stage++
			s.ServerBound.Flush()
		}
	case 1:
		// S -> C pre login
		// read packet id
		ok, err := state.packetID.Step(&s.ClientBound)
		if err != nil {
			state.Text = err.Error()
		}
		// read payload
		if ok {
			ok, err := state.clientbound_prelogin.Step(&state.Arena, &s.ClientBound)
			if ok {
				if state.clientbound_prelogin.ConnectionHash[0] != '-' {
					state.Text = "Only offline mode servers are supported"
					state.stage = -1
				} else {
					state.stage++
					state.Text = "Logging in"
					state.packetID.Reset()
				}
			}
			if err != nil {
				state.Text = err.Error()
			}
		}
	case 2:
		// C -> S login
		s.ServerBound.WriteByte(mc.PKT_Login)
		// prep payload
		state.serverbound_login.ProtocolVersion = 14
		state.serverbound_login.Username = []rune("magicbeta")
		// write payload
		err := state.serverbound_login.Write(&s.ServerBound)
		if err != nil {
			state.Text = err.Error()
		} else {
			state.stage++
			s.ServerBound.Flush()
		}
	case 3:
		// S -> C login
		// read packet id
		ok, err := state.packetID.Step(&s.ClientBound)
		if err != nil {
			state.Text = err.Error()
		}
		if ok {
			ok, err = state.clientbound_login.Step(&s.ClientBound)
			if ok {
				state.stage++
				state.packetID.Reset()
			}
			if err != nil {
				state.Text = err.Error()
			}
		}
	case 4:
		state.Text = "Connected"
		// dont do the "Sould Transition" thing yet.
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_INGAME
		return
	}

	// Drawing code

	// draw status text
	fnt := gui.ActivePack.Font()
	runes := []rune(state.Text)
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
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_MENU_SELECT_SERVER
		s.PlaySoundEffect(assets.Newsound_random_click)
		return
	}
	gui.Button("Back", bbox, hovered, true)
}
func (state *ScreenConnectServerState) ReadPacketID() {

}
