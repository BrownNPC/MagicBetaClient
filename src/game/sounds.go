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
	n -= 1
	f := sdl.IOFromFile(path.Join(&s.Scratch, "assets", musics[n].String()), "r")
	if f == nil {
		panic(sdl.GetError()) // should always succeed because wtf
	}
	if !mix.SetTrackIOStream(s.MusicTrack, f, true) {
		panic("game.RollBackgroundMusic: Failed to set music track")
	}
	mix.PlayTrack(s.MusicTrack, 0)
}
func (s *State) getAudio(audio assets.ID) *mix.Audio {
	if s.Audios.Has(audio) {
		return s.Audios.Get(audio)
	}
	if s.Audios.Len() > MaxAudioLoaded {
		i := s.Audios.Iter()
		id := i.Key()
		mix.DestroyAudio(i.Value())
		s.Audios.Delete(id)
	}
	s.Scratch.Reset()
	file := mix.LoadAudio(s.Mixer, path.Join(&s.Scratch, "assets", audio.String()), false)
	if file == nil {
		panic(sdl.GetError())
	}
	return file
}
func (s *State) PlaySoundEffect(audio assets.ID) {
	mix.SetTrackAudio(s.UISoundTrack, s.getAudio(audio))
	mix.PlayTrack(s.UISoundTrack, 0)
}
