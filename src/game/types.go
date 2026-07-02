package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/mix"
	"mbc/net"
	"mbc/net/mc"
	"mbc/sdl"

	"solod.dev/so/bufio"
	"solod.dev/so/maps"
	"solod.dev/so/mem"
	"solod.dev/so/time"
)

const TextureLifetimeInFrames = 120

type DefaultTexturePack struct {
	Textures maps.Map[assets.ID, gfx.Texture]
	scratch  mem.Arena
	font     gfx.Font
}

// max characters that can be inputted into a text field.
const MAX_TEXT_INPUT = 256

type TextInputBuffer struct {
	Text [MAX_TEXT_INPUT]rune
	Len  int
}

func (t *TextInputBuffer) Init(s string) {
	*t = TextInputBuffer{}

	for _, r := range []rune(s) {
		if t.Len == MAX_TEXT_INPUT {
			break
		}
		t.Text[t.Len] = r
		t.Len++
	}
}

func (t *TextInputBuffer) Add(r rune) {
	if t.Len == MAX_TEXT_INPUT {
		return
	}
	t.Text[t.Len] = r
	t.Len++
}

func (t *TextInputBuffer) Pop() {
	if t.Len == 0 {
		return
	}
	t.Len--
	t.Text[t.Len] = 0
}

func (t TextInputBuffer) String() string {
	return string(t.Text[:t.Len])
}

const (
	SCREEN_MENU_MAIN = iota
	// these are inside of MENU_MAIN
	SCREEN_MENU_SELECT_SERVER
	SCREEN_MENU_TEXTURE_PACKS
	SCREEN_MENU_OPTIONS

	// Inside of SCREEN_MENU_SELECT_SERVER
	SCREEN_JOIN_SERVER
	SCREEN_CONNECT_SERVER
	SCREEN_INGAME
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
	InputTap
	InputReturn //enter, or X on controller
	InputRightClick

	// arrow keys or dpad
	InputUp
	InputDown
	InputLeft
	InputRight

	InputBackspace
	InputClose
	InputLook
	InputMove
	InputTextInput // Text input
	TotalInputs
)

// since there are only 3 sound tracks. and all of them are well
// under 5 minutes. We can rol a dice and decide whether to play music or not every 5 minutes
// without having to track if a song is already playing.
const RollMusicEvery = time.Minute * 5

type ScreenMainMenuState struct {
	selected int
}
type ScreenSelectServerState struct {
	selected  int
	PageIndex int //page number
}
type ScreenJoinServerState struct {
	selected         int
	HaveInitialized  bool
	ShouldTransition bool
	switchToScreen   int
	// Text field
	// 0: nil text field
	// 1: Hostname text field
	// 2: Cmd text field
	TextFields [3]TextInputBuffer
	// 0: none focused
	// 1: Hostname text field
	// 2: Cmd text field
	TextFieldFocused uint
}
type ScreenConnectServerState struct {
	ShouldTransision bool
	TransisionTo     int
	Dialed           bool
	Text             string

	__ArenaBuf [512]byte
	Arena      mem.Arena

	packetID net.SteppedReader

	stage                int
	serverbound_prelogin mc.ServerboundPreLogin
	clientbound_prelogin mc.ClientboundPreLogin
	serverbound_login    mc.ServerboundLogin
	clientbound_login    mc.ClientboundLogin
}
type Kind int
type Thing struct {
	Kind     Kind
	Pos      gfx.Vector3
	Rotation gfx.Vector3
}
type ThingRef struct {
	idx, gen uint
}

var NilRef = ThingRef{}

const MAX_THINGS = 4096

const (
	KindNull Kind = iota
)

type ThingPool struct {
	// Index 0 is a nil Thing
	Things [MAX_THINGS]Thing
	gen    [MAX_THINGS]uint
	used   [MAX_THINGS]bool

	FreeListMemory [MAX_THINGS]uint
	FreeListCursor uint
	FreeListLen    int
	SlotsUsed      uint
}

