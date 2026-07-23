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
	state.Cam = gfx.Camera{
		Position: gfx.Vector3{Y: 2},
		Target:   gfx.Vector3{Y: 2, Z: -1},
		Up:       gfx.Vector3{Y: 1},
		Fovy:     90,
	}
	state.s = s
	state.CurrentScreen = SCREEN_INGAME
	state.PacketDecodeArena = mem.NewArena(state.__PacketDecodeArenaMemory[:])
	state.PersistentArena = mem.NewArena(state.__PersistentMemory[:])
	state.Player = state.Things.New(KindPlayer)
	state.Stars = state.GenMeshStars(mem.System)
	state.SunMesh = gfx.GenMeshPlane(mem.System, 32, 32, 1, 1)
	state.Chunks = maps.New[ChunkCoordinate, *Chunk](mem.System, 1000)
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
	state.Cam.Update(state.Cam.Position.Subtract(plr.Position).Add(gfx.Vector3{Y: 1.6}), gfx.Vector3{}, 0)
	//
	// send ack maybe?
	// if state.firstChunkdataRecieved == 1 {
	// 	plr.OnGround = true
	// 	ack := mc.PacketPlayerMovment{OnGround: true}
	// 	ack.Write(&s.ServerBound)
	// }
	// if state.acked && state.firstChunkdataRecieved == 0 && time.Since(state.LastMovementUpdateSent) > time.Second/5 {
	// 	state.LastMovementUpdateSent = time.Now()
	// 	pkt := mc.PacketPlayerPosition{
	// 		X:       float64(plr.Position.X),
	// 		Y:       float64(plr.Position.Y),
	// 		CameraY: float64(plr.Position.Y + 1.62),
	// 		Z:       float64(plr.Position.Z),
	// 		// Yaw:      state.Cam.GetYaw() * 360,
	// 		// Pitch:    state.Cam.GetPitch() * 360,
	// 		OnGround: plr.OnGround,
	// 	}
	// 	if err := pkt.Write(&state.s.ServerBound); err != nil {
	// 		state.HandleError(err)
	// 		return
	// 	}
	// }
	if time.Since(state.LastPositionUpdate) > time.Second/20 {
		state.LastPositionUpdate = time.Now()
		pkt := mc.PacketPlayerPosition{
			X:       float64(plr.Position.X),
			Y:       float64(plr.Position.Y),
			CameraY: float64(state.Cam.Position.Y),
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
	// Process mouse look
	if s.InputPressed(InputLook) {
		state.ProcessLook(s.Inputs[InputLook].Direction)
	}
	state.ProcessMovementInput()

	// RENDER SKY
	gfx.BeginMode3D(state.Cam)
	state.DrawSky3D(state.Cam)
	{
		it := state.Chunks.Iter()
		for it.Next() {
			chunk := it.Value()
			// TODO: in the future check if chunk is in render distance.
			if chunk.NeedMeshRebuild {
				chunk.NeedMeshRebuild = false
				chunk.mesh.Reset()
				state.BuildChunkMesh(chunk)
			}
			chunk.mesh.Draw(state.s.Pack.GetTexture(assets.Terrain), gfx.White, gfx.MatrixTranslate(
				float32(chunk.coord.X*16), 0, float32(chunk.coord.Z*16),
			))
		}
	}
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
func (state *ScreenInGameState) ProcessLook(delta gfx.Vector2) {
	const sensitivity = 0.001
	state.Cam.Yaw(-delta.X*sensitivity, false)
	state.Cam.Pitch(-delta.Y*sensitivity, true, false, false)
}
func (state *ScreenInGameState) ProcessMovementInput() {
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
	plr := state.Things.Get(state.Player)
	plr.Position.X += moveNorm.X
	plr.Position.Z += moveNorm.Y
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
