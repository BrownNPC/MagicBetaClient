package game

import (
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/mem"
	"solod.dev/so/strings"
)

func (s *State) Init() {
	s.Pack = NewDefaultTexturePack()
	gfx.SetTextureConfig(s.Pack.GetTexture(assets.Pack), true, false)
	s.Scratch = mem.NewArena(___scratchBuf[:])
	s.SplashText = s.LoadRandomSplashText()
	// create mixer device
	s.Mixer = mix.CreateMixerDevice(sdl.AUDIO_DEVICE_DEFAULT_PLAYBACK, nil)
	if s.Mixer == nil {
		panic(sdl.GetError())
	}
	s.MusicTrack = mix.CreateTrack(s.Mixer)
	if s.MusicTrack == nil {
		panic(sdl.GetError())
	}
	for i := range len(s.TracksPool) {
		track := mix.CreateTrack(s.Mixer)
		if track == nil {
			panic(sdl.GetError())
		}
		s.TracksPool[i] = track
	}
	s.UISoundTrack = mix.CreateTrack(s.Mixer)
	if s.UISoundTrack == nil {
		panic(sdl.GetError())
	}
	s.ScreenJoinServer.Arena = mem.NewArena(s.ScreenJoinServer.Buf[:])
	s.ScreenJoinServer.TextField = strings.NewBuilder(&s.ScreenJoinServer.Arena)

}

// return false to quit.
func (s *State) Update() bool {
	gui.Update(s.ScreenWidth, s.ScreenHeight, s.Pack)
	screen := gfx.Rectangle{W: float32(s.ScreenWidth), H: float32(s.ScreenHeight)}
	// s.RollBackgroundMusic()
	gfx.BeginDrawing()
	gfx.ClearBackground(gfx.Black)

	switch s.CurrentScreeen {
	case SCREEN_MENU_MAIN:
		s.Screen_MenuMain(screen)
	case SCREEN_MENU_JOIN_SERVER:
		s.Screen_JoinServer(screen, &s.ScreenJoinServer)
	}
	gfx.EndDrawing()

	return true
}
