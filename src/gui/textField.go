package gui

import (
	"mbc/gfx"

	"solod.dev/so/time"
)

func TextField(Text string, placeholder string, bbox gfx.Rectangle, Hovered, Enabled bool) {
	runes := []rune(Text)
	// blink every second
	blink := (time.Now().Second())%2 == 0

	const borderSize = 2
	border := bbox.Grow(borderSize)
	if Hovered || Enabled {
		gfx.DrawRectangle(border, gfx.White)
	} else {
		gfx.DrawRectangle(border, gfx.Gray)
	}

	gfx.DrawRectangle(bbox, gfx.Black)

	// align text
	font := ActivePack.Font()
	tBB := gfx.Rectangle{
		W: float32(font.TextWidth(runes)) * Scale,
		H: float32(font.TextHeight()) * Scale,
	}.Anchor(bbox, 0, .5)
	tBB.X += 4 * Scale
	// draw placeholder
	if !Enabled && len(Text) == 0 {
		font.DrawRunes([]rune(placeholder), tBB.Position(), Scale, 0, gfx.Gray, false)
	} else { // draw holding text
		font.DrawRunes(runes, tBB.Position(), Scale, 0, gfx.White, false)
	}
	if Enabled && blink {
		tBB.X += tBB.W
		font.DrawRunes([]rune{'_'}, tBB.Position(), Scale, 0, gfx.White, false)
	}
}
