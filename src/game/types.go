package game

import (
	"mbc/gfx"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
)

type TexturePack interface {
	Icon() gfx.Texture // should always return a valid texture.
	Name() string // name of the pack
	Description() string // Description of the pack
	GetTexture(path string) gfx.Texture // will return zero value if not found.
	Unload() // free all textures and memory.
}

type DefaultTexturePack struct {
	Textures maps.Map[string, gfx.Texture]
	scratch  mem.Arena
}
// Game state
type State struct {
	Dt float32
	ScreenWidth,ScreenHeight int

	Pack TexturePack
}
