package mc

import "mbc/sdl"

type BlockID = uint8

const (
	BLOCK_Air BlockID = 0

	BLOCK_Stone       BlockID = 1
	BLOCK_Grass       BlockID = 2
	BLOCK_Dirt        BlockID = 3
	BLOCK_Cobblestone BlockID = 4
	BLOCK_Planks      BlockID = 5
	BLOCK_Sapling     BlockID = 6
	BLOCK_Bedrock     BlockID = 7

	BLOCK_FlowingWater BlockID = 8
	BLOCK_StillWater   BlockID = 9
	BLOCK_FlowingLava  BlockID = 10
	BLOCK_StillLava    BlockID = 11

	BLOCK_Sand                BlockID = 12
	BLOCK_Gravel              BlockID = 13
	BLOCK_GoldOre             BlockID = 14
	BLOCK_IronOre             BlockID = 15
	BLOCK_CoalOre             BlockID = 16
	BLOCK_Log                 BlockID = 17
	BLOCK_Leaves              BlockID = 18
	BLOCK_Sponge              BlockID = 19
	BLOCK_Glass               BlockID = 20
	BLOCK_LapisOre            BlockID = 21
	BLOCK_LapisBlock          BlockID = 22
	BLOCK_Dispenser           BlockID = 23
	BLOCK_Sandstone           BlockID = 24
	BLOCK_NoteBlock           BlockID = 25
	BLOCK_Bed                 BlockID = 26
	BLOCK_PoweredRail         BlockID = 27
	BLOCK_DetectorRail        BlockID = 28
	BLOCK_StickyPiston        BlockID = 29
	BLOCK_Cobweb              BlockID = 30
	BLOCK_TallGrass           BlockID = 31
	BLOCK_DeadBush            BlockID = 32
	BLOCK_Piston              BlockID = 33
	BLOCK_PistonHead          BlockID = 34
	BLOCK_Wool                BlockID = 35
	BLOCK_MovingBlock         BlockID = 36
	BLOCK_Dandelion           BlockID = 37
	BLOCK_Rose                BlockID = 38
	BLOCK_BrownMushroom       BlockID = 39
	BLOCK_RedMushroom         BlockID = 40
	BLOCK_GoldBlock           BlockID = 41
	BLOCK_IronBlock           BlockID = 42
	BLOCK_DoubleSlab          BlockID = 43
	BLOCK_Slab                BlockID = 44
	BLOCK_Bricks              BlockID = 45
	BLOCK_TNT                 BlockID = 46
	BLOCK_Bookshelf           BlockID = 47
	BLOCK_MossyCobblestone    BlockID = 48
	BLOCK_Obsidian            BlockID = 49
	BLOCK_Torch               BlockID = 50
	BLOCK_Fire                BlockID = 51
	BLOCK_MonsterSpawner      BlockID = 52
	BLOCK_WoodenStairs        BlockID = 53
	BLOCK_Chest               BlockID = 54
	BLOCK_RedstoneWire        BlockID = 55
	BLOCK_DiamondOre          BlockID = 56
	BLOCK_DiamondBlock        BlockID = 57
	BLOCK_CraftingTable       BlockID = 58
	BLOCK_Wheat               BlockID = 59
	BLOCK_Farmland            BlockID = 60
	BLOCK_Furnace             BlockID = 61
	BLOCK_LitFurnace          BlockID = 62
	BLOCK_StandingSign        BlockID = 63
	BLOCK_WoodenDoor          BlockID = 64
	BLOCK_Ladder              BlockID = 65
	BLOCK_Rail                BlockID = 66
	BLOCK_CobblestoneStairs   BlockID = 67
	BLOCK_WallSign            BlockID = 68
	BLOCK_Lever               BlockID = 69
	BLOCK_StonePressurePlate  BlockID = 70
	BLOCK_IronDoor            BlockID = 71
	BLOCK_WoodenPressurePlate BlockID = 72
	BLOCK_RedstoneOre         BlockID = 73
	BLOCK_LitRedstoneOre      BlockID = 74
	BLOCK_RedstoneTorch       BlockID = 75
	BLOCK_LitRedstoneTorch    BlockID = 76
	BLOCK_StoneButton         BlockID = 77
	BLOCK_SnowLayer           BlockID = 78
	BLOCK_Ice                 BlockID = 79
	BLOCK_SnowBlock           BlockID = 80
	BLOCK_Cactus              BlockID = 81
	BLOCK_Clay                BlockID = 82
	BLOCK_SugarCane           BlockID = 83
	BLOCK_Jukebox             BlockID = 84
	BLOCK_Fence               BlockID = 85
	BLOCK_Pumpkin             BlockID = 86
	BLOCK_Netherrack          BlockID = 87
	BLOCK_SoulSand            BlockID = 88
	BLOCK_Glowstone           BlockID = 89
	BLOCK_NetherPortal        BlockID = 90
	BLOCK_JackOLantern        BlockID = 91
	BLOCK_Cake                BlockID = 92
	BLOCK_RedstoneRepeater    BlockID = 93
	BLOCK_LitRedstoneRepeater BlockID = 94
	BLOCK_LockedChest         BlockID = 95
	BLOCK_Trapdoor            BlockID = 96
)

//so:include math_include.h
type Direction = uint8

type AtlasUV struct {
	UV [4]float32
}

// algorithm that minecraft uses.
func getUV(id int) AtlasUV {
	const (
		atlasSize = 256.0
		tileSize  = 16.0
	)

	x := float32(id & 15)
	y := float32(id >> 4)

	var a [4]float32

	a[0] = x * tileSize / atlasSize
	a[1] = y * tileSize / atlasSize
	a[2] = (x*tileSize + tileSize) / atlasSize
	a[3] = (y*tileSize + tileSize) / atlasSize

	return AtlasUV{UV: a}
}

const (
	DIRECTION_Down  Direction = 0
	DIRECTION_Up    Direction = 1
	DIRECTION_North Direction = 2
	DIRECTION_South Direction = 3
	DIRECTION_West  Direction = 4
	DIRECTION_East  Direction = 5
)

func GetUVFromBlockSideAndMetadata(b BlockID, side Direction, metadata int) AtlasUV {
	// Ref: https://github.com/search?q=repo%3Ajacobo-mc%2Fmc_b1.7.3_release+getBlockTextureFromSideAndMetadata&type=code
	switch b {
	case BLOCK_Log:
		if side == DIRECTION_Up || side == DIRECTION_Down {
			return getUV(21)
		} else if metadata == 1 {
			return getUV(116)
		} else {
			// wood type
			switch metadata {
			case 1:
				return getUV(116)
			case 2:
				return getUV(117)
			default:
				return getUV(20)
			}
		}
	}
	sdl.Log("mc: could not find atlas uv for %d", b)
	panic("Atlas mapping not available")
}
