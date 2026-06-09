package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/mix"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
	"solod.dev/so/strings"
	"solod.dev/so/time"
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

type InputType uint32
type Input struct {
	Pressed   bool
	Released  bool
	Text      rune // for text input
	Direction gfx.Vector2
}

const (
	InputNone InputType = iota
	InputLeftClick
	InputRightClick
	InputClose
	InputLook
	InputMove
	InputText // Text input
	TotalInputs
)

// since there are only 3 sound tracks. and all of them are well
// under 5 minutes. We can rol a dice and decide whether to play music or not every 5 minutes
// without having to track if a song is already playing.
const RollMusicEvery = time.Minute * 5

var ___scratchBuf [1024 * 1024]byte // 1MiB
type ScreenJoinServerState struct {
	Buf   [4 * 120]byte
	Arena mem.Arena

	TextField        strings.Builder // text field on screen Join Server
	TextFieldFocused bool
}

// Max number of sound effects that can be loaded at a time.
const MaxAudioLoaded = 50

// Game state
type State struct {
	Dt                        float32
	ScreenWidth, ScreenHeight float32
	TextInput                 bool // whether text input should be enabled.
	TargetFPS                 int

	Pack           gfx.TexturePack
	Scratch        mem.Arena
	Cursor         gfx.Vector2
	ShowCursor     bool
	CursorDelta    gfx.Vector2
	CurrentScreeen int
	Inputs         [TotalInputs]Input
	SplashText     string // splash text shown on main menu

	Mixer *mix.Mixer // global mixer

	TimeSinceLastBackgroundMusicRoll time.Time  // when did we roll to play background music
	MusicTrack                       *mix.Track // track that plays background classic Minecraft music on loop.

	Audios     maps.Map[assets.ID, *mix.Audio]
	TracksPool [10]*mix.Track

	ScreenJoinServer ScreenJoinServerState
}
