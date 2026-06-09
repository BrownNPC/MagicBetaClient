package cfg

import (
	"mbc/json"
	"mbc/sdl"

	"solod.dev/so/io"
	"solod.dev/so/mem"
)

var __GlobalMemoryForConfigFiles [1024 * 100]byte
var Arena mem.Arena

func FreeFn(a any) { /* no-op */ }

func MallocFn(s json.Size_t) any {
	a, err := Arena.Alloc(int(s), 16)
	if err != nil {
		panic(err)
	}
	return a
}

func init() {
	Arena = mem.NewArena(__GlobalMemoryForConfigFiles[:])
	json.InitHooks(MallocFn, FreeFn)
}

type ServerConfig struct {
	Host string // ip + port combo eg. localhost:25565
	Cmd  string // command to run after joining the server
}

func (c ServerConfig) Encode() *json.JSON {
	return json.CreateObject().
		AddString("host", c.Host).
		AddString("cmd", c.Cmd)
}

func (c *ServerConfig) Decode(j *json.JSON) {
	c.Host = j.Item("host").String()
	c.Cmd = j.Item("cmd").String()
}

/*
	{
		"servers": [
			{"host":"", cmd":""}
		]
	}
*/
type Config struct {
	Servers []ServerConfig
}

func Parse(r io.Reader) (Config, error) {
	b, err := io.ReadAll(&Arena, r)
	if err != nil {
		return Config{}, err
	}
	j, err := json.Parse(b)
	if err != nil {
		return Config{}, err
	}
	var c Config
	c.Decode(j)
	return c, nil
}

func Marshal(c Config, w io.Writer) error {
	encoded := c.Encode().Marshal()
	defer Arena.Free(encoded, len(encoded), 16)
	_, err := w.Write([]byte(encoded))
	return err
}

func (c *Config) Encode() *json.JSON {
	parent := json.CreateObject()

	servers := parent.AddArray("servers")
	for _, s := range c.Servers {
		servers.AddItem(s.Encode())
	}

	return parent
}

func (c *Config) Decode(j *json.JSON) {
	servers := j.Item("servers")
	length := servers.Len()

	c.Servers = mem.AllocSlice[ServerConfig](&Arena, length, length)

	for i := range length {
		c.Servers[i].
			Decode(servers.Index(i))
	}
}

var DefaultConfig = Config{
	Servers: []ServerConfig{
		{Host: "localhost:25565", Cmd: ""},
	},
}

// This will invalidate previously loaded config files.
func Load(path string) (Config, error) {
	Arena.Reset()
	f := sdl.IOFromFile(path, "rb")
	if f == nil {
		err := Save(path, DefaultConfig)
		return DefaultConfig, err
	}
	defer f.Close()
	return Parse(f)
}
func Save(path string, c Config) error {
	f := sdl.IOFromFile(path, "w")
	if f == nil {
		return sdl.GetError()
	}
	return Marshal(DefaultConfig, f)
}
