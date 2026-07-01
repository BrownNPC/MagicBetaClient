package game

import (
	"mbc/cfg"
	"mbc/gfx"
	"mbc/gfx/assets"
	"mbc/gui"
	"mbc/mix"
	"mbc/sdl"

	"solod.dev/so/maps"
	"solod.dev/so/mem"
)

func (s *State) Init() {
	// init scratch arena
	s.Scratch = mem.NewArena(s.___scratchBuf[:])
	s.TargetFPS = 60
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

	s.Audios = maps.New[assets.ID, *mix.Audio](mem.System, MaxAudioLoaded)

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
	s.Config, err = cfg.LoadConfigFile(ORG, APP, CONFIG_FILE_PATH)
	if err != nil {
		panic(err)
	}
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
		s.Screen_MenuMain(&s.ScreenMainMenuState, screen)
	case SCREEN_MENU_SELECT_SERVER:
		s.Screen_SelectServer(&s.ScreenSelectServerState, screen)
	case SCREEN_JOIN_SERVER:
		s.Screen_JoinServer(&s.ScreenJoinServerState, screen)
	case SCREEN_MENU_TEXTURE_PACKS:
		s.CurrentScreeen = SCREEN_MENU_MAIN
	case SCREEN_MENU_OPTIONS:
		s.CurrentScreeen = SCREEN_MENU_MAIN
	case SCREEN_CONNECT_SERVER:
		s.Screen_ConnectServer(&s.ScreenConnectServerState, screen)
	case SCREEN_INGAME:
		s.Screen_InGame(&s.ScreenInGameState, screen)
	}
	gfx.EndDrawing()

	return true
}
