package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
)

func (s *State) Screen_JoinServer(state *ScreenJoinServerState, screen gfx.Rectangle) {
	// get selected server from config file
	var srv *cfg.ServerCfg = &s.Config.Servers[min(s.SelectedServer, cfg.MAX_SERVERS-1)]
	// init
	if state.HaveInitialized == false {
		state.TextFields[1].Init(srv.Host)
		state.TextFields[2].Init(srv.Cmd)
		state.HaveInitialized = true
	}
	// go back if close input
	if s.Inputs[InputClose].Pressed {
		// reset state on switch
		*state = ScreenJoinServerState{}
		s.CurrentScreeen = SCREEN_MENU_SELECT_SERVER
		return
	}

	// draw background
	bg := s.Pack.GetTexture(assets.Gui_background)

	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)

	// content bbox for this screen.
	content := gfx.Rectangle{
		W: gui.ButtonSize.W,
		H: 160,
	}.Scale(gui.Scale).
		Anchor(screen, .5, .45)

	// hostname text field
	hostname := gui.ButtonSize.Scale(gui.Scale).
		Anchor(content, .5, 2/content.H) // y offset 2px in gui units.

	// there was a click this frame
	clicked := s.Inputs[InputLeftClick].Released
	if clicked && hostname.Contains(s.Cursor) {
		state.TextFieldFocused = 1
		s.TextInputActive = true
	} else if clicked {
		s.TextInputActive = false
		state.TextFieldFocused = 0
	}
	input := s.Inputs[InputTextInput].Text
	tf := &state.TextFields[state.TextFieldFocused]
	if input != 0 && tf.Len < 128 {
		tf.Add(input)
	}
	if s.Inputs[InputBackspace].Pressed {
		tf.Pop()
	}

	// hostname text field
	gui.TextField(state.TextFields[1].String(), "example.com:25565", hostname, state.TextFieldFocused == 1)
}
