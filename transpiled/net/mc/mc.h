#pragma once
#include "so/builtin/builtin.h"
#include "net/net.h"
#include "sdl/sdl.h"
#include "so/c/c.h"
#include "so/encoding/binary/binary.h"
#include "so/errors/errors.h"
#include "so/fmt/fmt.h"
#include "so/io/io.h"
#include "so/math/math.h"
#include "so/mem/mem.h"
#include "so/slices/slices.h"

// -- Types --

typedef struct mc_String8Reader mc_String8Reader;
typedef struct mc_String16Reader mc_String16Reader;
typedef struct mc_EntityMetadata mc_EntityMetadata;
typedef struct mc_MetadataValue mc_MetadataValue;
typedef struct mc_MetadataReader mc_MetadataReader;
typedef struct mc_PacketKeepAlive mc_PacketKeepAlive;
typedef struct mc_ClientboundLogin mc_ClientboundLogin;
typedef struct mc_ServerboundLogin mc_ServerboundLogin;
typedef struct mc_ClientboundPreLogin mc_ClientboundPreLogin;
typedef struct mc_ServerboundPreLogin mc_ServerboundPreLogin;
typedef struct mc_PacketChatMessage mc_PacketChatMessage;
typedef struct mc_ClientboundSetTime mc_ClientboundSetTime;
typedef struct mc_ClientboundSetEquipment mc_ClientboundSetEquipment;
typedef struct mc_ClientboundSetSpawnPosition mc_ClientboundSetSpawnPosition;
typedef struct mc_ServerboundInteractWithEntity mc_ServerboundInteractWithEntity;
typedef struct mc_ClientboundSetHealth mc_ClientboundSetHealth;
typedef struct mc_PacketRespawn mc_PacketRespawn;
typedef struct mc_PacketPlayerMovement mc_PacketPlayerMovement;
typedef struct mc_PacketPlayerPosition mc_PacketPlayerPosition;
typedef struct mc_PacketPlayerRotation mc_PacketPlayerRotation;
typedef struct mc_PacketPlayerPositionAndRotation mc_PacketPlayerPositionAndRotation;
typedef struct mc_ServerboundMineBlock mc_ServerboundMineBlock;
typedef struct mc_ServerboundPlaceBlock mc_ServerboundPlaceBlock;
typedef struct mc_ServerboundSetHotbarSlot mc_ServerboundSetHotbarSlot;
typedef struct mc_ClientboundInteractWithBlock mc_ClientboundInteractWithBlock;
typedef struct mc_PacketAnimation mc_PacketAnimation;
typedef struct mc_ClientboundSpawnItem mc_ClientboundSpawnItem;
typedef struct mc_ClientboundCollectItem mc_ClientboundCollectItem;
typedef struct mc_ClientboundSpawnObject mc_ClientboundSpawnObject;
typedef struct mc_ClientBoundSpawnMob mc_ClientBoundSpawnMob;
typedef struct mc_ClientboundSpawnPainting mc_ClientboundSpawnPainting;
typedef struct mc_ClientboundEntityVelocity mc_ClientboundEntityVelocity;
typedef struct mc_ClientboundDespawnEntity mc_ClientboundDespawnEntity;
typedef struct mc_ClientboundEntityPosition mc_ClientboundEntityPosition;
typedef struct mc_ClientboundEntityRotation mc_ClientboundEntityRotation;
typedef struct mc_ClientboundEntityPositionAndRotation mc_ClientboundEntityPositionAndRotation;
typedef struct mc_ClientboundTeleportEntity mc_ClientboundTeleportEntity;
typedef struct mc_ClientboundEntityEvent mc_ClientboundEntityEvent;
typedef struct mc_ClientboundAddPassenger mc_ClientboundAddPassenger;
typedef struct mc_ServerboundPlayerAction mc_ServerboundPlayerAction;
typedef struct mc_ClientboundSetChunkVisibility mc_ClientboundSetChunkVisibility;
typedef struct mc_ClientboundSetBlock mc_ClientboundSetBlock;
typedef struct mc_ClientboundBlockEvent mc_ClientboundBlockEvent;
typedef struct mc_ClientboundWorldEvent mc_ClientboundWorldEvent;
typedef struct mc_ClientboundGameEvent mc_ClientboundGameEvent;
typedef struct mc_ClientboundLightningBolt mc_ClientboundLightningBolt;
typedef struct mc_PacketDisconnect mc_PacketDisconnect;
typedef uint8_t mc_BlockID;
typedef uint8_t mc_MobType;
typedef uint8_t mc_ObjectType;

