package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"

	"solod.dev/so/strconv"
	"solod.dev/so/strings"
)

func (s *State) Screen_JoinServer(state *ScreenJoinServerState, screen gfx.Rectangle) {
	// draw background
	bg := s.Pack.GetTexture(assets.Gui_background)
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	// go to main menu if close input
	if s.Inputs[InputClose].Released {
		s.CurrentScreeen = SCREEN_MENU_MAIN
		return
	}

	list := gfx.Rectangle{
		W: gui.ButtonSize.W,
		H: 160,
	}.Scale(gui.Scale).
		Anchor(screen, .5, .45)

	btn := gui.ButtonSize.Scale(gui.Scale).
		Anchor(list, .5, 0)
	const MaxPerScreen = 5

	pageCount := len(s.Config.Servers) / MaxPerScreen // rounds down
	// always round up the number of pages if needed.
	if len(s.Config.Servers)%MaxPerScreen != 0 {
		pageCount++
	}

	maxPage := max(0, pageCount-1)

	for i := range MaxPerScreen {
		idx := state.PageIndex*MaxPerScreen + i
		if i != 0 {
			btn.Y += btn.H
			btn.Y += 2 * gui.Scale //padding
		}

		if idx >= len(s.Config.Servers) {
			gui.Button("", btn, false, false)
			continue
		}
		srv := s.Config.Servers[idx]
		gui.Button(srv.Host, btn, btn.Contains(s.Cursor), true)
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
		contains := prevBtn.Contains(s.Cursor)
		enabled := state.PageIndex != 0
		clicked := s.Inputs[InputLeftClick].Released
		if enabled && contains && clicked {
			state.PageIndex = max(state.PageIndex-1, 0)
			s.PlaySoundEffect(assets.Newsound_random_click)
		}
		gui.Button("Prev", prevBtn, contains, enabled)
	}
	{ // next button click
		contains := nextBtn.Contains(s.Cursor)
		enabled := state.PageIndex != maxPage
		clicked := s.Inputs[InputLeftClick].Released
		if enabled && contains && clicked {
			state.PageIndex = min(state.PageIndex+1, maxPage)
			s.PlaySoundEffect(assets.Newsound_random_click)
		}
		gui.Button("Next", nextBtn, contains, enabled)
	}
	backButton := gui.ButtonSize.Scale(gui.Scale)
	backButton.X = btn.X
	backButton.Y = nextBtn.Y + nextBtn.H

	backButton.Y += nextBtn.H
	contains := backButton.Contains(s.Cursor)
	clicked := s.Inputs[InputLeftClick].Released
	if contains && clicked {
		s.CurrentScreeen = SCREEN_MENU_MAIN
		s.PlaySoundEffect(assets.Newsound_random_click)
		return
	}
	gui.Button("Back", backButton, contains, true)
}
