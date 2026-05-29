package gui

import "mbc/gfx"

func Button(GuiTexture gfx.Texture, X, Y, WidthScale float32, Hovered bool, Enabled bool) {
	state := float32(1)
	if !Enabled {
		state = 0
	} else if Hovered {
		state = 2
	}
	asX, asY := GetAtlasScale(GuiTexture)

	Width := min(200, 200*WidthScale) * Scale()
	Height := float32(20) * Scale()

	srcY := (46 + state*20) * asY
	srcWidth := 200 * asX
	srcHeight := 20 * asY
	dst1 := gfx.Rectangle{
		X:      X-Width/2,
		Y:      Y,
		Width:  Width / 2,
		Height: Height,
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
			X:      dst1.X + Width/2,
			Y:      Y,
			Width:  Width / 2,
			Height: Height,
		},
	)
}
