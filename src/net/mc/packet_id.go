package mc

type PacketID byte

const (
	PKT_KeepAlive                         PacketID = 0x00
	PKT_Login                             PacketID = 0x01
	PKT_PreLogin                          PacketID = 0x02
	PKT_ChatMessage                       PacketID = 0x03
	PKT_SetTime                           PacketID = 0x04
	PKT_SetEquipment                      PacketID = 0x05
	PKT_SetSpawnPosition                  PacketID = 0x06
	PKT_InteractWithEntity                PacketID = 0x07
	PKT_SetHealth                         PacketID = 0x08
	PKT_Respawn                           PacketID = 0x09
	PKT_PlayerMovement                    PacketID = 0x0A
	PKT_PlayerPosition                    PacketID = 0x0B
	PKT_PlayerRotation                    PacketID = 0x0C
	PKT_PlayerPositionAndRotation         PacketID = 0x0D
	PKT_MineBlock                         PacketID = 0x0E
	PKT_PlaceBlock                        PacketID = 0x0F
	PKT_SetHotbarSlot                     PacketID = 0x10
	PKT_InteractWithBlock                 PacketID = 0x11
	PKT_Animation                         PacketID = 0x12
	PKT_PlayerAction                      PacketID = 0x13
	PKT_SpawnPlayer                       PacketID = 0x14
	PKT_SpawnItem                         PacketID = 0x15
	PKT_CollectItem                       PacketID = 0x16
	PKT_SpawnObject                       PacketID = 0x17
	PKT_SpawnMob                          PacketID = 0x18
	PKT_SpawnPainting                     PacketID = 0x19
	PKT_PlayerInput                       PacketID = 0x1B // Unused, only implemented on the Client
	PKT_EntityVelocity                    PacketID = 0x1C
	PKT_DespawnEntity                     PacketID = 0x1D
	PKT_EntityMovement                    PacketID = 0x1E // Unused, in practice
	PKT_EntityPosition                    PacketID = 0x1F
	PKT_EntityRotation                    PacketID = 0x20
	PKT_EntityPositionAndRotationPacketID          = 0x21
	PKT_TeleportEntity                    PacketID = 0x22
	PKT_EntityEvent                       PacketID = 0x26
	PKT_AddPassenger                      PacketID = 0x27
	PKT_EntityMetadata                    PacketID = 0x28
	PKT_SetChunkVisibility                PacketID = 0x32
	PKT_Chunk                             PacketID = 0x33
	PKT_SetMultipleBlocks                 PacketID = 0x34
	PKT_SetBlock                          PacketID = 0x35
	PKT_BlockEvent                        PacketID = 0x36
	PKT_Explosion                         PacketID = 0x3C
	PKT_WorldEvent                        PacketID = 0x3D
	PKT_GameEvent                         PacketID = 0x46
	PKT_LightningBolt                     PacketID = 0x47
	PKT_OpenContainer                     PacketID = 0x64
	PKT_CloseContainer                    PacketID = 0x65
	PKT_ClickSlot                         PacketID = 0x66
	PKT_SetSlot                           PacketID = 0x67
	PKT_FillContainer                     PacketID = 0x68
	PKT_ContainerData                     PacketID = 0x69
	PKT_ContainerTransaction              PacketID = 0x6A
	PKT_UpdateSign                        PacketID = 0x82
	PKT_ItemData                          PacketID = 0x83
	PKT_IncrementStatistic                PacketID = 0xC8
	PKT_Disconnect                        PacketID = 0xFF
)
