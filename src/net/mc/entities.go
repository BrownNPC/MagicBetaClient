package mc

type EntityType = uint8

const (
	// Mobs: https://pixelbrush.dev/beta-wiki/entities/mobs
	MOB_Creeper      EntityType = 50
	MOB_Skeleton     EntityType = 51
	MOB_Spider       EntityType = 52
	MOB_GiantZombie  EntityType = 53
	MOB_Zombie       EntityType = 54
	MOB_Slime        EntityType = 55
	MOB_Ghast        EntityType = 56
	MOB_ZombiePigman EntityType = 57
	MOB_Pig          EntityType = 90
	MOB_Sheep        EntityType = 91
	MOB_Cow          EntityType = 92
	MOB_Chicken      EntityType = 93
	MOB_Squid        EntityType = 94
	MOB_Wolf         EntityType = 95
	// Objects: https://pixelbrush.dev/beta-wiki/entities/objects
	OBJECT_Boat            EntityType = 1
	OBJECT_Minecart        EntityType = 10
	OBJECT_StorageMinecart EntityType = 11
	OBJECT_FurnaceMinecart EntityType = 12
	OBJECT_LitTNT          EntityType = 50
	OBJECT_Arrow           EntityType = 60
	OBJECT_ThrownSnowball  EntityType = 61
	OBJECT_ThrownEgg       EntityType = 62
	OBJECT_FallingSand     EntityType = 70
	OBJECT_FallingGravel   EntityType = 71
	OBJECT_FishingBobber   EntityType = 90
)
