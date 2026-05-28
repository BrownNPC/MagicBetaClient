package game

import "mbc/gfx"


func (s *State) Init() {
	s.Pack = NewDefaultTexturePack()
}

// return false to quit.
func (s *State) Update() bool {

	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Red)
	packPNG := s.Pack.GetTexture("/pack.png")
	gfx.DrawTexture(packPNG, 0, 0, 1)
	gfx.EndDrawing()
	_ = DefaultTexturePack{}
	return true
}
