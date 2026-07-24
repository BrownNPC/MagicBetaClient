package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/net/mc"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
	"solod.dev/so/time"
)

func (state *ScreenInGameState) Init(s *State) {
	state.Cam = gfx.NewCamera3D(gfx.Vector3{}, gfx.Vector3{Z: -1}, 90)
	state.s = s
	state.CurrentScreen = SCREEN_INGAME
	state.PacketDecodeArena = mem.NewArena(state.__PacketDecodeArenaMemory[:])
	state.PersistentArena = mem.NewArena(state.__PersistentMemory[:])
	state.Player = state.Things.New(KindPlayer)
	state.Stars = state.GenMeshStars(mem.System)
	state.SunMesh = gfx.GenMeshPlane(mem.System, 32, 32, 1, 1)
	state.Chunks = maps.New[ChunkCoordinate, *Chunk](mem.System, 1000)
	state.GrassChunk = mem.Alloc[Chunk](mem.System)
	state.GrassChunk.data = mem.Alloc[mc.DecompressedChunkData](mem.System)
	state.GrassChunk.mesh = mem.Alloc[gfx.Mesh](mem.System)
	for z := range 16 {
		for x := range 16 {
			for y := range 128 {
				idx := mc.ChunkIndex(x, y, z)
				state.GrassChunk.data.Blocks[idx] = mc.BLOCK_Grass
			}
		}
	}
	state.BuildChunkMesh(state.GrassChunk)
}
func (state *ScreenInGameState) ScreenInGame(s *State) {
	if s.InputPressed(InputClose) {
		state.CurrentScreen = SCREEN_INGAME_PAUSED_SCREEN
	}
	// read packets.
	_, err := state.DecodeRecievedPackets(s)
	if err != nil {
		state.HandleError(err)
		return
	}
	// send buffered packets
	if err := s.ServerBound.Flush(); err != nil {
		state.HandleError(err)
		return
	}
	plr := state.Things.Get(state.Player)
	// state.UpdatePlayerMovement(plr)
	state.UpdateCamera(plr.Position)
	if time.Since(state.LastPositionUpdate) > time.Second/20 {
		state.LastPositionUpdate = time.Now()
		pkt := mc.PacketPlayerPosition{
			X:       float64(plr.Position.X),
			Y:       float64(plr.Position.Y),
			CameraY: float64(plr.Position.Y) + 1.6,
			Z:       float64(plr.Position.Z),
			// Yaw:      state.Cam.GetYaw() * 360,
			// Pitch:    state.Cam.GetPitch() * 360,
			OnGround: false,
		}
		err := pkt.Write(&s.ServerBound)
		if err != nil {
			state.HandleError(err)
			return
		}
	}
	// lerp ticks
	state.GameTimeFloat = state.LerpTicks()
	// RENDER SKY
	gfx.BeginMode3D(state.Cam)
	state.DrawSky3D(state.Cam)
	chunk := state.GrassChunk
	chunk.mesh.Draw(state.s.Pack.GetTexture(assets.Terrain), gfx.White, gfx.MatrixTranslate(
		float32(chunk.coord.X*16), 0, float32(chunk.coord.Z*16),
	))
	// draw chunk boundaries
	for i := range 2 {
		y := float32(i) * 128
		gfx.DrawCube(gfx.NewVector3(0, y, 0), 1, 1, 1, gfx.Green)
		gfx.DrawCube(gfx.NewVector3(16, y, 0), 1, 1, 1, gfx.Green)
		gfx.DrawCube(gfx.NewVector3(0, y, 16), 1, 1, 1, gfx.Green)
		gfx.DrawCube(gfx.NewVector3(16, y, 16), 1, 1, 1, gfx.Green)
	}
	// {
	// 	it := state.Chunks.Iter()
	// 	for it.Next() {
	// 		chunk := it.Value()
	// 		// TODO: in the future check if chunk is in render distance.
	// 		if chunk.NeedMeshRebuild {
	// 			chunk.NeedMeshRebuild = false
	// 			chunk.mesh.Reset()
	// 			state.BuildChunkMesh(chunk)
	// 		}
	// 		chunk.mesh.Draw(state.s.Pack.GetTexture(assets.Terrain), gfx.White, gfx.MatrixTranslate(
	// 			float32(chunk.coord.X*16), 0, float32(chunk.coord.Z*16),
	// 		))
	// 	}
	// }
	{
		it := state.Things.Iter()
		for it.Next() {
			e := it.Thing()
			switch e.Kind {
			case KindEntity:
				if e.Username != "" { // if it has a username then it's a player entity.
					gfx.DrawCube(e.Position, 1, 2, 1, gfx.Yellow)
				} else {
					gfx.DrawCube(e.Position, 5, 10, 5, gfx.Blue)
				}
			}
		}
	}

	gfx.EndMode3D()
}
func (state *ScreenInGameState) HandleError(err error) {
	state.CurrentScreen = SCREEN_INGAME_DISCONNECTED_SCREEN
	state.Error = err
	state.s.Conn.Close()
}

