package mc

type MobType = uint8
type ObjectType = uint8

const (
	// Mobs: https://pixelbrush.dev/beta-wiki/entities/mobs
	MOB_Creeper      MobType = 50
	MOB_Skeleton     MobType = 51
	MOB_Spider       MobType = 52
	MOB_GiantZombie  MobType = 53
	MOB_Zombie       MobType = 54
	MOB_Slime        MobType = 55
	MOB_Ghast        MobType = 56
	MOB_ZombiePigman MobType = 57
	MOB_Pig          MobType = 90
	MOB_Sheep        MobType = 91
	MOB_Cow          MobType = 92
	MOB_Chicken      MobType = 93
	MOB_Squid        MobType = 94
	MOB_Wolf         MobType = 95
)
const (
	// Objects: https://pixelbrush.dev/beta-wiki/entities/objects
	OBJECT_Boat            ObjectType = 1
	OBJECT_Minecart        ObjectType = 10
	OBJECT_StorageMinecart ObjectType = 11
	OBJECT_FurnaceMinecart ObjectType = 12
	OBJECT_LitTNT          ObjectType = 50
	OBJECT_Arrow           ObjectType = 60
	OBJECT_ThrownSnowball  ObjectType = 61
	OBJECT_ThrownEgg       ObjectType = 62
	OBJECT_FallingSand     ObjectType = 70
	OBJECT_FallingGravel   ObjectType = 71
	OBJECT_FishingBobber   ObjectType = 90
)
