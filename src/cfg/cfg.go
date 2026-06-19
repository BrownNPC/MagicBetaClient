package cfg

import (
	"mbc/json"

	"solod.dev/so/mem"
)

var __GlobalMemoryForConfigFiles [1024 * 100]byte
var Arena mem.Arena

func freeFn(a any) { /* no-op */ }

func mallocFn(s json.Size_t) any {
	a, err := Arena.Alloc(int(s), 8)
	if err != nil {
		panic(err)
	}
	return a
}

func init() {
	Arena = mem.NewArena(__GlobalMemoryForConfigFiles[:])
	json.InitHooks(mallocFn, freeFn)
}

/*
	{
		"servers": [
			{"host":"", cmd":""}
		]
	}
*/
// Configuration file struct
type Config struct {
	Servers []ServerCfg
}

type ServerCfg struct {
	Host string // ip + port combo eg. localhost:25565
	Cmd  string // command to run after joining the server
}

func (c ServerCfg) Encode() *json.JSON {
	o := json.CreateObject()

	o.AddString("host", c.Host)
	o.AddString("cmd", c.Cmd)
	return o
}

func (c *ServerCfg) Decode(j *json.JSON) {
	c.Host = j.Item("host").String()
	c.Cmd = j.Item("cmd").String()
}
func Parse(b []byte) (Config, error) {
	Arena.Reset()
	j, err := json.Parse(b)
	if err != nil {
		return Config{}, err
	}
	var c Config
	c.Decode(j)
	return c, nil
}

func (c Config) Marshal() []byte {
	return c.Encode().Marshal()
}

func (c *Config) Encode() *json.JSON {
	parent := json.CreateObject()

	servers := parent.AddArray("servers")
	for _, s := range c.Servers {
		if !servers.AddItem(s.Encode()) {
			panic(json.GetError())
		}
	}

	return parent
}

func (c *Config) Decode(j *json.JSON) {
	servers := j.Item("servers")
	length := servers.Len()

	c.Servers = mem.AllocSlice[ServerCfg](&Arena, length, length)

	for i := range length {
		c.Servers[i].
			Decode(servers.Index(i))
	}
}

var DefaultConfig = Config{
	Servers: []ServerCfg{
		{Host: "localhost:25565", Cmd: ""},
	},
}
