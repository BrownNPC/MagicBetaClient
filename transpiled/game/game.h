#pragma once
#include "so/builtin/builtin.h"
#include "cfg/cfg.h"
#include "gfx/assets/assets.h"
#include "gfx/gfx.h"
#include "gui/gui.h"
#include "mix/mix.h"
#include "net/mc/mc.h"
#include "net/net.h"
#include "sdl/sdl.h"
#include "so/bufio/bufio.h"
#include "so/bytes/bytes.h"
#include "so/c/c.h"
#include "so/errors/errors.h"
#include "so/fmt/fmt.h"
#include "so/maps/maps.h"
#include "so/math/rand/rand.h"
#include "so/mem/mem.h"
#include "so/path/path.h"
#include "so/strconv/strconv.h"
#include "so/strings/strings.h"
#include "so/time/time.h"

// -- Types --

typedef struct game_DefaultTexturePack game_DefaultTexturePack;
typedef struct game_TextInputBuffer game_TextInputBuffer;
typedef struct game_Input game_Input;
typedef struct game_ScreenMainMenuState game_ScreenMainMenuState;
typedef struct game_ScreenSelectServerState game_ScreenSelectServerState;
typedef struct game_ScreenJoinServerState game_ScreenJoinServerState;
typedef struct game_ScreenConnectServerState game_ScreenConnectServerState;
typedef struct game_Thing game_Thing;
typedef struct game_ThingRef game_ThingRef;
typedef struct game_ThingPool game_ThingPool;
typedef struct game_ThingsIter game_ThingsIter;
typedef struct game_ScreenInGameState game_ScreenInGameState;
typedef struct game_State game_State;

typedef struct game_DefaultTexturePack {
    maps_Map Textures;
    mem_Arena scratch;
    gfx_Font font;
} game_DefaultTexturePack;

typedef struct game_TextInputBuffer {
    so_rune Text[256];
    so_int Len;
} game_TextInputBuffer;
typedef uint32_t game_InputType;

typedef struct game_Input {
    bool Pressed;
    bool Released;
    so_rune Text;
    gfx_Vector2 Direction;
} game_Input;

typedef struct game_ScreenMainMenuState {
    so_int selected;
} game_ScreenMainMenuState;

typedef struct game_ScreenSelectServerState {
    so_int selected;
    so_int PageIndex;
} game_ScreenSelectServerState;

typedef struct game_ScreenJoinServerState {
    so_int selected;
    bool HaveInitialized;
    bool ShouldTransition;
    so_int switchToScreen;
    game_TextInputBuffer TextFields[3];
    so_uint TextFieldFocused;
} game_ScreenJoinServerState;

typedef struct game_ScreenConnectServerState {
    bool ShouldTransision;
    so_int TransisionTo;
    bool Dialed;
    so_String Text;
    so_byte __ArenaBuf[512];
    mem_Arena Arena;
    net_SteppedReader packetID;
    so_int stage;
    mc_ServerboundPreLogin serverbound_prelogin;
    mc_ClientboundPreLogin clientbound_prelogin;
    mc_ServerboundLogin serverbound_login;
    mc_ClientboundLogin clientbound_login;
} game_ScreenConnectServerState;
typedef so_int game_Kind;

typedef struct game_Thing {
    game_Kind Kind;
    gfx_Vector3 Pos;
    gfx_Vector3 Rotation;
} game_Thing;

typedef struct game_ThingRef {
    so_uint idx;
    so_uint gen;
} game_ThingRef;

typedef struct game_ThingPool {
    game_Thing Things[4096];
    so_uint gen[4096];
    bool used[4096];
    so_uint FreeListMemory[4096];
    so_uint FreeListCursor;
    so_int FreeListLen;
    so_uint SlotsUsed;
} game_ThingPool;

typedef struct game_ThingsIter {
    game_ThingPool* p;
    so_uint idx;
} game_ThingsIter;

// PacketHandler is called whenever a new packet arrives
typedef void (*game_PacketHandler)(mc_Decoder);

typedef struct game_ScreenInGameState {
    bool Initialized;
    bool Disconnected;
    so_Error Error;
    game_ThingPool Things;
    gfx_Camera Cam;
    gfx_Vector3 SpawnPosition;
    int64_t Time;
    so_byte __PacketDecodeArenaMemory[102400];
    mem_Arena PacketDecodeArena;
    so_byte PacketID;
    so_int DecodeState;
    mc_Decoder Decoder;
    mc_ClientboundSetChunkVisibility scv;
    so_byte __PersistentMemory[2097152];
    mem_Arena PersistentArena;
} game_ScreenInGameState;

// Game state
typedef struct game_State {
    float Dt;
    float ScreenWidth;
    float ScreenHeight;
    bool TextInputActive;
    so_int TargetFPS;
    cfg_Config Config;
    bool InteractingWithUI;
    bool UIDpadMode;
    gfx_TexturePack Pack;
    so_byte ___scratchBuf[102400];
    mem_Arena Scratch;
    SDL_Storage* Storage;
    gfx_Vector2 Cursor;
    bool ShowCursor;
    gfx_Vector2 CursorDelta;
    so_int CurrentScreeen;
    game_Input Inputs[13];
    so_String SplashText;
    MIX_Mixer* Mixer;
    time_Time TimeSinceLastBackgroundMusicRoll;
    MIX_Track* MusicTrack;
    maps_Map Audios;
    MIX_Track* TracksPool[10];
    so_uint SelectedServer;
    game_ScreenMainMenuState ScreenMainMenuState;
    game_ScreenSelectServerState ScreenSelectServerState;
    game_ScreenJoinServerState ScreenJoinServerState;
    game_ScreenConnectServerState ScreenConnectServerState;
    game_ScreenInGameState ScreenInGameState;
    net_Conn Conn;
    so_byte __bufioWriterBuffer[10240];
    so_byte __bufioReaderBuffer[41960];
    mem_Arena __arenaForServerbound;
    mem_Arena __arenaForClientbound;
    bufio_Writer ServerBound;
    net_BufferedReader ClientBound;
} game_State;

