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

// register packet handlers here
func (state *ScreenInGameState) dispatchPacketHandler(id mc.PacketID, data mc.Decoder) {
	switch id {
	case mc.PKT_SetSpawnPosition:
		state.OnSetSpawnPosition(data)
	case mc.PKT_SetTime:
		state.OnSetTime(data)
	case mc.PKT_SpawnMob:
		state.OnSpawnMob(data)
	default:
		return
	}
}
