package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
	"solod.dev/so/path"
)

func (p *DefaultTexturePack) Unload() {
	iter := p.Textures.Iter()
	defer p.Textures.Clear()
	for iter.Next() {
		gfx.UnloadTexture(iter.Value())
	}
}

// Destroy implements [TexturePack].
func (p *DefaultTexturePack) Destroy() {
	iter := p.Textures.Iter()
	for iter.Next() {
		gfx.UnloadTexture(iter.Value())
	}
	p.Textures.Free()
	p.font.Destroy()
}

// Description implements [TexturePack].
func (p *DefaultTexturePack) Description() string {
	return "The default look of Minecraft"
}

// GetTexture implements [TexturePack].
func (p *DefaultTexturePack) GetTexture(asset assets.ID) gfx.Texture {
	if p.Textures.Has(asset) {
		return p.Textures.Get(asset)
	}
	p.scratch.Reset()
	t, err := gfx.LoadTexture(path.Join(&p.scratch, "assets", asset.String()))
	if err != nil {
		return gfx.Texture{}
	}
	p.Textures.Set(asset, t)

	return t
}

// Icon implements [TexturePack].
func (p *DefaultTexturePack) Icon() gfx.Texture {
	if tex := p.GetTexture(assets.Pack); (tex != gfx.Texture{}) {
		return tex

	}
	panic("pack.png not found. assets are missing.")
}

// Font implements [TexturePack].
func (p *DefaultTexturePack) Font() *gfx.Font {
	p.scratch.Reset()
	if p.font.Atlas.ID != 0 {
		return &p.font
	}
	var err error
	p.font, err = gfx.LoadFont(path.Join(&p.scratch,
		"assets", "font", "default.png"))
	if err != nil {
		panic(err)
	}
	return &p.font
}

// Name implements [TexturePack].
func (p *DefaultTexturePack) Name() string {
	return "Default"
}

// memory for storing Maps in DefaultTexturePack
var _defaultTexturePackScratchBuffer = [1024 * 20]byte{}

func NewDefaultTexturePack() gfx.TexturePack {
	parent := mem.NewArena(_defaultTexturePackScratchBuffer[:])
	p := mem.Alloc[DefaultTexturePack](&parent)
	p.scratch = mem.NewArena(mem.AllocSlice[byte](&parent, 512, 512))
	p.Textures = maps.New[assets.ID, gfx.Texture](&parent, 100)
	return p
}