// delta is mouse delta movement.
// UpdateCamera updates rotation based on mouse delta and movement based on WASD.
func (state *ScreenInGameState) UpdateCamera(pos gfx.Vector3) {
	yaw := state.Cam.Yaw
	pitch := state.Cam.Pitch
	delta := state.s.Inputs[InputLook].Direction

	const sensitivity = 0.001
	yaw += -delta.X * sensitivity
	pitch += -delta.Y * sensitivity

	var moveLocal gfx.Vector3
	if state.s.InputHeld(InputMoveForward) {
		moveLocal.Z += 1
	}
	if state.s.InputHeld(InputMoveBackward) {
		moveLocal.Z -= 1
	}
	if state.s.InputHeld(InputMoveLeft) {
		moveLocal.X += 1
	}
	if state.s.InputHeld(InputMoveRight) {
		moveLocal.X -= 1
	}

	const speed = 0.5
	camPos := state.Cam.Position

	if state.s.InputHeld(InputJump) {
		camPos.Y += speed
	} else if state.s.InputHeld(InputSneak) {
		camPos.Y -= speed
	}

	if moveLocal.X != 0 || moveLocal.Z != 0 {
		sinYaw, cosYaw := gfx.Sincos(yaw)

		camPos.X += (sinYaw * moveLocal.Z) * speed
		camPos.Z += (cosYaw * moveLocal.Z) * speed

		camPos.X += (cosYaw * moveLocal.X) * speed
		camPos.Z -= (sinYaw * moveLocal.X) * speed
	}
	state.Cam.Update(camPos, yaw, pitch)
}
func (state *ScreenInGameState) UpdatePlayerMovement(plr *Thing) {
	var moveNorm gfx.Vector3
	if state.s.InputHeld(InputMoveForward) {
		moveNorm.Z = 1
	} else if state.s.InputHeld(InputMoveBackward) {
		moveNorm.Z = -1
	} else if state.s.InputHeld(InputMoveLeft) {
		moveNorm.X = -1
	} else if state.s.InputHeld(InputMoveRight) {
		moveNorm.X = 1
	}
	plr.Position.X += moveNorm.X
	plr.Position.Z += moveNorm.Z
}
func (state *ScreenInGameState) DecodeRecievedPackets(s *State) (bool, error) {
	// drain packets from buffer
	for {
		ok, err := state.TickPacketDecoder(s)
		if !ok {
			return ok, err
		}
	}
}

// Called in ScreenDisconnect
func (s *ScreenInGameState) Unload() {
	s.Stars.Destroy()
}
func (state *ScreenInGameState) LerpTicks() float32 {
	elapsed := time.Since(state.LastTimeUpdate).Seconds() * 20
	return float32(state.GameTicksInt) + float32(elapsed)
}
