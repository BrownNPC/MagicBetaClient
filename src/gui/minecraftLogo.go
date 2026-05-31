package gui

import (
	"mbc/gfx"
	"mbc/gfx/assets"
)

// size of Minecraft Logo
var MinecraftLogoSize = gfx.Rectangle{
	W: 155 + 120,
	H: 45,
}

func MinecraftLogo(bbox gfx.Rectangle) {
	logoTexture := ActivePack.GetTexture(assets.Title_mclogo)
	as := float32(logoTexture.Width) / 256 // atlas scale

	// first half of logo
	logoSrc1 := gfx.Rectangle{
		W: 155,
		H: 45,
	}.Scale(as)
	logoSrc2 := gfx.Rectangle{
		X: 0, Y: 45,
		W: 120, H: 90,
	}.Scale(as)

	dst1 := gfx.Rectangle{
		W: 155 * Scale, H: 45 * Scale,
	}.SetPosition(bbox.Position())

	gfx.DrawTextureEx(logoTexture, logoSrc1, dst1)
	dst2 := gfx.Rectangle{
		X: bbox.X + dst1.W,
		Y: bbox.Y,
		W: 120, H: 90,
	}.Scale(Scale)
	gfx.DrawTextureEx(logoTexture, logoSrc2, dst2)
}
