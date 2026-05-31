package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
)

func (s *State) Screen_MenuMain(screen gfx.Rectangle) {
	// draw background
	bg := s.Pack.GetTexture(assets.Gui_background)
	// Draw dirt background
	gfx.DrawTextureTiled(bg,
		gfx.NewRectangle(0, 0, float32(s.ScreenWidth), float32(s.ScreenHeight)),
		gui.Scale*2,
		gfx.White.Tint(gfx.Black, 75),
	)

	menuScreen := gfx.Rectangle{H: gui.BaseWidth * .75, W: 200}.
		Scale(gui.Scale).
		Anchor(screen, .5, .1)
	{ // Draw Minecraft logo
		logo := gui.MinecraftLogoSize.
			Scale(gui.Scale).
			Anchor(menuScreen, .50, .1)

		gui.MinecraftLogo(logo)
	}
	const Nbuttons = 3
	ButtonTitles := [Nbuttons]string{
		"Join Server",
		"Texture Packs",
		"Options",
	}

	buttonSet := gfx.Rectangle{W: gui.ButtonSize.W, H: (gui.ButtonSize.H + 2) * 4}.
		Scale(gui.Scale).
		Anchor(menuScreen, .5, .70)
	btn := gui.ButtonSize.
		SetPosition(buttonSet.Position()).
		Scale(gui.Scale)
	for i := range Nbuttons {
		btn.Y += btn.H
		if i != 0 {
			btn.Y += 2 * gui.Scale //padding
		}
		hovered := btn.Contains(s.Cursor)
		if hovered && s.Inputs[InputLeftClick].Released {
			println("Clicked:", ButtonTitles[i])
		}
		gui.Button(ButtonTitles[i],
			btn,
			hovered, true,
		)
	}
}
