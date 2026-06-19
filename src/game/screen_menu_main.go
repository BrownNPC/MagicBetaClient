package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/sdl"

	"solod.dev/so/bufio"
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
	storage := sdl.OpenTitleStorage("", 0)
	if storage == nil {
		err := sdl.GetError().Error()
		sdl.LogError(1, "Failed to open title storage %s", err)
		return err // 90000 IQ move 
	}

	for !storage.Ready() {
		// hang while not ready
	}

	size, ok := storage.FileSize("assets/title/splashes.txt")
	if !ok {
		err := sdl.GetError().Error()
		sdl.LogError(1, "Failed to open splashes.txt %s", err)
		return err
	}
	file := mem.AllocSlice[byte](&s.Scratch, size, size)
	if !storage.ReadFile("assets/title/splashes.txt", file) {
		err := sdl.GetError().Error()
		sdl.LogError(1, "Failed to read splashes.txt %s", err)
		return err
	}
	defer storage.Close()

	var __Rbuf [1024 * 10]byte
	rArena := mem.NewArena(__Rbuf[:])
	rd := bytes.NewReader(file)
	r := bufio.NewReader(&rArena, &rd)

	const TotalSplashes = 226
	n := rand.IntN(TotalSplashes - 1)

	var final string
	var err error
	for range n {
		final, err = r.ReadString('\n')
		if err != nil {
			panic(err)
		}
	}
	final = strings.TrimSpace(final)
	return strings.Clone(mem.System, final)
}