typedef struct mc_Decoder {
    void* self;
    so_R_bool_err (*Step)(void* self, mem_Allocator a, net_BufferedReader* rd);
} mc_Decoder;

typedef struct mc_Encoder {
    void* self;
    so_Error (*Write)(void* self, io_Writer );
} mc_Encoder;
typedef so_Slice mc_String16;

// a zero value string16Reader is valid to use.
typedef struct mc_String8Reader {
    so_int step;
    so_int length;
    net_SteppedReader16 lenReader;
    net_SteppedReader byteReader;
    so_int bytesIndex;
    so_Slice bytes;
} mc_String8Reader;

// a zero value String16Reader is valid to use.
typedef struct mc_String16Reader {
    so_int step;
    so_int length;
    net_SteppedReader16 lenReader;
    net_SteppedReader16 ucs2Reader;
    so_int runesIndex;
    so_Slice Runes;
} mc_String16Reader;

typedef struct mc_EntityMetadata {
    bool Burning;
    bool Sneaking;
    bool Riding;
    so_int X;
    so_int Y;
    so_int Z;
    uint8_t SheepColor;
    bool Sheared;
    bool BlowingUp;
    bool Charged;
    bool Attacking;
    uint8_t Size;
    bool Saddled;
    bool Sitting;
    int32_t Health;
} mc_EntityMetadata;

typedef struct mc_MetadataValue {
    uint8_t ID;
    uint8_t DataType;
    so_byte Byte;
    net_SteppedReader rByte;
    int16_t Short;
    net_SteppedReader16 rShort;
    int32_t Integer;
    net_SteppedReader32 rInteger;
    float Float;
    net_SteppedReader32 rFloat;
    so_Slice String;
    mc_String16Reader rString;
    struct {
        int16_t ID;
        net_SteppedReader16 rID;
        uint8_t Quantity;
        net_SteppedReader rQuantity;
        uint16_t Metadata;
        net_SteppedReader16 rMetadata;
    } Item;
    struct {
        int32_t X;
        int32_t Y;
        int32_t Z;
        net_SteppedReader32 rX;
        net_SteppedReader32 rY;
        net_SteppedReader32 rZ;
    } Coordinates;
} mc_MetadataValue;

typedef struct mc_MetadataReader {
    so_Slice metadataValues;
    net_SteppedReader header;
    uint8_t dataType;
    uint8_t metadataID;
    mc_MetadataValue metadata;
    uint8_t state;
} mc_MetadataReader;

// https://pixelbrush.dev/beta-wiki/networking/packets/000-keep-alive
typedef struct mc_PacketKeepAlive {
    so_byte _;
} mc_PacketKeepAlive;

// https://pixelbrush.dev/beta-wiki/networking/packets/001-login
typedef struct mc_ClientboundLogin {
    net_SteppedReader32 entityID;
    int32_t EntityID;
    so_Slice Unused;
    mc_String16Reader unused;
    int64_t WorldSeed;
    net_SteppedReader64 worldSeed;
    uint8_t Dimension;
    net_SteppedReader dimension;
    so_int step;
} mc_ClientboundLogin;

typedef struct mc_ServerboundLogin {
    int32_t ProtocolVersion;
    so_Slice Username;
    int64_t _;
    so_byte __;
} mc_ServerboundLogin;

typedef struct mc_ClientboundPreLogin {
    so_Slice ConnectionHash;
    mc_String16Reader connectionHash;
} mc_ClientboundPreLogin;

typedef struct mc_ServerboundPreLogin {
    so_Slice Username;
} mc_ServerboundPreLogin;

typedef struct mc_PacketChatMessage {
    so_Slice Message;
    mc_String16Reader message;
} mc_PacketChatMessage;

typedef struct mc_ClientboundSetTime {
    int64_t Time;
    net_SteppedReader64 time;
} mc_ClientboundSetTime;

typedef struct mc_ClientboundSetEquipment {
    int32_t EntityID;
    net_SteppedReader32 entityID;
    int16_t InventorySlot;
    net_SteppedReader16 inventorySlot;
    int16_t ItemID;
    net_SteppedReader16 itemID;
    int16_t ItemMetadata;
    net_SteppedReader16 itemMetadata;
} mc_ClientboundSetEquipment;

typedef struct mc_ClientboundSetSpawnPosition {
    int32_t X;
    int32_t Y;
    int32_t Z;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
} mc_ClientboundSetSpawnPosition;

