package gui

import "mbc/gfx"

const MaxAutoScale = 3

var (
	guiBaseWidth, guiBaseHeight float32
	guiOffsetX, guiOffsetY      float32
	screenWidth, screenHeight   float32

	// Final integer GUI scale actually in use.
	guiScale   int
	ActivePack gfx.TexturePack
	AtlasScale float32

	// 0 = automatic scaling
	ScaleFactor uint // manually set scale factor.
)

func Init(baseWidth, baseHeight float32) {
	guiBaseWidth = baseWidth
	guiBaseHeight = baseHeight
}

// Must be called whenever screen size changes.
func Update(screenW, screenH int, pack gfx.TexturePack) {
	if ScaleFactor != 0 { // manual scale factor
		guiScale = int(ScaleFactor)
		return
	}
	ActivePack = pack
	AtlasScale = float32(pack.GetTexture("/gui/gui.png").Width / 255)

	screenWidth = float32(screenW)
	screenHeight = float32(screenH)

	var scale int
	sx := float32(screenW) / guiBaseWidth
	sy := float32(screenH) / guiBaseHeight

	scale = int(min(sx, sy))

	scale = max(1, scale)

	guiScale = min(scale, MaxAutoScale)
}

// Current GUI scale factor
func Scale() float32 {
	return float32(guiScale)
}

// Minecraft uses a 256x256 atlas.
// HD texture packs may use larger atlases.
