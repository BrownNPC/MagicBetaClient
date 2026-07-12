package game

import (
	"mbc/gfx"
	"mbc/net/mc"
)

func (state *ScreenInGameState) OnSetSpawnPosition(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetSpawnPosition)
	state.SpawnPosition = gfx.NewVector3(float32(pkt.X), float32(pkt.Y), float32(pkt.Z))
}
func (state *ScreenInGameState) OnSetTime(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSetTime)
	state.InGameTime = pkt.Time
}
func (state *ScreenInGameState) OnSpawnMob(data mc.Decoder) {
	pkt := data.(*mc.ClientboundSpawnMob)
	_ = pkt // nothing for now.
}
func (state *ScreenInGameState) OnPlayerRotation(pitch, yaw float32) {
	state.LastPlayerPitch = pitch
	state.LastPlayerYaw = yaw
	lockView := true
	rotateAroundTarget := false
	rotateUp := false
	// Reset camera target and up vector to baseline before applying absolute rotation
	state.Cam.Target = gfx.Vector3Add(state.Cam.Position, gfx.NewVector3(0, 0, -1))
	state.Cam.Up = gfx.NewVector3(0, 1, 0)
	state.Cam.Pitch(pitch*gfx.Deg2rad, lockView, rotateAroundTarget, rotateUp)
	state.Cam.Yaw(yaw*gfx.Deg2rad, rotateAroundTarget)
}

func (state *ScreenInGameState) OnPlayerPosition(X, Y, Z float32, camY float32) {
	state.LastPlayerPosition = gfx.NewVector3(X, Y, Z)
	state.Things.Get(state.Player).Position = state.LastPlayerPosition

	oldPos := state.Cam.Position
	newPos := gfx.NewVector3(X, Y+camY, Z)
	diff := gfx.Vector3Subtract(newPos, oldPos)
	state.Cam.Position = newPos
	state.Cam.Target = gfx.Vector3Add(state.Cam.Target, diff)
}

// register packet handlers here
func (state *ScreenInGameState) dispatchPacketHandler(id mc.PacketID, data mc.Decoder) {
	switch id {
	case mc.PKT_SetSpawnPosition:
		state.OnSetSpawnPosition(data)
	case mc.PKT_SetTime:
		state.OnSetTime(data)
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
	default:
		return
	}
}