func (things *ThingPool) New(kind Kind) ThingRef {
	var i uint = 0
	if things.FreeListLen > 0 {
		things.FreeListCursor--
		things.FreeListLen--
		i = things.FreeListMemory[things.FreeListCursor]
	} else {
		i = things.SlotsUsed + 1
	}
	things.Things[i] = Thing{}
	things.Things[i].Kind = kind
	things.used[i] = true
	things.SlotsUsed++
	return ThingRef{idx: i, gen: things.gen[i]}
}
func (things *ThingPool) Delete(ref ThingRef) {
	if ref.gen == things.gen[ref.idx] {
		things.used[ref.idx] = false
		things.gen[ref.idx] += 1
		things.SlotsUsed--
		things.FreeListMemory[things.FreeListCursor] = ref.idx
		things.FreeListCursor++
		things.FreeListLen++
	}
}

type ThingsIter struct {
	p   *ThingPool
	idx uint
}

func (it *ThingsIter) Thing() *Thing {
	return &it.p.Things[it.idx]
}
func (it *ThingsIter) Ref() ThingRef {
	return ThingRef{idx: it.idx, gen: it.p.gen[it.idx]}
}

func (it *ThingsIter) Next() bool {
	it.idx++
	// Find the next valid item
	for it.idx < MAX_THINGS {
		if it.p.used[it.idx] {
			return true
		}
		it.idx++
	}
	return false
}

func (things *ThingPool) Iter() ThingsIter {
	return ThingsIter{p: things}
}

// PacketHandler is called whenever a new packet arrives
type PacketHandler func(data mc.Decoder)
type DecodeState int

const (
	DECODE_WAITING DecodeState = iota
	DECODE_DECODING
	DECODE_HANDLING
)

type ScreenInGameState struct {
	Initialized  bool
	Disconnected bool

	Error error

	Things ThingPool

	Cam gfx.Camera
	// Player spawn position
	SpawnPosition gfx.Vector3

	__PacketDecodeArenaMemory [100 * 1024]byte
	PacketDecodeArena         mem.Arena

	__PersistentMemory [2 * 1024 * 1024]byte
	// PersistentArena lives for as long as the user is on this screen.
	PersistentArena mem.Arena

	PacketID       mc.PacketID
	DecodeState    DecodeState
	Decoder        mc.Decoder
	// will be used when https://github.com/solod-dev/solod/issues/83 is fixed
	PacketHandlers [mc.MAX_PACKETS]PacketHandler
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
	TextInputActive           bool // whether text input should be enabled.
	TargetFPS                 int
	Config                    cfg.Config

	// Moving with dpad
	InteractingWithUI bool // is interacting with UI
	UIDpadMode        bool

	Pack gfx.TexturePack

	___scratchBuf [SCRATCH_SIZE]byte
	// Lifetime of Scratch allocated objects should be the same as stack allocated objects.
	Scratch mem.Arena
	Storage *sdl.Storage // Title storage

	Cursor         gfx.Vector2
	ShowCursor     bool
	CursorDelta    gfx.Vector2
	CurrentScreeen int
	// Inputs are parsed from SDL events in main.go
	Inputs     [TotalInputs]Input
	SplashText string // splash text shown on main menu

	Mixer *mix.Mixer // global mixer

	TimeSinceLastBackgroundMusicRoll time.Time  // when did we roll to play background music
	MusicTrack                       *mix.Track // track that plays background classic Minecraft music on loop.

	Audios     maps.Map[assets.ID, *mix.Audio]
	TracksPool [10]*mix.Track

	SelectedServer           uint // index into Config.Servers
	ScreenMainMenuState      ScreenMainMenuState
	ScreenSelectServerState  ScreenSelectServerState
	ScreenJoinServerState    ScreenJoinServerState
	ScreenConnectServerState ScreenConnectServerState
	ScreenInGameState        ScreenInGameState

	// SHOULD NOT BE USED FOR READ/WRITE DIRECTLY
	Conn net.Conn
	// Backed by Conn
	__bufioWriterBuffer   [1024 * 10]byte
	__bufioReaderBuffer   [1024 * 10]byte
	__arenaForServerbound mem.Arena
	__arenaForClientbound mem.Arena
	ServerBound           bufio.Writer
	ClientBound           bufio.Reader
}
