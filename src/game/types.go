package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
)

const TextureLifetimeInFrames = 120

type DefaultTexturePack struct {
	Textures        maps.Map[assets.ID, gfx.Texture]
	scratch         mem.Arena
	font            gfx.Font
}

// Game state
var ___scratchBuf [1024 * 1024]byte // 1MiB
type State struct {
	Dt                        float32
	ScreenWidth, ScreenHeight float32

	Pack    gfx.TexturePack
	Scratch mem.Arena
}