// -- Variables and constants --
extern so_Error game_NoDecoderForPacketErr;
static const int64_t game_TextureLifetimeInFrames = 120;

// max characters that can be inputted into a text field.
static const int64_t game_MAX_TEXT_INPUT = 256;
static const int64_t game_SCREEN_MENU_MAIN = 0;
static const int64_t game_SCREEN_MENU_SELECT_SERVER = 1;
static const int64_t game_SCREEN_MENU_TEXTURE_PACKS = 2;
static const int64_t game_SCREEN_MENU_OPTIONS = 3;
static const int64_t game_SCREEN_JOIN_SERVER = 4;
static const int64_t game_SCREEN_CONNECT_SERVER = 5;
static const int64_t game_SCREEN_INGAME = 6;
static const game_InputType game_InputNone = 0;
static const game_InputType game_InputTap = 1;
static const game_InputType game_InputReturn = 2;
static const game_InputType game_InputRightClick = 3;
static const game_InputType game_InputUp = 4;
static const game_InputType game_InputDown = 5;
static const game_InputType game_InputLeft = 6;
static const game_InputType game_InputRight = 7;
static const game_InputType game_InputBackspace = 8;
static const game_InputType game_InputClose = 9;
static const game_InputType game_InputLook = 10;
static const game_InputType game_InputMove = 11;
static const game_InputType game_InputTextInput = 12;
static const game_InputType game_TotalInputs = 13;

// since there are only 3 sound tracks. and all of them are well
// under 5 minutes. We can rol a dice and decide whether to play music or not every 5 minutes
// without having to track if a song is already playing.
static const time_Duration game_RollMusicEvery = time_Minute * 5;
extern game_ThingRef game_NilRef;
static const int64_t game_MAX_THINGS = 4096;
static const game_Kind game_KindNull = 0;

// Max number of sound effects that can be loaded at a time.
static const int64_t game_MaxAudioLoaded = 20;

// size of the scratch memory arena in State
static const int64_t game_SCRATCH_SIZE = 1024 * 100;
static const so_String game_ORG = so_str("io.github.brownnpc");
static const so_String game_APP = so_str("MagicBetaClient");
static const so_String game_CONFIG_FILE_PATH = so_str("config.json");

// -- Functions and methods --
void game_DefaultTexturePack_Unload(void* self);

// Destroy implements [TexturePack].
void game_DefaultTexturePack_Destroy(void* self);

// Description implements [TexturePack].
so_String game_DefaultTexturePack_Description(void* self);

// GetTexture implements [TexturePack].
gfx_Texture game_DefaultTexturePack_GetTexture(void* self, assets_ID asset);

// Icon implements [TexturePack].
gfx_Texture game_DefaultTexturePack_Icon(void* self);

// Font implements [TexturePack].
gfx_Font* game_DefaultTexturePack_Font(void* self);

// Name implements [TexturePack].
so_String game_DefaultTexturePack_Name(void* self);
gfx_TexturePack game_NewDefaultTexturePack(void);
void game_State_Init(void* self);

// return false to quit.
bool game_State_Update(void* self);
void game_State_Screen_ConnectServer(void* self, game_ScreenConnectServerState* state, gfx_Rectangle screen);
void game_ScreenInGameState_Init(void* self, game_State* s);
void game_State_Screen_InGame(void* self, game_ScreenInGameState* state, gfx_Rectangle screen);
so_Error game_ScreenInGameState_DecodePackets(void* self, game_State* s);

// Show disconnected screen.
void game_ScreenInGameState_OnDisconnect(void* self, game_State* s, gfx_Rectangle screen);
void game_ScreenInGameState_OnSetSpawnPosition(void* self, mc_Decoder data);
void game_ScreenInGameState_OnSetTime(void* self, mc_Decoder data);
void game_ScreenInGameState_OnSpawnMob(void* self, mc_Decoder data);
void game_State_Screen_JoinServer(void* self, game_ScreenJoinServerState* state, gfx_Rectangle screen);
void game_State_Screen_MenuMain(void* self, game_ScreenMainMenuState* state, gfx_Rectangle screen);
so_String game_State_LoadRandomSplashText(void* self);
void game_State_Screen_SelectServer(void* self, game_ScreenSelectServerState* state, gfx_Rectangle screen);

// should be ran every frame
void game_State_RollBackgroundMusic(void* self);
MIX_Track* game_State_PlaySoundEffect(void* self, assets_ID audio);
void game_TextInputBuffer_Init(void* self, so_String s);
void game_TextInputBuffer_Add(void* self, so_rune r);
void game_TextInputBuffer_Pop(void* self);
so_String game_TextInputBuffer_String(game_TextInputBuffer t);
game_ThingRef game_ThingPool_New(void* self, game_Kind kind);
void game_ThingPool_Delete(void* self, game_ThingRef ref);
game_Thing* game_ThingsIter_Thing(void* self);
game_ThingRef game_ThingsIter_Ref(void* self);
bool game_ThingsIter_Next(void* self);
game_ThingsIter game_ThingPool_Iter(void* self);
