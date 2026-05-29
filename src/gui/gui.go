package gui

import "mbc/gfx"

var (
	guiBaseWidth, guiBaseHeight, guiOffsetX, guiOffsetY float32

	guiScale  int          //uses integer scaling
	ScaleFactor = uint(0) //0 means auto scale
)

func Init(baseWidth, baseHeight float32) {
	guiBaseWidth = baseWidth
	guiBaseHeight = baseHeight
}

// Must be at least called when screen size changes.
func Update(screenWidth, screenHeight int) {
	sx := float32(screenWidth) / guiBaseWidth
	sy := float32(screenHeight) / guiBaseHeight

	scale := max(int(min(sx, sy)), 1)

	guiScale = scale

	// center GUI
	guiW := guiBaseWidth * float32(scale)
	guiH := guiBaseHeight * float32(scale)

	guiOffsetX = (float32(screenWidth) - guiW) * 0.5
	guiOffsetY = (float32(screenHeight) - guiH) * 0.5
}

// convert GUI space co-ordinates to Screen space.
func ToScreen(x, y float32) (float32, float32) {
	return guiOffsetX + x*float32(guiScale),
		guiOffsetY + y*float32(guiScale)
}

// return scaling factor for GUI
func Scale() float32 {
	if ScaleFactor == 0 {
		return min(float32(guiScale),3)
	}
	return float32(ScaleFactor)
}

// Minecraft uses 256x256 atlas. But HD texture packs can have 512x512 atlases or other scaling factors.
// Use this to scale 256x256 assumptions.
func GetAtlasScale(t gfx.Texture) (float32, float32) {
	return float32(t.Width) / 256,
		float32(t.Height) / 256
}