typedef struct mc_ServerboundInteractWithEntity {
    int32_t PlayerID;
    int32_t EntityID;
    bool Attack;
} mc_ServerboundInteractWithEntity;

typedef struct mc_ClientboundSetHealth {
    int16_t Health;
    net_SteppedReader16 health;
} mc_ClientboundSetHealth;

typedef struct mc_PacketRespawn {
    int8_t World;
    net_SteppedReader world;
} mc_PacketRespawn;

typedef struct mc_PacketPlayerMovement {
    bool OnGround;
    net_SteppedReader onGround;
} mc_PacketPlayerMovement;

typedef struct mc_PacketPlayerPosition {
    double X;
    double Y;
    double CameraY;
    double Z;
    net_SteppedReader64 x;
    net_SteppedReader64 y;
    net_SteppedReader64 cameraY;
    net_SteppedReader64 z;
    bool OnGround;
    net_SteppedReader onGround;
} mc_PacketPlayerPosition;

typedef struct mc_PacketPlayerRotation {
    float Yaw;
    float Pitch;
    net_SteppedReader32 yaw;
    net_SteppedReader32 pitch;
    bool OnGround;
    net_SteppedReader onGround;
} mc_PacketPlayerRotation;

typedef struct mc_PacketPlayerPositionAndRotation {
    mc_PacketPlayerPosition Position;
    mc_PacketPlayerRotation Rotation;
} mc_PacketPlayerPositionAndRotation;

typedef struct mc_ServerboundMineBlock {
    so_byte Status;
    int32_t X;
    so_byte Y;
    int32_t Z;
    so_byte Face;
} mc_ServerboundMineBlock;

typedef struct mc_ServerboundPlaceBlock {
    int32_t X;
    so_byte Y;
    int32_t Z;
    so_byte Face;
    int16_t BlockItemID;
    so_byte Amount;
    int16_t Metadata;
} mc_ServerboundPlaceBlock;

typedef struct mc_ServerboundSetHotbarSlot {
    int16_t Slot;
} mc_ServerboundSetHotbarSlot;

// Clientbound: Interact With Block / Sleep packet (0x11)
typedef struct mc_ClientboundInteractWithBlock {
    int32_t EntityID;
    so_byte Type;
    int32_t X;
    int32_t Y;
    int32_t Z;
    net_SteppedReader32 entityID;
    net_SteppedReader _type;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
} mc_ClientboundInteractWithBlock;

// Both: Animation (0x12)
typedef struct mc_PacketAnimation {
    int32_t PlayerID;
    so_byte Animation;
    net_SteppedReader32 playerID;
    net_SteppedReader animation;
} mc_PacketAnimation;

// Clientbound: Spawn Item (0x15)
typedef struct mc_ClientboundSpawnItem {
    int32_t EntityID;
    int16_t ItemID;
    so_byte Amount;
    int16_t Meta;
    int32_t X;
    int32_t Y;
    int32_t Z;
    so_byte Yaw;
    so_byte Pitch;
    so_byte Roll;
    net_SteppedReader32 entityID;
    net_SteppedReader16 itemID;
    net_SteppedReader amount;
    net_SteppedReader16 meta;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
    net_SteppedReader yaw;
    net_SteppedReader pitch;
    net_SteppedReader roll;
} mc_ClientboundSpawnItem;

// Clientbound: Collect Item (0x16)
typedef struct mc_ClientboundCollectItem {
    int32_t ItemEntityID;
    int32_t CollectorEntityID;
    net_SteppedReader32 itemEntityID;
    net_SteppedReader32 collectorEntityID;
} mc_ClientboundCollectItem;

// Clientbound: Spawn Object (0x17)
typedef struct mc_ClientboundSpawnObject {
    int32_t EntityID;
    so_byte ObjectType;
    int32_t X;
    int32_t Y;
    int32_t Z;
    so_byte Pitch;
    so_byte Yaw;
    net_SteppedReader32 entityID;
    net_SteppedReader objectType;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
    net_SteppedReader pitch;
    net_SteppedReader yaw;
} mc_ClientboundSpawnObject;

typedef struct mc_ClientBoundSpawnMob {
    int32_t EntityID;
    uint8_t MobType;
    int32_t X;
    int32_t Y;
    int32_t Z;
    float Yaw;
    float Pitch;
    mc_EntityMetadata Metadata;
    net_SteppedReader32 entityID;
    net_SteppedReader mobType;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
    net_SteppedReader yaw;
    net_SteppedReader pitch;
    mc_MetadataReader metadata;
} mc_ClientBoundSpawnMob;

