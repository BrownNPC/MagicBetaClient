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
	// init scratch arena
	s.Scratch = mem.NewArena(s.___scratchBuf[:])
	// init title storage, opened once.
	s.Storage = sdl.OpenTitleStorage("", 0)
	if s.Storage == nil {
		panic(sdl.GetError())
	}
	for !s.Storage.Ready() {
		// hang while not ready
	}
	// init default texture pack
	s.Pack = NewDefaultTexturePack()
	// pack.png should apply bilinear interpolation (TODO: implement a better way to do this)
	gfx.SetTextureConfig(s.Pack.GetTexture(assets.Pack), true, false)

	// create mixer device
	s.Mixer = mix.CreateMixerDevice(sdl.AUDIO_DEVICE_DEFAULT_PLAYBACK, nil)
	if s.Mixer == nil {
		panic(sdl.GetError())
	}
	// initialize audio tracks
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

	// load splash text for main menu screen
	s.SplashText = s.LoadRandomSplashText()

	// Load config.json file.

	var err error
	s.Config, err = s.LoadConfigFile()
	if err != nil {
		panic(err)
	}
	println("servers:", len(s.Config.Servers))
	for _, srv := range s.Config.Servers {
		println("server:", srv.Host)
	}

	s.ScreenJoinServer.Arena = mem.NewArena(s.ScreenJoinServer.Buf[:])
	s.ScreenJoinServer.TextField = strings.NewBuilder(&s.ScreenJoinServer.Arena)
}

// return false to quit.
func (s *State) Update() bool {
	gui.Update(s.ScreenWidth, s.ScreenHeight, s.Pack)
	screen := gfx.Rectangle{W: float32(s.ScreenWidth), H: float32(s.ScreenHeight)}
	s.RollBackgroundMusic()
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
