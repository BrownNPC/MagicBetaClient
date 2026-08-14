package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/net/mc"

	"solod.dev/so/fmt"
	"solod.dev/so/maps"
	"solod.dev/so/math"
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
	state.SetRenderDistance(5)
	state.SystemTracker = mem.Tracker{
		Allocator: mem.System,
	}
	state.SetDecompressedChunkLimit(&state.SystemTracker, 25)
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
			X:        float64(plr.Position.X),
			Y:        float64(plr.Position.Y),
			CameraY:  float64(plr.Position.Y) + 1.6,
			Z:        float64(plr.Position.Z),
			OnGround: false,
		}
		err := pkt.Write(&s.ServerBound)
		if err != nil {
			state.HandleError(err)
			return
		}
	}

	gfx.BeginMode3D(state.Cam)
	// RENDER SKY
	state.DrawSky3D(state.Cam)

	// lerp ticks
	state.GameTimeFloat = state.LerpTicks()
	terrain := state.s.Pack.GetTexture(assets.Terrain)
	_ = terrain
	{
		it := state.Chunks.Iter()
		for it.Next() {
			chunk := it.Value()
			for i := range 8 {
				pos := chunk.GetSectionCenter(i)
				visible := state.Cam.IsSphereInFrustum(pos, CHUNK_SECTION_SPHERE_RADIUS)
				if !visible {
					continue
				}
				if math.Abs(float64(state.Cam.Position.Distance(pos))) > 7*7*3 {
					continue
				}
				if chunk.NeedsRebuild {
					// state.RequestChunkData(chunk)
					// chunk.ResetMeshes()
					// state.BuildChunkMesh(chunk)
					// chunk.NeedsRebuild = false
					// state.SaveChunkData(chunk)
				}

				// chunk.DrawSectionMesh(i, terrain)
			}
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
					gfx.DrawCube(e.Position, .5, 1, .5, gfx.Blue)
				}
			}
		}
	}
	gfx.EndMode3D()

	{
		fnt := state.s.Pack.Font()
		stats := state.SystemTracker.Stats()
		text := fmt.Sprintf(fmt.NewBuffer(1024), "Alloc=%.1f KiB (%.1f MiB)\nObjects=%d", float32(stats.Alloc)/1024, float32(stats.Alloc)/1024/1024, stats.Mallocs-stats.Frees)
		fnt.DrawRunes([]rune(text), gfx.NewVector2(15, 15), gui.Scale, 0, gfx.Green, false)
	}
}
func (state *ScreenInGameState) HandleError(err error) {
	state.CurrentScreen = SCREEN_INGAME_DISCONNECTED_SCREEN
	state.Error = err
	state.s.Conn.Close()
}
func ScreenInGameState_SortChunksFarToNear(a, b any) int {
	c1 := a.(*Chunk)
	c2 := b.(*Chunk)

	camPos := GlobalState.ScreenInGameState.Cam.Position

	d1 := c1.GetCenter().Subtract(camPos)
	d2 := c2.GetCenter().Subtract(camPos)

	dist1 := d1.DotProduct(d1)
	dist2 := d2.DotProduct(d2)

	switch {
	case dist1 > dist2:
		return -1 // c1 is farther, so comes first
	case dist1 < dist2:
		return 1 // c2 is farther, so c1 comes after
	default:
		return 0
	}
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
	if state.s.IsMovingWithGamepad {
		moveLocal.X = state.s.GamepadMovement.X
		moveLocal.Z = state.s.GamepadMovement.Y
	} else {
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
	}
	var speed = 50 * state.s.Dt
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