// Clientbound: Spawn Painting (0x19)
typedef struct mc_ClientboundSpawnPainting {
    int32_t EntityID;
    so_Slice Title;
    mc_String16Reader titleReader;
    net_SteppedReader32 entityID;
} mc_ClientboundSpawnPainting;

// Clientbound: Entity Velocity (0x1C)
typedef struct mc_ClientboundEntityVelocity {
    int32_t EntityID;
    int16_t XV;
    int16_t YV;
    int16_t ZV;
    net_SteppedReader32 entityID;
    net_SteppedReader16 xv;
    net_SteppedReader16 yv;
    net_SteppedReader16 zv;
} mc_ClientboundEntityVelocity;

// Clientbound: Despawn Entity (0x1D)
typedef struct mc_ClientboundDespawnEntity {
    int32_t EntityID;
    net_SteppedReader32 entityID;
} mc_ClientboundDespawnEntity;

// Clientbound: Entity Position (0x1F)
typedef struct mc_ClientboundEntityPosition {
    int32_t EntityID;
    float X;
    float Y;
    float Z;
    net_SteppedReader32 entityID;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
} mc_ClientboundEntityPosition;

// Clientbound: Entity Rotation (0x20)
typedef struct mc_ClientboundEntityRotation {
    int32_t EntityID;
    so_byte Yaw;
    so_byte Pitch;
    net_SteppedReader32 entityID;
    net_SteppedReader yaw;
    net_SteppedReader pitch;
} mc_ClientboundEntityRotation;

// Clientbound: Entity Position and Rotation (0x21)
typedef struct mc_ClientboundEntityPositionAndRotation {
    int32_t EntityID;
    float X;
    float Y;
    float Z;
    so_byte Yaw;
    so_byte Pitch;
    net_SteppedReader32 entityID;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
    net_SteppedReader yaw;
    net_SteppedReader pitch;
} mc_ClientboundEntityPositionAndRotation;

// Clientbound: Teleport Entity (0x22)
typedef struct mc_ClientboundTeleportEntity {
    int32_t EntityID;
    int32_t X;
    int32_t Y;
    int32_t Z;
    so_byte Yaw;
    so_byte Pitch;
    net_SteppedReader32 entityID;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
    net_SteppedReader yaw;
    net_SteppedReader pitch;
} mc_ClientboundTeleportEntity;

// Clientbound: Entity Event (0x26)
typedef struct mc_ClientboundEntityEvent {
    int32_t EntityID;
    so_byte Action;
    net_SteppedReader32 entityID;
    net_SteppedReader action;
} mc_ClientboundEntityEvent;

// Clientbound: Add Passenger (0x27)
typedef struct mc_ClientboundAddPassenger {
    int32_t EntityID;
    int32_t VehicleID;
    net_SteppedReader32 entityID;
    net_SteppedReader32 vehicleID;
} mc_ClientboundAddPassenger;

// Serverbound: Player Action (0x13) struct and writer
typedef struct mc_ServerboundPlayerAction {
    int32_t EntityID;
    so_byte Action;
} mc_ServerboundPlayerAction;

// Clientbound: Set Chunk Visibility (0x32)
typedef struct mc_ClientboundSetChunkVisibility {
    int32_t X;
    int32_t Z;
    bool Load;
    net_SteppedReader32 x;
    net_SteppedReader32 z;
    net_SteppedReader l;
} mc_ClientboundSetChunkVisibility;

// Clientbound: Set Block (0x35)
typedef struct mc_ClientboundSetBlock {
    int32_t X;
    int16_t Y;
    int32_t Z;
    so_byte Type;
    so_byte Metadata;
    net_SteppedReader32 x;
    net_SteppedReader16 y;
    net_SteppedReader32 z;
    net_SteppedReader typeR;
    net_SteppedReader meta;
} mc_ClientboundSetBlock;

// Clientbound: Block Event (0x36)
typedef struct mc_ClientboundBlockEvent {
    int32_t X;
    int16_t Y;
    int32_t Z;
    so_byte A;
    so_byte B;
    net_SteppedReader32 x;
    net_SteppedReader16 y;
    net_SteppedReader32 z;
    net_SteppedReader a;
    net_SteppedReader b;
} mc_ClientboundBlockEvent;

// Clientbound: World Event (0x3D)
typedef struct mc_ClientboundWorldEvent {
    int32_t EffectID;
    int32_t X;
    so_byte Y;
    int32_t Z;
    int32_t Data;
    net_SteppedReader32 effectID;
    net_SteppedReader32 x;
    net_SteppedReader y;
    net_SteppedReader32 z;
    net_SteppedReader32 data;
} mc_ClientboundWorldEvent;

