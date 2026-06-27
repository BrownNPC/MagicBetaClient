package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
)

func (s *State) Screen_JoinServer(state *ScreenJoinServerState, screen gfx.Rectangle) {
	// Draw dirt background
	bg := s.Pack.GetTexture(assets.Gui_background)
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)

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
		state.ShouldTransition = true
		state.switchToScreen = SCREEN_MENU_SELECT_SERVER
	}
	if state.ShouldTransition {
		if srv.Host != state.TextFields[1].String() {
			srv.Host = state.TextFields[1].String()
			*srv = srv.Clone()
		}
		if srv.Cmd != state.TextFields[2].String() {
			srv.Cmd = state.TextFields[2].String()
			*srv = srv.Clone()
		}
		cfg.SaveConfigFile(ORG, APP, CONFIG_FILE_PATH, s.Config)
		s.TextInputActive = false
		// reset state on switch
		s.CurrentScreeen = state.switchToScreen
		*state = ScreenJoinServerState{}
		return
	}

	// content bbox for this screen.
	content := gfx.Rectangle{
		W: gui.ButtonSize.W,
		H: 160,
	}.Scale(gui.Scale).
		Anchor(screen, .5, .45)

	vertical := gfx.Vector2{X: content.X, Y: content.Y}
	fnt := gui.ActivePack.Font()

	// Hostname text field header text
	vertical.Y += 10 * gui.Scale
	fnt.DrawRunes([]rune("Hostname including port:"),
		vertical, gui.Scale, 0, gfx.White, false)
	vertical.Y += float32(fnt.TextHeight())*gui.Scale + 2*gui.Scale
	// hostname text field
	hostname := gui.ButtonSize.Scale(gui.Scale).SetPosition(vertical)
	gui.TextField(state.TextFields[1].String(), "example.com:25565", hostname, state.TextFieldFocused == 1)
	vertical.Y += hostname.H
	// cmd text field header text
	vertical.Y += +10 * gui.Scale
	fnt.DrawRunes([]rune("Command to run on join:"),
		vertical, gui.Scale, 0, gfx.White, false)
	vertical.Y += float32(fnt.TextHeight())*gui.Scale + 2*gui.Scale
	//cmd text field
	cmd := gui.ButtonSize.Scale(gui.Scale).SetPosition(vertical)
	gui.TextField(state.TextFields[2].String(), "eg. /login password123", cmd, state.TextFieldFocused == 2)

	// there was a click this frame
	clicked := s.Inputs[InputLeftClick].Released
	actionButtons := gfx.Rectangle{W: gui.ButtonSize.W, H: gui.ButtonSize.H*2 + 2}.Scale(gui.Scale).
		Anchor(content, .5, 1)
	{
		backButton := gui.ButtonSize.Scale(gui.Scale).
			Anchor(actionButtons, .5, 1)
		hovered := backButton.Contains(s.Cursor)
		gui.Button("Back", backButton, hovered, true)
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.ShouldTransition = true
			state.switchToScreen = SCREEN_MENU_SELECT_SERVER
		}
	}
	// connect button
	{
		clicked := s.Inputs[InputLeftClick].Released
		connectButton := gui.ButtonSize.Scale(gui.Scale).
			Anchor(actionButtons, .5, 0)
		hovered := connectButton.Contains(s.Cursor)
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.ShouldTransition = true
			state.switchToScreen = SCREEN_CONNECT_SERVER
		}
		gui.Button("Connect", connectButton, hovered, state.TextFields[1].String() != "")
	}

	if clicked && hostname.Contains(s.Cursor) {
		state.TextFieldFocused = 1
		s.TextInputActive = true
	} else if clicked && cmd.Contains(s.Cursor) {
		state.TextFieldFocused = 2
		s.TextInputActive = true
	} else if clicked {
		s.TextInputActive = false
		state.TextFieldFocused = 0
	}
	input := s.Inputs[InputTextInput].Text
	tf := &state.TextFields[state.TextFieldFocused]

	if input != 0 && tf.Len < 70 {
		tf.Add(input)
	}
	if s.Inputs[InputBackspace].Pressed {
		tf.Pop()
	}

}
