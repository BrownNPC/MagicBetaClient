package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/fmt"
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
		assets.Newmusic_hal1,
		assets.Newmusic_hal2,
		assets.Newmusic_hal3,
		assets.Newmusic_hal4,
		assets.Newmusic_hal4,
		assets.Newmusic_nuance1,
		assets.Newmusic_nuance2,
		assets.Newmusic_piano1,
		assets.Newmusic_piano2,
		assets.Newmusic_piano3,
	}
	n := rand.IntN(len(musics) * 2)
	if n > len(musics)-1 {
		return
	}
	s.Scratch.Reset()
	f := sdl.IOFromFile(path.Join(&s.Scratch, gfx.AssetsPath, musics[n].String()), "rb")
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
		// delete one audio.
		i := s.Audios.Iter()
		id := i.Key()
		mix.DestroyAudio(i.Value())
		s.Audios.Delete(id)
	}
	s.Scratch.Reset()
	file := mix.LoadAudio(s.Mixer, path.Join(&s.Scratch, gfx.AssetsPath, audio.String()), true)
	s.Audios.Set(audio, file)
	if file == nil {
		panic(sdl.GetError())
	}
	return file
}
func (s *State) PlaySoundEffect(audio assets.ID) *mix.Track {
	for _, t := range s.TracksPool {
		if !mix.TrackPlaying(t) {
			mix.SetTrackAudio(t, s.getAudio(audio))
			mix.PlayTrack(t, 0)
			return t
		}
	}
	c.Assert(false, fmt.Sprintf(fmt.NewBuffer(2048),
		"PlaySoundEffect: out of Tracks in the pool. trying to play %s", audio.String()))
	return nil
}