// Clientbound: Game Event (0x46)
typedef struct mc_ClientboundGameEvent {
    so_byte Type;
    net_SteppedReader typeR;
} mc_ClientboundGameEvent;

// Clientbound: Lightning Bolt (0x47)
typedef struct mc_ClientboundLightningBolt {
    int32_t EntityID;
    so_byte EntityType;
    int32_t X;
    int32_t Y;
    int32_t Z;
    net_SteppedReader32 entityID;
    net_SteppedReader entityType;
    net_SteppedReader32 x;
    net_SteppedReader32 y;
    net_SteppedReader32 z;
} mc_ClientboundLightningBolt;

// Both: Disconnect (0xFF)
typedef struct mc_PacketDisconnect {
    so_Slice Reason;
    mc_String16Reader reason;
} mc_PacketDisconnect;
typedef so_byte mc_PacketID;

// -- Variables and constants --
static const uint8_t mc_BLOCK_Air = 0;
static const uint8_t mc_BLOCK_Stone = 1;
static const uint8_t mc_BLOCK_Grass = 2;
static const uint8_t mc_BLOCK_Dirt = 3;
static const uint8_t mc_BLOCK_Cobblestone = 4;
static const uint8_t mc_BLOCK_Planks = 5;
static const uint8_t mc_BLOCK_Sapling = 6;
static const uint8_t mc_BLOCK_Bedrock = 7;
static const uint8_t mc_BLOCK_FlowingWater = 8;
static const uint8_t mc_BLOCK_StillWater = 9;
static const uint8_t mc_BLOCK_FlowingLava = 10;
static const uint8_t mc_BLOCK_StillLava = 11;
static const uint8_t mc_BLOCK_Sand = 12;
static const uint8_t mc_BLOCK_Gravel = 13;
static const uint8_t mc_BLOCK_GoldOre = 14;
static const uint8_t mc_BLOCK_IronOre = 15;
static const uint8_t mc_BLOCK_CoalOre = 16;
static const uint8_t mc_BLOCK_Log = 17;
static const uint8_t mc_BLOCK_Leaves = 18;
static const uint8_t mc_BLOCK_Sponge = 19;
static const uint8_t mc_BLOCK_Glass = 20;
static const uint8_t mc_BLOCK_LapisOre = 21;
static const uint8_t mc_BLOCK_LapisBlock = 22;
static const uint8_t mc_BLOCK_Dispenser = 23;
static const uint8_t mc_BLOCK_Sandstone = 24;
static const uint8_t mc_BLOCK_NoteBlock = 25;
static const uint8_t mc_BLOCK_Bed = 26;
static const uint8_t mc_BLOCK_PoweredRail = 27;
static const uint8_t mc_BLOCK_DetectorRail = 28;
static const uint8_t mc_BLOCK_StickyPiston = 29;
static const uint8_t mc_BLOCK_Cobweb = 30;
static const uint8_t mc_BLOCK_TallGrass = 31;
static const uint8_t mc_BLOCK_DeadBush = 32;
static const uint8_t mc_BLOCK_Piston = 33;
static const uint8_t mc_BLOCK_PistonHead = 34;
static const uint8_t mc_BLOCK_Wool = 35;
static const uint8_t mc_BLOCK_MovingBlock = 36;
static const uint8_t mc_BLOCK_Dandelion = 37;
static const uint8_t mc_BLOCK_Rose = 38;
static const uint8_t mc_BLOCK_BrownMushroom = 39;
static const uint8_t mc_BLOCK_RedMushroom = 40;
static const uint8_t mc_BLOCK_GoldBlock = 41;
static const uint8_t mc_BLOCK_IronBlock = 42;
static const uint8_t mc_BLOCK_DoubleSlab = 43;
static const uint8_t mc_BLOCK_Slab = 44;
static const uint8_t mc_BLOCK_Bricks = 45;
static const uint8_t mc_BLOCK_TNT = 46;
static const uint8_t mc_BLOCK_Bookshelf = 47;
static const uint8_t mc_BLOCK_MossyCobblestone = 48;
static const uint8_t mc_BLOCK_Obsidian = 49;
static const uint8_t mc_BLOCK_Torch = 50;
static const uint8_t mc_BLOCK_Fire = 51;
static const uint8_t mc_BLOCK_MonsterSpawner = 52;
static const uint8_t mc_BLOCK_WoodenStairs = 53;
static const uint8_t mc_BLOCK_Chest = 54;
static const uint8_t mc_BLOCK_RedstoneWire = 55;
static const uint8_t mc_BLOCK_DiamondOre = 56;
static const uint8_t mc_BLOCK_DiamondBlock = 57;
static const uint8_t mc_BLOCK_CraftingTable = 58;
static const uint8_t mc_BLOCK_Wheat = 59;
static const uint8_t mc_BLOCK_Farmland = 60;
static const uint8_t mc_BLOCK_Furnace = 61;
static const uint8_t mc_BLOCK_LitFurnace = 62;
static const uint8_t mc_BLOCK_StandingSign = 63;
static const uint8_t mc_BLOCK_WoodenDoor = 64;
static const uint8_t mc_BLOCK_Ladder = 65;
static const uint8_t mc_BLOCK_Rail = 66;
static const uint8_t mc_BLOCK_CobblestoneStairs = 67;
static const uint8_t mc_BLOCK_WallSign = 68;
static const uint8_t mc_BLOCK_Lever = 69;
static const uint8_t mc_BLOCK_StonePressurePlate = 70;
static const uint8_t mc_BLOCK_IronDoor = 71;
static const uint8_t mc_BLOCK_WoodenPressurePlate = 72;
static const uint8_t mc_BLOCK_RedstoneOre = 73;
static const uint8_t mc_BLOCK_LitRedstoneOre = 74;
static const uint8_t mc_BLOCK_RedstoneTorch = 75;
static const uint8_t mc_BLOCK_LitRedstoneTorch = 76;
static const uint8_t mc_BLOCK_StoneButton = 77;
static const uint8_t mc_BLOCK_SnowLayer = 78;
static const uint8_t mc_BLOCK_Ice = 79;
static const uint8_t mc_BLOCK_SnowBlock = 80;
static const uint8_t mc_BLOCK_Cactus = 81;
static const uint8_t mc_BLOCK_Clay = 82;
static const uint8_t mc_BLOCK_SugarCane = 83;
static const uint8_t mc_BLOCK_Jukebox = 84;
static const uint8_t mc_BLOCK_Fence = 85;
static const uint8_t mc_BLOCK_Pumpkin = 86;
static const uint8_t mc_BLOCK_Netherrack = 87;
static const uint8_t mc_BLOCK_SoulSand = 88;
static const uint8_t mc_BLOCK_Glowstone = 89;
static const uint8_t mc_BLOCK_NetherPortal = 90;
static const uint8_t mc_BLOCK_JackOLantern = 91;
static const uint8_t mc_BLOCK_Cake = 92;
static const uint8_t mc_BLOCK_RedstoneRepeater = 93;
static const uint8_t mc_BLOCK_LitRedstoneRepeater = 94;
static const uint8_t mc_BLOCK_LockedChest = 95;
static const uint8_t mc_BLOCK_Trapdoor = 96;
static const uint8_t mc_MOB_Creeper = 50;
static const uint8_t mc_MOB_Skeleton = 51;
static const uint8_t mc_MOB_Spider = 52;
static const uint8_t mc_MOB_GiantZombie = 53;
static const uint8_t mc_MOB_Zombie = 54;
static const uint8_t mc_MOB_Slime = 55;
static const uint8_t mc_MOB_Ghast = 56;
static const uint8_t mc_MOB_ZombiePigman = 57;
static const uint8_t mc_MOB_Pig = 90;
static const uint8_t mc_MOB_Sheep = 91;
static const uint8_t mc_MOB_Cow = 92;
static const uint8_t mc_MOB_Chicken = 93;
static const uint8_t mc_MOB_Squid = 94;
static const uint8_t mc_MOB_Wolf = 95;
static const uint8_t mc_OBJECT_Boat = 1;
static const uint8_t mc_OBJECT_Minecart = 10;
static const uint8_t mc_OBJECT_StorageMinecart = 11;
static const uint8_t mc_OBJECT_FurnaceMinecart = 12;
static const uint8_t mc_OBJECT_LitTNT = 50;
static const uint8_t mc_OBJECT_Arrow = 60;
static const uint8_t mc_OBJECT_ThrownSnowball = 61;
static const uint8_t mc_OBJECT_ThrownEgg = 62;
static const uint8_t mc_OBJECT_FallingSand = 70;
static const uint8_t mc_OBJECT_FallingGravel = 71;
static const uint8_t mc_OBJECT_FishingBobber = 90;
extern so_Error mc_IncompleteMetadataErr;
static const int64_t mc_MAX_PACKETS = 255;
static const so_byte mc_PKT_KeepAlive = 0x00;
static const so_byte mc_PKT_Login = 0x01;
static const so_byte mc_PKT_PreLogin = 0x02;
static const so_byte mc_PKT_ChatMessage = 0x03;
static const so_byte mc_PKT_SetTime = 0x04;
static const so_byte mc_PKT_SetEquipment = 0x05;
static const so_byte mc_PKT_SetSpawnPosition = 0x06;
static const so_byte mc_PKT_InteractWithEntity = 0x07;
static const so_byte mc_PKT_SetHealth = 0x08;
static const so_byte mc_PKT_Respawn = 0x09;
static const so_byte mc_PKT_PlayerMovement = 0x0A;
static const so_byte mc_PKT_PlayerPosition = 0x0B;
static const so_byte mc_PKT_PlayerRotation = 0x0C;
static const so_byte mc_PKT_PlayerPositionAndRotation = 0x0D;
static const so_byte mc_PKT_MineBlock = 0x0E;
static const so_byte mc_PKT_PlaceBlock = 0x0F;
static const so_byte mc_PKT_SetHotbarSlot = 0x10;
static const so_byte mc_PKT_InteractWithBlock = 0x11;
static const so_byte mc_PKT_Animation = 0x12;
static const so_byte mc_PKT_PlayerAction = 0x13;
static const so_byte mc_PKT_SpawnPlayer = 0x14;
static const so_byte mc_PKT_SpawnItem = 0x15;
static const so_byte mc_PKT_CollectItem = 0x16;
static const so_byte mc_PKT_SpawnObject = 0x17;
static const so_byte mc_PKT_SpawnMob = 0x18;
static const so_byte mc_PKT_SpawnPainting = 0x19;
static const so_byte mc_PKT_PlayerInput = 0x1B;
static const so_byte mc_PKT_EntityVelocity = 0x1C;
static const so_byte mc_PKT_DespawnEntity = 0x1D;
static const so_byte mc_PKT_EntityMovement = 0x1E;
static const so_byte mc_PKT_EntityPosition = 0x1F;
static const so_byte mc_PKT_EntityRotation = 0x20;
static const int64_t mc_PKT_EntityPositionAndRotationPacketID = 0x21;
static const so_byte mc_PKT_TeleportEntity = 0x22;
static const so_byte mc_PKT_EntityEvent = 0x26;
static const so_byte mc_PKT_AddPassenger = 0x27;
static const so_byte mc_PKT_EntityMetadata = 0x28;
static const so_byte mc_PKT_SetChunkVisibility = 0x32;
static const so_byte mc_PKT_Chunk = 0x33;
static const so_byte mc_PKT_SetMultipleBlocks = 0x34;
static const so_byte mc_PKT_SetBlock = 0x35;
static const so_byte mc_PKT_BlockEvent = 0x36;
static const so_byte mc_PKT_Explosion = 0x3C;
static const so_byte mc_PKT_WorldEvent = 0x3D;
static const so_byte mc_PKT_GameEvent = 0x46;
static const so_byte mc_PKT_LightningBolt = 0x47;
static const so_byte mc_PKT_OpenContainer = 0x64;
static const so_byte mc_PKT_CloseContainer = 0x65;
static const so_byte mc_PKT_ClickSlot = 0x66;
static const so_byte mc_PKT_SetSlot = 0x67;
static const so_byte mc_PKT_FillContainer = 0x68;
static const so_byte mc_PKT_ContainerData = 0x69;
static const so_byte mc_PKT_ContainerTransaction = 0x6A;
static const so_byte mc_PKT_UpdateSign = 0x82;
static const so_byte mc_PKT_ItemData = 0x83;
static const so_byte mc_PKT_IncrementStatistic = 0xC8;
static const so_byte mc_PKT_Disconnect = 0xFF;

