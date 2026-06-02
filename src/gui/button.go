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
	runes := []rune(Text)
	font := ActivePack.Font()
	tBB := gfx.Rectangle{
		W: float32(font.TextWidth(runes)) * Scale,
		H: float32(font.TextHeight()) * Scale,
	}.Anchor(bbox, .5, .5)

	btnTextColor := gfx.White
	if !Enabled {
		btnTextColor = gfx.Gray
	} else if Hovered {
		btnTextColor = gfx.Yellow
	}
	font.DrawRunes(runes, tBB.Position().AddValue(1), Scale, 0, btnTextColor, true) // shadow
	font.DrawRunes(runes, tBB.Position(), Scale, 0, btnTextColor, false)
}
