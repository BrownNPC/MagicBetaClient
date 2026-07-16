package game

import (
	"mbc/gfx"
	"mbc/net/mc"
	"mbc/sdl"

	"solod.dev/so/mem"
	"solod.dev/so/slices"
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
	// Send keep alive every tick
	return mc.PacketKeepAlive{}.Write(&state.s.ServerBound)
}

func (state *ScreenInGameState) OnSpawnMob(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSpawnMob)
	_ = pkt // nothing for now.
}
func (state *ScreenInGameState) OnPlayerRotation(pitch, yaw float32) {
	state.LastPlayerPitch = pitch
	state.LastPlayerYaw = yaw
	state.Cam.Update(gfx.Vector3{}, gfx.NewVector3(yaw, pitch, 0), 0)
}

func (state *ScreenInGameState) OnPlayerPosition(X, Y, Z float32, camY float32) {
	state.LastPlayerPosition = state.Things.Get(state.Player).Position
	state.Things.Get(state.Player).Position = gfx.NewVector3(X, Y, Z)

	oldPos := state.Cam.Position
	newPos := gfx.NewVector3(X, Y+camY, Z)
	diff := gfx.Vector3Subtract(newPos, oldPos)
	state.Cam.Update(diff, gfx.Vector3{}, 0)
}
func (state *ScreenInGameState) OnSetChunkVisibility(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetChunkVisibility)
	coord := [2]int32{pkt.X, pkt.Y}
	if !pkt.Load { // unload
		if chunk := state.Chunks.Get(coord); chunk != nil {
			state.Chunks.Delete(coord)
			state.ChunkFreeList = slices.Append(mem.System, state.ChunkFreeList, chunk)
		}
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
	chunk := mem.Alloc[mc.DecompressedChunkData](mem.System)
	state.Chunks.Set(coord, chunk)
}
func (state *ScreenInGameState) OnChunk(data mc.Decoder) error {
	pkt := data.(*mc.ClientboundChunk)
	chunkX := pkt.X / 16
	chunkZ := pkt.Z / 16
	coord := [2]int32{chunkX, chunkZ}
	var chunk *mc.DecompressedChunkData = state.Chunks.Get(coord)
	if chunk == nil {
		sdl.Log("WARNING: server sent chunk data for unloaded chunk")
		return nil
	}
	return chunk.ProcessChunkData(pkt)
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
		pos := pkt.Pos
		rot := pkt.Rot
		state.OnPlayerPosition(float32(pos.X), float32(pos.Y), float32(pos.Z), float32(pos.CameraY))
		state.OnPlayerRotation(rot.Pitch, rot.Yaw)
	case mc.PKT_PlayerRotation:
		pkt := data.(*mc.PacketPlayerRotation)
		state.OnPlayerRotation(pkt.Pitch, pkt.Yaw)
	case mc.PKT_SetChunkVisibility:
		state.OnSetChunkVisibility(data)
	case mc.PKT_Chunk:
		return state.OnChunk(data)
	}
	return nil
}
