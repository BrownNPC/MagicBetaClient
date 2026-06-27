package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net"
)

func (s *State) Screen_ConnectServer(state *ScreenConnectServerState, screen gfx.Rectangle) {
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	if state.ShouldTransision {
		s.CurrentScreeen = state.TransisionTo
		s.Conn.Close()
		*state = ScreenConnectServerState{}
	}
	if s.Inputs[InputClose].Pressed {
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_JOIN_SERVER
	}
	// get selected server from config file
	var srv *cfg.ServerCfg = &s.Config.Servers[min(s.SelectedServer, cfg.MAX_SERVERS-1)]
	var err error
	if !state.Dialed {
		state.Dialed = true
		s.Conn, err = net.Dial(srv.Host)
		if err != nil {
			state.Error = err
		}
	}

	fnt := gui.ActivePack.Font()
	var text []rune = []rune("Authenticating")

	if state.Error != nil {
		text = []rune(state.Error.Error())
	}
	size := fnt.TextSize(text).Scale(gui.Scale)

	bbox := gfx.Rectangle{W: size.X, H: size.Y}.
		Anchor(screen, .5, .5)

	fnt.DrawRunes(text, bbox.Position(), gui.Scale, 0, gfx.White, false)

	bbox.W = gui.ButtonSize.W * gui.Scale
	bbox.H = gui.ButtonSize.H * gui.Scale
	bbox = bbox.Anchor(screen, .5, .5)
	bbox.Y += bbox.H
	bbox.Y += 4 * gui.Scale

	clicked := s.Inputs[InputLeftClick].Released
	hovered := bbox.Contains(s.Cursor)
	gui.Button("Back", bbox, hovered, true)
	if clicked && hovered {
		state.ShouldTransision = true
		state.TransisionTo = SCREEN_MENU_SELECT_SERVER
		s.PlaySoundEffect(assets.Newsound_random_click)
	}
}
