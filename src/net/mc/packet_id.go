package mc

type PacketID byte

const (
	KeepAlive                         PacketID = 0x00
	Login                             PacketID = 0x01
	PreLogin                          PacketID = 0x02
	ChatMessage                       PacketID = 0x03
	SetTime                           PacketID = 0x04
	SetEquipment                      PacketID = 0x05
	SetSpawnPosition                  PacketID = 0x06
	InteractWithEntity                PacketID = 0x07
	SetHealth                         PacketID = 0x08
	Respawn                           PacketID = 0x09
	PlayerMovement                    PacketID = 0x0A
	PlayerPosition                    PacketID = 0x0B
	PlayerRotation                    PacketID = 0x0C
	PlayerPositionAndRotation         PacketID = 0x0D
	MineBlock                         PacketID = 0x0E
	PlaceBlock                        PacketID = 0x0F
	SetHotbarSlot                     PacketID = 0x10
	InteractWithBlock                 PacketID = 0x11
	Animation                         PacketID = 0x12
	PlayerAction                      PacketID = 0x13
	SpawnPlayer                       PacketID = 0x14
	SpawnItem                         PacketID = 0x15
	CollectItem                       PacketID = 0x16
	SpawnObject                       PacketID = 0x17
	SpawnMob                          PacketID = 0x18
	SpawnPainting                     PacketID = 0x19
	PlayerInput                       PacketID = 0x1B // Unused, only implemented on the Client
	EntityVelocity                    PacketID = 0x1C
	DespawnEntity                     PacketID = 0x1D
	EntityMovement                    PacketID = 0x1E // Unused, in practice
	EntityPosition                    PacketID = 0x1F
	EntityRotation                    PacketID = 0x20
	EntityPositionAndRotationPacketID          = 0x21
	TeleportEntity                    PacketID = 0x22
	EntityEvent                       PacketID = 0x26
	AddPassenger                      PacketID = 0x27
	EntityMetadata                    PacketID = 0x28
	SetChunkVisibility                PacketID = 0x32
	Chunk                             PacketID = 0x33
	SetMultipleBlocks                 PacketID = 0x34
	SetBlock                          PacketID = 0x35
	BlockEvent                        PacketID = 0x36
	Explosion                         PacketID = 0x3C
	WorldEvent                        PacketID = 0x3D
	GameEvent                         PacketID = 0x46
	LightningBolt                     PacketID = 0x47
	OpenContainer                     PacketID = 0x64
	CloseContainer                    PacketID = 0x65
	ClickSlot                         PacketID = 0x66
	SetSlot                           PacketID = 0x67
	FillContainer                     PacketID = 0x68
	ContainerData                     PacketID = 0x69
	ContainerTransaction              PacketID = 0x6A
	UpdateSign                        PacketID = 0x82
	ItemData                          PacketID = 0x83
	IncrementStatistic                PacketID = 0xC8
	Disconnect                        PacketID = 0xFF
)
