package game

import (
	"mbc/gfx/assets"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/math/rand"
	"solod.dev/so/path"
	"solod.dev/so/time"
)

// should be ran every frame
func (s *State) RollBackgroundMusic() {
	if time.Since(s.TimeSinceLastBackgroundMusicRoll) < RollMusicEvery {
		return
	}
	s.TimeSinceLastBackgroundMusicRoll = time.Now()
	musics := [...]assets.ID{
		assets.Music_calm1, /* /music/calm1.ogg */
		assets.Music_calm2,
		assets.Music_calm3,
	}
	s.Scratch.Reset()
	n := rand.IntN(4)
	if n == 0 {
		return
	}
	f := sdl.IOFromFile(path.Join(&s.Scratch, "assets", musics[n-1].String()), "r")
	if f == nil {
		panic(sdl.GetError()) // should always succeed because wtf
	}
	if !mix.SetTrackIOStream(s.MusicTrack, f, false) {
		panic("game.RollBackgroundMusic: Failed to set music track")
	}
	mix.PlayTrack(s.MusicTrack, 0)
}
