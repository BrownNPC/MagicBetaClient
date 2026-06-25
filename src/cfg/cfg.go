package cfg

import (
	"mbc/json"
	"mbc/sdl"

	"solod.dev/so/mem"
	"solod.dev/so/slices"
	"solod.dev/so/strings"
)

var __GlobalMemoryForConfigFiles [1024 * 100]byte

// this is what holds all the memory for this package
var Arena mem.Arena

const MAX_SERVERS = 25

/*
	{
		"servers": [
			{"host":"", cmd":""}
		]
	}
*/

var DefaultConfig = Config{
	Servers: [MAX_SERVERS]ServerCfg{
		0: {Host: "localhost:25565", Cmd: ""},
	},
}

func init() {
	Arena = mem.NewArena(__GlobalMemoryForConfigFiles[:])
}

type ServerCfg struct {
	Host string // ip + port combo eg. localhost:25565
	Cmd  string // command to run after joining the server
}

// creates clone using Arena
func (s ServerCfg) Clone() ServerCfg {
	return ServerCfg{
		Host: strings.Clone(&Arena, s.Host),
		Cmd:  strings.Clone(&Arena, s.Cmd),
	}
}

func (c ServerCfg) encode() *json.JSON {
	o := json.CreateObject()

	o.AddString("host", c.Host)
	o.AddString("cmd", c.Cmd)
	return o
}

func (c *ServerCfg) decode(j *json.JSON) {
	c.Host = j.Item("host").String()
	c.Cmd = j.Item("cmd").String()
}

// Configuration file struct
type Config struct {
	Servers [MAX_SERVERS]ServerCfg
}

// Allocates copy of Config on the Arena
func (c Config) Clone() Config {
	var c2 Config
	for i, srv := range c.Servers {
		c2.Servers[i] = srv.Clone()
	}
	return c2
}

func Parse(b []byte) (Config, error) {
	j, err := json.Parse(b)
	if err != nil {
		return Config{}, err
	}
	defer json.Delete(j)

	var c Config
	decodeConfig(&c, j)

	return c.Clone(), nil
}

func (c Config) Marshal() []byte {
	encoded := c.encode()
	defer json.Delete(encoded)

	b := encoded.Marshal()
	defer mem.FreeSlice(mem.System, b)

	b2 := slices.Clone(&Arena, b)
	return b2
}

func (c *Config) encode() *json.JSON {
	parent := json.CreateObject()

	servers := parent.AddArray("servers")
	for _, s := range c.Servers {
		if !servers.AddItem(s.encode()) {
			panic(json.GetError())
		}
	}

	return parent
}
func decodeConfig(c *Config, j *json.JSON) {

	servers := j.Item("servers")
	length := min(servers.Len(), MAX_SERVERS)

	for i := range length {
		c.Servers[i].
			decode(servers.Index(i))
	}
}

// Loads a config file from SDL user storage.
func LoadConfigFile(ORG, APP string, filePath string) (Config, error) {
	user := sdl.OpenUserStorage(ORG, APP, 0)
	if user == nil {
		return Config{}, sdl.GetError()
	}
	defer user.Close()

	cfgFileMem, err := user.ReadFile(mem.System, filePath)
	if err != nil {
		err := user.WriteFile(filePath, DefaultConfig.Marshal())
		return DefaultConfig, err
	}
	defer mem.FreeSlice(mem.System, cfgFileMem)
	return Parse(cfgFileMem)
}
// Save a config file to SDL user storage
func SaveConfigFile(ORG, APP string, filePath string, c Config) error {
	user := sdl.OpenUserStorage(ORG, APP, 0)
	if user == nil {
		return sdl.GetError()
	}
	defer user.Close()

	return user.WriteFile(filePath, c.Marshal())
}
