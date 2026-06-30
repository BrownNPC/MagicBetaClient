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
	s.InteractingWithUI = true
	// get selected server from config file
	var srv *cfg.ServerCfg = &s.Config.Servers[min(s.SelectedServer, cfg.MAX_SERVERS-1)]
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

	// Dpad Navigation (0: Hostname, 1: Cmd, 2: Connect, 3: Back)
	const NInteractables = 4
	if s.UIDpadMode && (s.Inputs[InputDown].Pressed || s.Inputs[InputRight].Pressed) {
		state.selected = min(state.selected+1, NInteractables-1)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}
	if s.UIDpadMode && (s.Inputs[InputUp].Pressed || s.Inputs[InputLeft].Pressed) {
		state.selected = max(state.selected-1, 0)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}

	// content bbox for this screen.
	content := gfx.Rectangle{
		W: gui.ButtonSize.W,
		H: 160,
	}.Scale(gui.Scale).
		Anchor(screen, .5, .45)

	vertical := gfx.Vector2{X: content.X, Y: content.Y}
	fnt := gui.ActivePack.Font()

	// --- Layout Setup ---

	// Hostname text field header text
	vertical.Y += 10 * gui.Scale
	fnt.DrawRunes([]rune("Hostname including port:"),
		vertical, gui.Scale, 0, gfx.White, false)
	vertical.Y += float32(fnt.TextHeight())*gui.Scale + 2*gui.Scale
	// hostname text field rect
	hostname := gui.ButtonSize.Scale(gui.Scale).SetPosition(vertical)
	vertical.Y += hostname.H

	// cmd text field header text
	vertical.Y += 10 * gui.Scale
	fnt.DrawRunes([]rune("Command to run on join:"),
		vertical, gui.Scale, 0, gfx.White, false)
	vertical.Y += float32(fnt.TextHeight())*gui.Scale + 2*gui.Scale
	// cmd text field rect
	cmd := gui.ButtonSize.Scale(gui.Scale).SetPosition(vertical)

	// --- Interaction Logic ---

	// Establish a unified click state for this frame
	clicked := s.Inputs[InputTap].Released
	if s.UIDpadMode {
		clicked = s.Inputs[InputReturn].Released
	}

	// Determine hover/visual selection states
	hostnameHovered := hostname.Contains(s.Cursor)
	cmdHovered := cmd.Contains(s.Cursor)
	if s.UIDpadMode {
		hostnameHovered = (state.selected == 0)
		cmdHovered = (state.selected == 1)
	}

	// Text Field Focus Logic
	if clicked {
		if hostnameHovered {
			state.TextFieldFocused = 1
			s.TextInputActive = true
		} else if cmdHovered {
			state.TextFieldFocused = 2
			s.TextInputActive = true
		} else {
			// Clicked elsewhere; unfocus
			state.TextFieldFocused = 0
			s.TextInputActive = false
		}
	} else if !s.TextInputActive {
		// Update visual focus based on hover (if not actively typing)
		if hostnameHovered {
			state.TextFieldFocused = 1
		} else if cmdHovered {
			state.TextFieldFocused = 2
		} else if !s.UIDpadMode {
			state.TextFieldFocused = 0
		} else if s.UIDpadMode && state.selected > 1 {
			state.TextFieldFocused = 0
		}
	}

	// Draw Text Fields using the properly evaluated focus state
	gui.TextField(state.TextFields[1].String(), "example.com:25565", hostname, state.TextFieldFocused == 1, s.TextInputActive && state.TextFieldFocused == 1)
	gui.TextField(state.TextFields[2].String(), "eg. /login password123", cmd, state.TextFieldFocused == 2, s.TextInputActive && state.TextFieldFocused == 2)

	// --- Buttons ---
	actionButtons := gfx.Rectangle{W: gui.ButtonSize.W, H: gui.ButtonSize.H*2 + 2}.Scale(gui.Scale).
		Anchor(content, .5, 1)

	// Connect button
	{
		connectButton := gui.ButtonSize.Scale(gui.Scale).Anchor(actionButtons, .5, 0)
		hovered := connectButton.Contains(s.Cursor)
		if s.UIDpadMode {
			hovered = (state.selected == 2)
		}
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.ShouldTransition = true
			state.switchToScreen = SCREEN_CONNECT_SERVER
		}
		gui.Button("Connect", connectButton, hovered, state.TextFields[1].String() != "")
	}

	// Back button
	{
		backButton := gui.ButtonSize.Scale(gui.Scale).Anchor(actionButtons, .5, 1)
		hovered := backButton.Contains(s.Cursor)
		if s.UIDpadMode {
			hovered = (state.selected == 3)
		}
		gui.Button("Back", backButton, hovered, true)
		if hovered && clicked {
			s.PlaySoundEffect(assets.Newsound_random_click)
			state.ShouldTransition = true
			state.switchToScreen = SCREEN_MENU_SELECT_SERVER
		}
	}

	// --- Text Typing Input Processing ---
	if (state.TextFieldFocused == 1 || state.TextFieldFocused == 2) && s.TextInputActive {
		tf := &state.TextFields[state.TextFieldFocused]
		input := s.Inputs[InputTextInput].Text

		if input != 0 && tf.Len < 70 {
			tf.Add(input)
		}
		if s.Inputs[InputBackspace].Pressed {
			tf.Pop()
		}
	}
}
