package game

import (
	"mbc/gfx"
	"mbc/gui"
)

func (s *State) DrawMainMenu(screen gfx.Rectangle) {
	// draw background
	bg := s.Pack.GetTexture("/gui/background.png")
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale()*2,
		gfx.White.Tint(gfx.Black, 75),
	)

	menuScreen := gfx.Rectangle{H: 320 * .75, W: 200}.
		Scale(gui.Scale()).
		Anchor(screen, .5, .1)

	// Main Menu Layout(menuScreen)
	logo := gfx.Rectangle{}.
		SetSize(gui.MinecraftLogoSize).
		Scale(gui.Scale()).
		Anchor(menuScreen, .50, .1)
	gui.MinecraftLogo(logo)

	buttonSet := gfx.Rectangle{W: gui.ButtonSize.X, H: (gui.ButtonSize.Y + 2) * 4}.
		Scale(gui.Scale()).
		Anchor(menuScreen, .5, .70)
	btn := gfx.Rectangle{}.
		SetSize(gui.ButtonSize).
		SetPosition(buttonSet.Position())
	for i := range 4 {
		// btn.X *= gui.Scale()
		btn.Y += btn.H * gui.Scale()
		if i != 0 {
			btn.Y += 2 //padding
		}
		gui.Button("Play",
			btn.Scale(gui.Scale()),
			i == 0, true,
		)
	}
}
