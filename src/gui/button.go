package gui

import "mbc/gfx"

var ButtonSize = gfx.Vector2{
	X: 200, Y: 20,
}

func Button(Text string, bbox gfx.Rectangle, Hovered bool, Enabled bool) {
	GuiTexture := ActivePack.GetTexture("/gui/gui.png")
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
	}.Scale(AtlasScale)
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
		W: float32(font.TextWidth(runes)) * float32(guiScale),
		H: float32(font.TextHeight()) * float32(guiScale),
	}.Anchor(bbox, .5, .5)

	font.DrawRunes(runes, tBB.X, tBB.Y, guiScale, gfx.White, false)
}
