package game

import (
	"mbc/cfg"
	"mbc/sdl"
)

func (s *State) LoadConfigFile() (cfg.Config, error) {
	user := sdl.OpenUserStorage(ORG, APP, 0)
	if user == nil {
		panic(sdl.GetError())
	}
	defer user.Close()

	cfgFileMem, err := user.ReadFile(&s.Scratch, CONFIG_FILE_NAME)
	if err != nil {
		err := user.WriteFile(CONFIG_FILE_NAME, cfg.DefaultConfig.Marshal())
		return cfg.DefaultConfig, err
	}
	return cfg.Parse(cfgFileMem)
}
