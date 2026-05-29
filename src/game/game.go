package game

import (
	"mbc/gfx"
	"mbc/gui"
)

func (s *State) Init() {
	s.Pack = NewDefaultTexturePack()
	gfx.SetTextureConfig(s.Pack.GetTexture("/pack.png"), true, false)
	gui.Init(320, 180)
	var err error
	s.Font, err = gfx.LoadFont("assets/font/default.png")
	if err != nil {
		panic(err)
	}
}

// return false to quit.
func (s *State) Update() bool {
	gui.Update(s.ScreenWidth, s.ScreenHeight)

	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Red)
	s.DrawMainMenu()
	gfx.EndDrawing()
	return true
}

func (s *State) DrawMainMenu() {
	// draw background
	bg := s.Pack.GetTexture("/gui/background.png")
	if (bg == gfx.Texture{}) {
		panic("bruh")
	}
	GuiTexture := s.Pack.GetTexture("/gui/gui.png")
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale()*2,
		gfx.White.Tint(gfx.Black, 75),
	)
	cX, cY := gui.Anchor(.5, .5, 0, 0)
	gui.Button(
		GuiTexture,
		cX, cY, .90,
		true, true)
	r := []rune{gfx.SectionSign, '2', 'H', 'e', 'l', 'l', 'o'}
	s.Font.DrawRunes(r, 0, 0, 20, gfx.White, false)
}
