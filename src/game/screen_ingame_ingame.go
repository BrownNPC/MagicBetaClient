package game

import (
	"mbc/gfx"

	"solod.dev/so/mem"
	"solod.dev/so/time"
)

func (state *ScreenInGameState) Init(s *State) {
	state.Cam = gfx.Camera{
		Position: gfx.Vector3{Y: 2},
		Target:   gfx.Vector3{Y: 2, Z: -1},
		Up:       gfx.Vector3{Y: 1},
		Fovy:     70,
	}
	state.s = s
	state.CurrentScreen = SCREEN_INGAME
	state.PacketDecodeArena = mem.NewArena(state.__PacketDecodeArenaMemory[:])
	state.PersistentArena = mem.NewArena(state.__PersistentMemory[:])
	state.Player = state.Things.New(KindPlayer)
	state.Stars = state.GenMeshStars(mem.System)
	// state.SkyPlanes = state.GenMeshSkyPlanes(mem.System)
}
func (state *ScreenInGameState) ScreenInGame(s *State) {
	if s.Inputs[InputClose].Released {
		state.CurrentScreen = SCREEN_INGAME_PAUSED_SCREEN
	}
	// read packets.
	_, err := state.DecodeRecievedPackets(s)
	if err != nil {
		state.CurrentScreen = SCREEN_INGAME_DISCONNECTED_SCREEN
		state.Error = err
		s.Conn.Close()
		return
	}
	// send buffered packets
	if err := s.ServerBound.Flush(); err != nil {
		state.CurrentScreen = SCREEN_INGAME_DISCONNECTED_SCREEN
		state.Error = err
		s.Conn.Close()
		return
	}
	// lerp ticks
	state.GameTimeFloat = state.LerpTicks()
	// Process mouse look
	if s.Inputs[InputLook].Updated {
		state.ProcessLook(s.Inputs[InputLook].Direction)
	}

	// RENDER
	gfx.BeginMode3D(state.Cam)
	state.DrawSky3D()
	gfx.EndMode3D()
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
