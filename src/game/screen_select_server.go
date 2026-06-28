package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"

	"solod.dev/so/strconv"
	"solod.dev/so/strings"
)

func (s *State) Screen_SelectServer(state *ScreenSelectServerState, screen gfx.Rectangle) {
	s.InteractingWithUI=true
	// draw background
	bg := s.Pack.GetTexture(assets.Gui_background)
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	// go to main menu if close input
	if s.Inputs[InputClose].Pressed {
		s.CurrentScreeen = SCREEN_MENU_MAIN
		return
	}
	const NInteractables = 5 + 2 + 1
	if s.UIDpadMode && (s.Inputs[InputDown].Released || s.Inputs[InputRight].Released) {
		state.selected = min(state.selected+1, NInteractables-1)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}
	if s.UIDpadMode && (s.Inputs[InputUp].Released || s.Inputs[InputLeft].Released) {
		state.selected = max(state.selected-1, 0)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}

	list := gfx.Rectangle{
		W: gui.ButtonSize.W,
		H: 160,
	}.Scale(gui.Scale).
		Anchor(screen, .5, .45)

	btn := gui.ButtonSize.Scale(gui.Scale).
		Anchor(list, .5, 0)

	const MaxPerScreen = 5
	pageCount := cfg.MAX_SERVERS / MaxPerScreen // rounds down
	// always round up the number of pages if needed.
	if cfg.MAX_SERVERS%MaxPerScreen != 0 {
		pageCount++
	}

	maxPage := max(0, pageCount-1)

	for i := range MaxPerScreen {
		idx := state.PageIndex*MaxPerScreen + i
		if i != 0 {
			btn.Y += btn.H
			btn.Y += 2 * gui.Scale //padding
		}

		hovered := btn.Contains(s.Cursor)
		clicked := s.Inputs[InputTap].Released
		if s.UIDpadMode {
			hovered = state.selected == i
			clicked = s.Inputs[InputReturn].Released
		}
		if idx >= len(s.Config.Servers) {
			panic("screen_join_server: how is this possible?")
		}
		// Set selected server
		srv := s.Config.Servers[idx]

		if s.Config.Servers[idx].Host == "" {
			gui.Button("[EMPTY]", btn, hovered, true)
		} else {
			gui.Button(srv.Host, btn, hovered, true)
		}
		if clicked && hovered {
			s.PlaySoundEffect(assets.Newsound_random_click)
			s.SelectedServer = uint(idx)
			s.CurrentScreeen = SCREEN_JOIN_SERVER
			return
		}
	}

	// next/prev buttons + pager bounding box
	navGroup := gfx.Rectangle{W: gui.ButtonSize.W, H: gui.ButtonSize.H * 2}.Scale(gui.Scale).
		Anchor(list, .5, .95)

	// half width button
	halfBtn :=
		gfx.Rectangle{W: gui.ButtonSize.W/2 - 1, H: gui.ButtonSize.H}.Scale(gui.Scale)
	// anchored left
	prevBtn := halfBtn.Anchor(navGroup, 0, 1)
	// anchored right
	nextBtn := halfBtn.Anchor(navGroup, 1, 1)

	// Page number
	tmp := make([]byte, strconv.MaxIntBase10Len*10)

	s.Scratch.Reset()
	// Page n/pageCount
	sb := strings.NewBuilder(&s.Scratch)
	sb.WriteString("Page ")
	sb.WriteString(strconv.Itoa(tmp, state.PageIndex+1))
	sb.WriteRune('/')
	sb.WriteString(strconv.Itoa(tmp, pageCount))

	gui.Button(sb.String(),
		gui.ButtonSize.Scale(gui.Scale).Anchor(navGroup, .5, 0),
		false, false,
	)
	{ // previous button click
		hovered := prevBtn.Contains(s.Cursor)
		clicked := s.Inputs[InputTap].Released
		enabled := state.PageIndex != 0
		if s.UIDpadMode {
			hovered = state.selected == 5
			clicked = s.Inputs[InputReturn].Released
		}

		if enabled && hovered && clicked {
			state.PageIndex = max(state.PageIndex-1, 0)
			s.PlaySoundEffect(assets.Newsound_random_click)
		}
		gui.Button("Prev", prevBtn, hovered, enabled)
	}
	{ // next button click
		hovered := nextBtn.Contains(s.Cursor)
		clicked := s.Inputs[InputTap].Released
		if s.UIDpadMode {
			hovered = state.selected == 6
			clicked = s.Inputs[InputReturn].Released
		}
		enabled := state.PageIndex != maxPage
		if enabled && hovered && clicked {
			state.PageIndex = min(state.PageIndex+1, maxPage)
			s.PlaySoundEffect(assets.Newsound_random_click)
		}
		gui.Button("Next", nextBtn, hovered, enabled)
	}
	backButton := gui.ButtonSize.Scale(gui.Scale)
	backButton.X = btn.X
	backButton.Y = nextBtn.Y + nextBtn.H

	backButton.Y += nextBtn.H
	hovered := backButton.Contains(s.Cursor)
	clicked := s.Inputs[InputTap].Released
	if s.UIDpadMode {
		hovered = state.selected == 7
		clicked = s.Inputs[InputReturn].Released
	}
	if hovered && clicked {
		s.CurrentScreeen = SCREEN_MENU_MAIN
		s.PlaySoundEffect(assets.Newsound_random_click)
		return
	}
	gui.Button("Back", backButton, hovered, true)
}
