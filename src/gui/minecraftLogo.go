package gui

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"solod.dev/so/math"
	"time"
)

// size of Minecraft Logo
var MinecraftLogoSize = gfx.Rectangle{
	W: 155 + 120,
	H: 45,
}

func MinecraftLogo(Splash string, bbox gfx.Rectangle) {
	logoTexture := ActivePack.GetTexture(assets.Title_mclogo)
	as := float32(logoTexture.Width) / 256

	logoSrc1 := gfx.Rectangle{W: 155, H: 45}.Scale(as)
	logoSrc2 := gfx.Rectangle{X: 0, Y: 45, W: 120, H: 90}.Scale(as)

	dst1 := gfx.Rectangle{W: 155 * Scale, H: 45 * Scale}.SetPosition(bbox.Position())
	gfx.DrawTextureEx(logoTexture, logoSrc1, dst1)

	dst2 := gfx.Rectangle{
		X: bbox.X + dst1.W,
		Y: bbox.Y,
		W: 120, H: 90,
	}.Scale(Scale)
	gfx.DrawTextureEx(logoTexture, logoSrc2, dst2)

	font := ActivePack.Font()
	t := float64(time.Now().UnixMilli()%1000) / 1000.0

	wave := math.Sin(t*2*math.Pi) * 0.2
	scale := float32(float64(Scale) - math.Abs(wave))+.5

	textSize := font.TextSize([]rune(Splash)).Scale(scale)
	textBBox := gfx.Rectangle{}.SetSize(textSize).
		Anchor(bbox, .75, .75).
		AddPosition(textSize.Half())

	font.DrawRunes([]rune(Splash), textBBox.Position(), float32(scale), -20, gfx.Yellow, false)
}
