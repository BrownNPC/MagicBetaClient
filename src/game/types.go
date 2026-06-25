package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/mix"
	"mbc/sdl"

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
	Type      InputType
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

type ScreenJoinServerState struct {
	Buf       [4 * 120]byte
	Arena     mem.Arena
	PageIndex int //page number

	TextField        strings.Builder // text field on screen Join Server
	TextFieldFocused bool

	SelectedServer *cfg.ServerCfg
}

// Max number of sound effects that can be loaded at a time.
const MaxAudioLoaded = 20
const SCRATCH_SIZE = 1024 * 100 // size of the scratch memory arena in State

const ORG = "io.github.brownnpc"
const APP = "MagicBetaClient"
const CONFIG_FILE_PATH = "config.json"

// Game state
type State struct {
	Dt                        float32
	ScreenWidth, ScreenHeight float32
	TextInput                 bool // whether text input should be enabled.
	TargetFPS                 int
	Config                    cfg.Config

	Pack gfx.TexturePack

	___scratchBuf [SCRATCH_SIZE]byte
	// Lifetime of Scratch allocated objects should be the same as stack allocated objects.
	Scratch mem.Arena
	Storage *sdl.Storage // Title storage

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
