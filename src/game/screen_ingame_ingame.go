package game

import (
	"mbc/gfx"
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
	if s.Inputs[InputClose].Released {
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
	if time.Since(state.LastMovementUpdateSent) > time.Second/20 {
		state.LastMovementUpdateSent = time.Now()
		plr := state.Things.Get(state.Player)
		pkt := mc.PacketPlayerPositionAndRotation{
			Pos: mc.PacketPlayerPosition{
				X:        float64(plr.Position.X),
				Y:        float64(plr.Position.Y),
				CameraY:  float64(state.Cam.Position.Y),
				Z:        float64(plr.Position.Z),
				OnGround: true,
			},
			Rot: mc.PacketPlayerRotation{
				Yaw:      state.Cam.GetYaw() * 360,
				Pitch:    state.Cam.GetPitch() * 360,
				OnGround: true,
			},
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
	if s.Inputs[InputLook].Updated {
		state.ProcessLook(s.Inputs[InputLook].Direction)
	}

	// RENDER SKY
	plr := state.Things.Get(state.Player)
	state.Cam.Position = gfx.NewVector3(plr.Position.X, plr.CameraY, plr.Position.Z)
	gfx.BeginMode3D(state.Cam)
	state.DrawSky3D(state.Cam)
	// draw cube (floor)
	gfx.DrawCube(gfx.NewVector3(0, 0, 0), 2, 2, 2, gfx.Red)
	{
		it := state.Chunks.Iter()
		for it.Next() {
			chunk := it.Value()
			// TODO: in the future check if chunk is in render distance.
			if chunk.NeedMeshRebuild {
				println("needs rebuild")
				chunk.NeedMeshRebuild = false
				chunk.mesh.Reset()
				state.BuildChunkMesh(chunk)
				state.Chunks.Set(it.Key(), chunk)
			}
			chunk.mesh.Draw(gfx.DefaultTexture(), gfx.Green, gfx.MatrixTranslate(
				float32(chunk.data.X)*16, -127, float32(chunk.data.Z)*16,
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
