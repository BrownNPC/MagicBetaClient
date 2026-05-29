package gui

import "mbc/gfx"

func Button(GuiTexture gfx.Texture, X, Y, ScalePercent float32, Hovered bool, Enabled bool) {
	state := float32(1)
	if !Enabled {
		state = 0
	} else if Hovered {
		state = 2
	}
	asX, asY := GetAtlasScale(GuiTexture)
	const maxWidth float32 = 200.0
	Width := ScalePercent * 0.01 * 200
	Height := float32(20)

	srcY := (46 + state*20) * asY
	srcWidth := 200 * asX
	srcHeight := 20 * asY
	dst1 := gfx.Rectangle{
		X:      X,
		Y:      Y,
		Width:  min(Width/2, maxWidth/2) * Scale(),
		Height: Height * Scale(),
	}
	gfx.DrawTextureRec(
		GuiTexture,
		gfx.Rectangle{
			X:      0,
			Y:      srcY,
			Width:  srcWidth / 2,
			Height: srcHeight,
		},
		dst1,
	)
	gfx.DrawTextureRec(
		GuiTexture,
		gfx.Rectangle{
			X:      srcWidth / 2,
			Y:      srcY,
			Width:  srcWidth / 2,
			Height: srcHeight,
		},
		gfx.Rectangle{
			X:      X + dst1.Width,
			Y:      Y,
			Width:  min(Width/2, maxWidth/2) * Scale(),
			Height: Height * Scale(),
		},
	)
}
