package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"

	"solod.dev/so/mem"
)

func (s *State) Init() {
	s.Pack = NewDefaultTexturePack()
	gfx.SetTextureConfig(s.Pack.GetTexture(assets.Pack), true, false)
	s.Scratch = mem.NewArena(___scratchBuf[:])
}

// return false to quit.
func (s *State) Update() bool {
	gui.Update(s.ScreenWidth, s.ScreenHeight, s.Pack)

	screen := gfx.Rectangle{W: float32(s.ScreenWidth), H: float32(s.ScreenHeight)}
	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Black)
	s.DrawMainMenu(screen)
	gfx.EndDrawing()
	return true
}
