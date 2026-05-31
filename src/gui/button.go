package gui

import (
	"mbc/gfx"
	"mbc/gfx/assets"
)

var ButtonSize = gfx.Rectangle{
	W: 200, H: 20,
}

func Button(Text string, bbox gfx.Rectangle, Hovered bool, Enabled bool) {
	GuiTexture := ActivePack.GetTexture(assets.Gui_gui)
	as := float32(GuiTexture.Width) / 256 // atlas scale

	state := float32(1)
	if !Enabled {
		state = 0
	} else if Hovered {
		state = 2
	}

	src := gfx.Rectangle{
		X: 0,
		Y: 46 + state*20,
		W: 100,
		H: 20,
	}.Scale(as)
	// draw button in two halves, centered.
	dst := bbox
	dst.W *= .5

	gfx.DrawTextureRec(GuiTexture, src, dst)

	src.X += 100 // capture other half

	dst.X += bbox.W / 2
	gfx.DrawTextureRec(GuiTexture, src, dst)
	font := ActivePack.Font()
	runes := []rune(Text)
	tBB := gfx.Rectangle{
		W: float32(font.TextWidth(runes)) * Scale,
		H: float32(font.TextHeight()) * Scale,
	}.Anchor(bbox, .5, .5)

	font.DrawRunes(runes, tBB.X, tBB.Y, int(Scale), gfx.White, false)
}
