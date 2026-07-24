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
	"solod.dev/so/math"

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
	// within screen ingame
	SCREEN_INGAME_DISCONNECTED_SCREEN
	SCREEN_INGAME_PAUSED_SCREEN
)

type InputType uint32
type Input struct {
	Down bool
	Up   bool
	Text rune // for text input
	// used for input look
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
	InputMoveForward
	InputMoveBackward
	InputMoveLeft
	InputMoveRight
	InputJump
	InputSneak
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
	selected         int
	ShouldTransision bool
	TransisionTo     int
	Dialed           bool
	Err              error

	__ArenaBuf [512]byte
	Arena      mem.Arena

	packetID byte

	stage   int
	Decoder mc.Decoder
}
type Kind int
type Thing struct {
	Kind     Kind
	Username string
	EntityID int32 // minecraft server EntityID
	Position gfx.Vector3
	OnGround bool
}

// deallocate any heap memory used by this thing.
func (thing *Thing) deallocateHeapMemoryIfUsed() {
	if thing.Username != "" {
		mem.FreeString(mem.System, thing.Username)
	}
}

type ThingRef struct {
	idx, gen uint
}

var NilRef = ThingRef{}

const MAX_THINGS = 1024

const (
	KindNull Kind = iota
	KindEntity
	KindPlayer
)

type ThingPool struct {
	// Index 0 is a nil Thing
	Things [MAX_THINGS]Thing
	gen    [MAX_THINGS]uint
	used   [MAX_THINGS]bool

	FreeListMemory [MAX_THINGS]uint
	FreeListCursor uint
	SlotsUsed      uint
}

func (things *ThingPool) New(kind Kind) ThingRef {
	var i uint = 0
	if things.FreeListCursor > 0 {
		things.FreeListCursor--
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
		things.Things[ref.idx].deallocateHeapMemoryIfUsed()
		things.used[ref.idx] = false
		things.gen[ref.idx] += 1
		things.SlotsUsed--
		things.FreeListMemory[things.FreeListCursor] = ref.idx
		things.FreeListCursor++
	}
}
func (things *ThingPool) Get(ref ThingRef) *Thing {
	if !things.used[ref.idx] || ref.gen != things.gen[ref.idx] {
		ref.idx = 0
		things.Things[0] = Thing{}
	}

	return &things.Things[ref.idx]
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

// A ChunkCoordinate represents a chunk in Chunk space.
// To convert world chunk coordinates to Chunk space:
//
//	X >> 4
//	Z >> 4
type ChunkCoordinate struct{ X, Z int32 }

var CHUNK_SPHERE_RADIUS float32

func init() {
	CHUNK_SPHERE_RADIUS = float32(math.Sqrt(8*8 + 64*64 + 8*8))
}

type Chunk struct {
	NeedMeshRebuild bool

	coord ChunkCoordinate
	mesh  *gfx.Mesh
	data  *mc.DecompressedChunkData
}

// GetPosition returns chunk position in world coordinates.
func (chunk *Chunk) GetPosition() gfx.Vector3 {
	return gfx.NewVector3(float32(chunk.coord.X*16), 0, float32(chunk.coord.Z*16))
}

// GetCenter returns chunk center in world coordinates.
func (chunk *Chunk) GetCenter() gfx.Vector3 {
	return gfx.NewVector3(
		float32(chunk.coord.X*16+8),
		64,
		float32(chunk.coord.Z*16+8),
	)
}
func (chunk *Chunk) Render() {

}

type ScreenInGameState struct {
	s *State
	// STATE BOOK KEEPING
	CurrentScreen int
	selected      int
	Initialized   bool
	Error         error
	Things        ThingPool
	// GAME STATE
	Cam    gfx.Camera
	Player ThingRef

	LastPositionUpdate time.Time

	// Player spawn position
	SpawnPosition gfx.Vector3
	acc           float32

	// Used for celestial angle and day/night cycle
	GameTimeFloat  float32 // In game time but it's a float
	GameTicksInt   int64   // game time in ticks.
	LastTimeUpdate time.Time

	// RENDERING DATA
	Stars   *gfx.Mesh // Initialized
	SunMesh *gfx.Mesh // Also used for moon

	// -----NETWORKING RELATED FIELDS ----
	__PacketDecodeArenaMemory [100 * 1024]byte
	PacketDecodeArena         mem.Arena

	PacketID    mc.PacketID
	DecodeState int // used in DecodePackets()
	Decoder     mc.Decoder
	scv         mc.ClientboundSetChunkVisibility

	__PersistentMemory [2 * 1024 * 1024]byte
	// PersistentArena lives for as long as the user is on this screen.
	PersistentArena mem.Arena

	Chunks        maps.Map[ChunkCoordinate, *Chunk]
	ChunkFreeList []*Chunk

	GrassChunk *Chunk
}

// Max number of sound effects that can be loaded at a time.
const MaxAudioLoaded = 20
const SCRATCH_SIZE = 1024 * 100 // size of the scratch memory arena in State

const ORG = "io.github.brownnpc"
const APP = "MagicBetaClient"
const CONFIG_FILE_PATH = "config.json"

// Game state
type State struct {
	Dt float32
	//Dt as time.Duration
	FrameTime time.Duration

	TargetFrameTime           time.Duration // set automatically
	TargetFPS                 int
	ScreenWidth, ScreenHeight float32
	TextInputActive           bool // whether text input should be enabled.
	Config                    cfg.Config

	// Moving with dpad
	InteractingWithUI bool // is interacting with UI
	MouseLock         bool // lock mouse (FPS mode)
	UIDpadMode        bool

	Pack gfx.TexturePack

	___scratchBuf [SCRATCH_SIZE]byte
	// Lifetime of Scratch allocated objects should be the same as stack allocated objects.
	Scratch mem.Arena
	Storage *sdl.Storage // Title storage

	Cursor gfx.Vector2

	ShowCursor     bool
	CurrentScreeen int
	// Inputs are parsed from SDL events in main.go
	Inputs          [TotalInputs]Input
	InputsPrevFrame [TotalInputs]Input

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
	__bufioReaderBuffer   [net.DefaultBufSize + 1000]byte
	__arenaForServerbound mem.Arena
	__arenaForClientbound mem.Arena

	ServerBound bufio.Writer
	ClientBound net.BufferedReader
}

func (s *State) InputPressed(i InputType) bool { return !s.InputsPrevFrame[i].Down && s.Inputs[i].Down }
func (s *State) InputReleased(i InputType) bool {
	return s.InputsPrevFrame[i].Down && !s.Inputs[i].Down
}
func (s *State) InputHeld(i InputType) bool { return s.Inputs[i].Down }

// ProcessDpadUIInput updates the selected UI element.
func (s *State) ProcessDpadUIInput(nInteractables int, selected *int) {
	if !s.UIDpadMode {
		return
	}

	delta := 0

	if s.InputPressed(InputDown) || s.InputPressed(InputRight) {
		delta = 1
	} else if s.InputPressed(InputUp) || s.InputPressed(InputLeft) {
		delta = -1
	}

	newSelection := *selected + delta
	if delta != 0 && newSelection >= 0 && newSelection < nInteractables {
		*selected = newSelection
		s.PlaySoundEffect(assets.Newsound_step_stone3)
		s.TextInputActive = false // Stop typing if focus moves
	}
}