// -- Functions and methods --

// rotation data is quantized to only a single 8-bit Byte.
float mc_UnquantizeAngle(so_byte angle);
mc_EntityMetadata mc_MetadataReader_Parse(void* self, uint8_t e);
so_R_bool_err mc_MetadataReader_Step(void* self, mem_Allocator a, net_BufferedReader* rd);

// Read implements [ClientBoundPacket].
so_R_bool_err mc_PacketKeepAlive_Step(void* self);

// Write implements [ServerBoundPacket].
so_Error mc_PacketKeepAlive_Write(mc_PacketKeepAlive p);
so_R_bool_err mc_ClientboundLogin_Step(void* self, mem_Allocator _, net_BufferedReader* r);
so_Error mc_ServerboundLogin_Write(mc_ServerboundLogin p, io_Writer w);
so_R_bool_err mc_ClientboundPreLogin_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_Error mc_ServerboundPreLogin_Write(mc_ServerboundPreLogin p, io_Writer w);
so_Error mc_PacketChatMessage_Write(void* self, io_Writer w);
so_R_bool_err mc_PacketChatMessage_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSetTime_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSetEquipment_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSetSpawnPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSetHealth_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketRespawn_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketPlayerMovement_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketPlayerPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketPlayerRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketPlayerPositionAndRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_Error mc_ServerboundMineBlock_Write(mc_ServerboundMineBlock p, io_Writer w);
so_Error mc_ServerboundPlaceBlock_Write(mc_ServerboundPlaceBlock p, io_Writer w);
so_Error mc_ServerboundSetHotbarSlot_Write(mc_ServerboundSetHotbarSlot p, io_Writer w);

