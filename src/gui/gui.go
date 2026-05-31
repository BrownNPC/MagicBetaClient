package gui

import "mbc/gfx"

const MaxAutoScale = 3

const (
	BaseWidth  float32 = 320
	BaseHeight float32 = 180
)

var (
	ActivePack gfx.TexturePack
	// 0 = automatic scaling
	Scale float32
)

// Must be called whenever screen size changes.
func Update(screenW, screenH float32, pack gfx.TexturePack) {
	ActivePack = pack

	var scale int
	sx := screenW / BaseWidth
	sy := screenH / BaseHeight

	scale = int(min(sx, sy))

	scale = max(1, scale)

	Scale = min(float32(scale), MaxAutoScale)
}
