package game

import (
	"mbc/gfx"
	"mbc/gui"
)

func (s *State) Init() {
	s.Pack = NewDefaultTexturePack()
	gfx.SetTextureConfig(s.Pack.GetTexture("/pack.png"), true, false)
	// gfx.SetTextureConfig(s.Pack.GetTexture("/gui/background.png"), false, false)
	gui.Init(320, 180)
}

// return false to quit.
func (s *State) Update() bool {
	gui.Update(s.ScreenWidth, s.ScreenHeight)

	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Red)

	textureBackground := s.Pack.GetTexture("/gui/background.png")
	gfx.DrawTextureTiled(textureBackground,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale()*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	gui.Button(s.Pack.GetTexture("/gui/gui.png"), 20, 20, 80, true, true)

	gfx.EndDrawing()
	return true
}
