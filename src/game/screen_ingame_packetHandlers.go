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
	state.Time = pkt.Time
}
