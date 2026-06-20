package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/sdl"

	"solod.dev/so/bytes"
	"solod.dev/so/math/rand"
	"solod.dev/so/mem"
	"solod.dev/so/strings"
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
	// bounding box that contains title screen and buttons
	menuScreen := gfx.Rectangle{H: gui.Base.W * .75, W: 200}.
		Scale(gui.Scale).
		Anchor(screen, .5, .1)

	// Draw Minecraft logo
	logo := gui.MinecraftLogoSize.
		Scale(gui.Scale).
		Anchor(menuScreen, .50, .1)
	gui.MinecraftLogo(
		s.SplashText,
		logo,
	)

	// Draw buttons
	const Nbuttons = 3
	ButtonTitles := [Nbuttons]string{
		"Join Server",
		"Texture Packs",
		"Options",
	}

	buttonSet := gfx.Rectangle{W: gui.ButtonSize.W, H: (gui.ButtonSize.H + 2) * 4}.
		Scale(gui.Scale).
		Anchor(menuScreen, .5, .70)
	btn := gui.ButtonSize.Scale(gui.Scale).
		Anchor(buttonSet, .5, 0)
	for i := range Nbuttons {
		if i != 0 {
			btn.Y += btn.H
			btn.Y += 2 * gui.Scale //padding
		}
		hovered := btn.Contains(s.Cursor)
		if hovered && s.Inputs[InputLeftClick].Released {
			s.CurrentScreeen = SCREEN_MENU_MAIN + i + 1 // Switch screen
			s.PlaySoundEffect(assets.Sound3_random_click)
		}
		gui.Button(ButtonTitles[i],
			btn,
			hovered, true,
		)
	}
}

func (s *State) LoadRandomSplashText() string {
	s.Scratch.Reset()

	// get size of file in bytes
	file, err := s.Storage.ReadFile(&s.Scratch, "assets/title/splashes.txt")
	if err != nil {
		sdl.LogError(1, "Failed to open file %s", err.Error())
		return err.Error()
	}

	r := bytes.NewBuffer(&s.Scratch, file)

	const TotalSplashes = 226
	n := rand.IntN(TotalSplashes - 1)

	var final string
	for range n {
		final, err = r.ReadString('\n')
		if err != nil {
			panic(err)
		}
	}
	final = strings.TrimSpace(final)
	return strings.Clone(mem.System, final)
}
