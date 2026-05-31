package game

import (
	"mbc/gfx"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
)

type DefaultTexturePack struct {
	Textures maps.Map[string, gfx.Texture]
	scratch  mem.Arena
	font     gfx.Font
}

// Game state
var ___scratchBuf [1024 * 1024]byte // 1MiB
type State struct {
	Dt                        float32
	ScreenWidth, ScreenHeight int

	Pack    gfx.TexturePack
	Scratch mem.Arena
}
