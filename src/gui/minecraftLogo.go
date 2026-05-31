package gui

import "mbc/gfx"

// size of Minecraft Logo
var MinecraftLogoSize = gfx.Vector2{
	X: 155 + 120,
	Y: 45,
}

func MinecraftLogo(bbox gfx.Rectangle) {
	logoTexture := ActivePack.GetTexture("/title/mclogo.png")

	// first half of logo
	logoSrc1 := gfx.Rectangle{
		W: 155,
		H: 45,
	}.Scale(AtlasScale)
	logoSrc2 := gfx.Rectangle{
		X: 0, Y: 45,
		W: 120, H: 90,
	}.Scale(AtlasScale)

	dst1 := gfx.Rectangle{
		W: 155 * Scale(), H: 45 * Scale(),
	}.SetPosition(bbox.Position())

	gfx.DrawTextureEx(logoTexture, logoSrc1, dst1)
	dst2 := gfx.Rectangle{
		X: bbox.X + dst1.W,
		Y: bbox.Y,
		W: 120, H: 90,
	}.Scale(Scale())
	gfx.DrawTextureEx(logoTexture, logoSrc2, dst2)
}
