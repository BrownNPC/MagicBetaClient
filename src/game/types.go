package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
)

const TextureLifetimeInFrames = 120

type DefaultTexturePack struct {
	Textures maps.Map[assets.ID, gfx.Texture]
	scratch  mem.Arena
	font     gfx.Font
}

const (
	SCREEN_MENU_MAIN = iota
	SCREEN_MENU_JOIN_SERVER
	SCREEN_MENU_TEXTURE_PACKS
	SCREEN_MENU_OPTIONS
)

type MenuMain struct {
	Buttons [3]bool
}

type InputType uint32
type Input struct {
	Pressed   bool
	Released  bool
	Direction gfx.Vector2
}

const (
	InputNone       InputType = iota
	InputLeftClick            // 0 for release, 1 for press
	InputRightClick           // 0 for release, 1 for press
	InputClose
	InputLook
	InputMove
	TotalInputs
)

// Game state
var ___scratchBuf [1024 * 1024]byte // 1MiB
type State struct {
	Dt                        float32
	ScreenWidth, ScreenHeight float32

	Pack        gfx.TexturePack
	Scratch     mem.Arena
	Cursor      gfx.Vector2
	ShowCursor  bool
	CursorDelta gfx.Vector2
	Screen      int
	Inputs      [TotalInputs]Input
	SplashText  string // splash text shown on main menu
}
