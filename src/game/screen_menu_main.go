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

func (s *State) Screen_MenuMain(state *ScreenMainMenuState, screen gfx.Rectangle) {
	s.InteractingWithUI = true
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
	ButtonTitles := [...]string{
		"Join Server",
		"Texture Packs",
		"Options",
	}
	var NInteractables = len(ButtonTitles)
	if s.UIDpadMode && (s.Inputs[InputDown].Pressed || s.Inputs[InputRight].Pressed) {
		state.selected = min(state.selected+1, NInteractables-1)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}
	if s.UIDpadMode && (s.Inputs[InputUp].Pressed || s.Inputs[InputLeft].Pressed) {
		state.selected = max(state.selected-1, 0)
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}

	buttonSet := gfx.Rectangle{W: gui.ButtonSize.W, H: (gui.ButtonSize.H + 2) * 4}.
		Scale(gui.Scale).
		Anchor(menuScreen, .5, .70)
	btn := gui.ButtonSize.Scale(gui.Scale).
		Anchor(buttonSet, .5, 0)
	for i := range len(ButtonTitles) {
		if i != 0 {
			btn.Y += btn.H
			btn.Y += 2 * gui.Scale //padding
		}
		hovered := btn.Contains(s.Cursor)
		clicked := s.Inputs[InputTap].Released
		if s.UIDpadMode {
			hovered = state.selected == i
			clicked = s.Inputs[InputReturn].Released
		}
		// selected :=
		if hovered && clicked {
			s.CurrentScreeen = SCREEN_MENU_MAIN + i + 1 // Switch screen
			s.PlaySoundEffect(assets.Newsound_random_click)
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
