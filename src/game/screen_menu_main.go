package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/sdl"

	"solod.dev/so/bufio"
	"solod.dev/so/math/rand"
	"solod.dev/so/mem"
	"solod.dev/so/path"
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

	menuScreen := gfx.Rectangle{H: gui.BaseWidth * .75, W: 200}.
		Scale(gui.Scale).
		Anchor(screen, .5, .1)
	{ // Draw Minecraft logo
		logo := gui.MinecraftLogoSize.
			Scale(gui.Scale).
			Anchor(menuScreen, .50, .1)
		gui.MinecraftLogo(
			s.SplashText,
			logo,
		)
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
	btn := gui.ButtonSize.Scale(gui.Scale)
	btn.X = btn.Anchor(buttonSet, .5, 0).X
	btn.Y = buttonSet.Y
	for i := range Nbuttons {
		btn.Y += btn.H
		if i != 0 {
			btn.Y += 2 * gui.Scale //padding
		}
		hovered := btn.Contains(s.Cursor)
		if hovered && s.Inputs[InputLeftClick].Released {
			s.Screen = SCREEN_MENU_MAIN + i + 1 // Switch screen
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
	f := sdl.IOFromFile(path.Join(&s.Scratch, "assets/", "title/splashes.txt"), "r")
	if f == nil {
		panic(sdl.GetError())
	}

	var __Rbuf [1024 * 10]byte
	rArena := mem.NewArena(__Rbuf[:])
	r := bufio.NewReader(&rArena, f)

	const TotalSplashes = 226
	n := rand.IntN(TotalSplashes - 1)

	finalString := ""
	var err error
	for range n {
		finalString, err = r.ReadString('\n')
		if err != nil {
			panic(err)
		}
	}
	finalString = strings.TrimSpace(finalString)
	return strings.Clone(mem.System, finalString)
}
