package gui

import "mbc/gfx"

const MaxAutoScale = 3

var Base = gfx.Rectangle{W: 320, H: 180}

var (
	ActivePack gfx.TexturePack
	// 0 = automatic scaling
	Scale float32
)

// Must be called whenever screen size changes.
func Update(screenW, screenH float32, pack gfx.TexturePack) {
	ActivePack = pack

	var scale int
	sx := screenW / Base.W
	sy := screenH / Base.H

	scale = int(min(sx, sy))

	scale = max(1, scale)
	Scale = min(float32(scale), MaxAutoScale)
}
