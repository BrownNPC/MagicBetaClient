package game

import (
	"mbc/gfx"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
	"solod.dev/so/path"
)

// Unload implements [TexturePack].
func (p *DefaultTexturePack) Unload() {
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
func (p *DefaultTexturePack) GetTexture(name string) gfx.Texture {
	if p.Textures.Has(name) {
		return p.Textures.Get(name)
	}
	p.scratch.Reset()

	t, err := gfx.LoadTexture(path.Join(&p.scratch, "assets", name))
	if err != nil {
		return gfx.Texture{}
	}

	p.Textures.Set(name, t)
	return t
}

// Icon implements [TexturePack].
func (p *DefaultTexturePack) Icon() gfx.Texture {
	if tex := p.GetTexture("/pack.png"); (tex != gfx.Texture{}) {
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

var _defaultTexturePackScratchBuffer = [1024 * 1024 * 2]byte{}

func NewDefaultTexturePack() gfx.TexturePack {
	parent := mem.NewArena(_defaultTexturePackScratchBuffer[:])
	p := mem.Alloc[DefaultTexturePack](&parent)
	// minecraft has 76 pngs:
	// find assets -type f -name "*.png" | wc -l
	p.Textures = maps.New[string, gfx.Texture](&parent, 1)
	p.scratch = mem.NewArena(mem.AllocSlice[byte](&parent, 512, 512))
	return p
}