// Serverbound: Interact With Entity (0x07)
so_Error mc_ServerboundInteractWithEntity_Write(mc_ServerboundInteractWithEntity p, io_Writer w);
so_R_bool_err mc_ClientboundInteractWithBlock_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketAnimation_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_Error mc_PacketAnimation_Write(mc_PacketAnimation p, io_Writer w);
so_R_bool_err mc_ClientboundSpawnItem_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundCollectItem_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSpawnObject_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientBoundSpawnMob_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSpawnPainting_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundEntityVelocity_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundDespawnEntity_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundEntityPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundEntityRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundEntityPositionAndRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundTeleportEntity_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundEntityEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundAddPassenger_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_Error mc_ServerboundPlayerAction_Write(mc_ServerboundPlayerAction p, io_Writer w);
so_R_bool_err mc_ClientboundSetChunkVisibility_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundSetBlock_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundBlockEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundWorldEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundGameEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_ClientboundLightningBolt_Step(void* self, mem_Allocator _, net_BufferedReader* rd);
so_R_bool_err mc_PacketDisconnect_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_Error mc_PacketDisconnect_Write(mc_PacketDisconnect p, io_Writer w);

// PacketRespawn (both directions)
so_Error mc_PacketRespawn_Write(void* self, io_Writer w);

