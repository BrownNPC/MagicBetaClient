package game

import (
	"mbc/gfx"
	"mbc/net/mc"
	"mbc/sdl"

	"solod.dev/so/mem"
	"solod.dev/so/slices"
	"solod.dev/so/strings"
	"solod.dev/so/time"
)

func (state *ScreenInGameState) OnSetSpawnPosition(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetSpawnPosition)
	state.SpawnPosition = gfx.NewVector3(float32(pkt.X), float32(pkt.Y), float32(pkt.Z))
}
func (state *ScreenInGameState) OnSetTime(data mc.Decoder) error {
	pkt := data.(*mc.ClientboundSetTime)
	state.LastTimeUpdate = time.Now()
	state.GameTicksInt = pkt.Time
	return mc.PacketKeepAlive{}.Write(&state.s.ServerBound)
}

func (state *ScreenInGameState) OnSpawnMob(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSpawnMob)
	ref := state.Things.New(KindEntity)
	e := state.Things.Get(ref)
	e.EntityID = pkt.EntityID
	e.Position = gfx.NewVector3(float32(pkt.X), float32(pkt.Y), float32(pkt.Z))
}
func (state *ScreenInGameState) OnDespawnEntity(data mc.Decoder) {
	pkt := data.(*mc.ClientboundDespawnEntity)
	it := state.Things.Iter()
	for it.Next() {
		e := it.Thing()
		if e.Kind == KindEntity && e.EntityID == pkt.EntityID {
			state.Things.Delete(it.Ref())
			return
		}
	}
}

// spawn player entity
func (state *ScreenInGameState) OnSpawnPlayer(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSpawnPlayer)
	ref := state.Things.New(KindEntity)
	plr := state.Things.Get(ref)
	plr.Position = gfx.NewVector3(float32(pkt.X), float32(pkt.Y), float32(pkt.Z))
	plr.EntityID = pkt.EntityID
	plr.Username = strings.Clone(mem.System, string(pkt.Username))
}
func (state *ScreenInGameState) OnPlayerRotation(pitch, yaw float32) {
	state.LastPlayerPitch = pitch
	state.LastPlayerYaw = yaw
	state.Cam.Update(gfx.Vector3{}, gfx.NewVector3(yaw, pitch, 0), 0)
}

func (state *ScreenInGameState) OnPlayerPosition(X, Y, Z float32, camY float32) {
	state.LastPlayerPosition = state.Things.Get(state.Player).Position
	plr := state.Things.Get(state.Player)
	plr.Position = gfx.NewVector3(X, Y, Z)
}
func (state *ScreenInGameState) OnEntityPosition(entityID int32, pos mc.ClientboundEntityPosition) {
	i := state.Things.Iter()
	for i.Next() {
		e := i.Thing()
		if e.EntityID == entityID {
			// Add the unquantized packet movement delta directly to the entity
			movementDelta := gfx.NewVector3(pos.X, pos.Y, pos.Z)
			e.Position = e.Position.Add(movementDelta)
			return
		}
	}
}
func (state *ScreenInGameState) OnTeleportEntity(data mc.Decoder) {
	pkt := data.(*mc.ClientboundTeleportEntity)
	i := state.Things.Iter()
	for i.Next() {
		e := i.Thing()
		if e.EntityID == pkt.EntityID {
			// Add the unquantized packet movement delta directly to the entity
			e.Position = gfx.NewVector3(pkt.X, pkt.Y, pkt.Z)
			return
		}
	}
}
func (state *ScreenInGameState) OnSetChunkVisibility(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetChunkVisibility)
	// this packet uses chunk space while the other Chunk packet uses block space/world space
	coord := ChunkCoordinate{pkt.X, pkt.Z}
	if !pkt.Load { // unload
		if !state.Chunks.Has(coord) {
			sdl.Log("WARNING: Server told us to unload a chunk we dont have loaded.")
			return
		}
		chunk := state.Chunks.Get(coord)
		state.Chunks.Delete(coord)
		state.ChunkFreeList = slices.Append(mem.System, state.ChunkFreeList, chunk)
		return
	}
	// load
	if len(state.ChunkFreeList) > 0 {
		chunk := state.ChunkFreeList[len(state.ChunkFreeList)-1]
		state.ChunkFreeList = state.ChunkFreeList[:len(state.ChunkFreeList)-1] // pop
		state.Chunks.Set(coord, chunk)
		return
	}
	// allocate
	chunk := mem.Alloc[Chunk](mem.System)
	chunk.data = mem.Alloc[mc.DecompressedChunkData](mem.System)
	chunk.mesh = gfx.NewMesh(mem.System)
	state.Chunks.Set(coord, chunk)
}
func (state *ScreenInGameState) OnChunk(data mc.Decoder) error {
	pkt := data.(*mc.ClientboundChunk)
	// block space to chunk space
	chunkX := pkt.X >> 4
	chunkZ := pkt.Z >> 4
	coord := ChunkCoordinate{chunkX, chunkZ}
	var chunk *Chunk = state.Chunks.Get(coord)
	if chunk == nil {
		sdl.Log("WARNING: we have chunk data but chunk is unloaded.")
		return nil
	}
	err := chunk.data.ProcessChunkData(pkt)
	if err == nil {
		chunk.NeedMeshRebuild = true
	}
	return err
}

// register packet handlers here
func (state *ScreenInGameState) dispatchPacketHandler(id mc.PacketID, data mc.Decoder) error {
	switch id {
	case mc.PKT_SetSpawnPosition:
		state.OnSetSpawnPosition(data)
	case mc.PKT_SetTime:
		return state.OnSetTime(data)
	case mc.PKT_SpawnMob:
		state.OnSpawnMob(data)
	case mc.PKT_PlayerPosition:
		pkt := data.(*mc.PacketPlayerPosition)
		state.OnPlayerPosition(float32(pkt.X), float32(pkt.Y), float32(pkt.Z), float32(pkt.CameraY))
	case mc.PKT_PlayerPositionAndRotation:
		pkt := data.(*mc.PacketPlayerPositionAndRotation)
		state.OnPlayerPosition(float32(pkt.X), float32(pkt.Y), float32(pkt.Z), float32(pkt.CameraY))
		state.OnPlayerRotation(pkt.Pitch, pkt.Yaw)
	case mc.PKT_PlayerRotation:
		pkt := data.(*mc.PacketPlayerRotation)
		state.OnPlayerRotation(pkt.Pitch, pkt.Yaw)
	case mc.PKT_SetChunkVisibility:
		state.OnSetChunkVisibility(data)
	case mc.PKT_EntityPosition:
		pkt := data.(*mc.ClientboundEntityPosition)
		state.OnEntityPosition(pkt.EntityID, *pkt)
	case mc.PKT_EntityPositionAndRotation:
		pkt := data.(*mc.ClientboundEntityPositionAndRotation)
		state.OnEntityPosition(pkt.EntityID, pkt.Pos)
	case mc.PKT_Chunk:
		return state.OnChunk(data)
	case mc.PKT_DespawnEntity:
		state.OnDespawnEntity(data)
	case mc.PKT_SpawnPlayer:
		state.OnSpawnPlayer(data)
	case mc.PKT_TeleportEntity:
		state.OnTeleportEntity(data)
	}
	return nil
}
