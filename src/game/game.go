package game

import "mbc/gfx"

type State struct {
	Dt float32
}

// return false to quit.
func (s *State) Update() bool {

	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Red)
	gfx.EndDrawing()

	return true
}