// PacketPlayerMovement (both directions)
so_Error mc_PacketPlayerMovement_Write(void* self, io_Writer w);

// PacketPlayerPosition (both directions)
so_Error mc_PacketPlayerPosition_Write(void* self, io_Writer w);

// PacketPlayerRotation (both directions)
so_Error mc_PacketPlayerRotation_Write(void* self, io_Writer w);

// PacketPlayerPositionAndRotation (both directions)
so_Error mc_PacketPlayerPositionAndRotation_Write(void* self, io_Writer w);

// Returns a decoder for the given packet id. It is the user's job to free the decoder.
// Returns nil if packetID is invalid.
mc_Decoder mc_NewDecoder(mem_Allocator a, so_byte packetID);
so_String mc_PacketIDString(so_byte p);

// -------------------- BYTE --------------------
so_Error mc_WriteByte(io_Writer w, so_byte v);

// -------------------- UINT16 / INT16 --------------------
so_Error mc_WriteUnsignedShort(io_Writer w, uint16_t v);
so_Error mc_WriteShort(io_Writer w, int16_t v);

// -------------------- UINT32 / INT32 --------------------
so_Error mc_WriteUnsignedInteger(io_Writer w, uint32_t v);
so_Error mc_WriteInteger(io_Writer w, int32_t v);

// -------------------- UINT64 / INT64 --------------------
so_Error mc_WriteUnsignedLong(io_Writer w, uint64_t v);
so_Error mc_WriteLong(io_Writer w, int64_t v);

// -------------------- BOOL --------------------
so_Error mc_WriteBool(io_Writer w, bool v);

// -------------------- FLOAT / DOUBLE --------------------
so_Error mc_WriteFloat32(io_Writer w, float v);
so_Error mc_WriteFloat64(io_Writer w, double v);

// -------------------- STRING8 (UTF-8) --------------------
so_Error mc_WriteString8(io_Writer w, so_String s);
so_R_bool_err mc_String8Reader_Step(void* self, mem_Allocator a, net_BufferedReader* rd);

// -------------------- STRING16 (UCS-2 / UTF-16 subset) --------------------
so_R_bool_err mc_String16Reader_Step(void* self, mem_Allocator a, net_BufferedReader* rd);
so_Error mc_WriteString16(io_Writer w, so_Slice s);
