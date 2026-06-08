package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
)

func (s *State) Screen_JoinServer(screen gfx.Rectangle, state *ScreenJoinServerState) {
	// draw background
	bg := s.Pack.GetTexture(assets.Gui_background)
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	if s.Inputs[InputClose].Pressed {
		s.CurrentScreeen = SCREEN_MENU_MAIN
		return
	}
}
