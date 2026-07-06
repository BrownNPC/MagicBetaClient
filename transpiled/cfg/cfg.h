#pragma once
#include "so/builtin/builtin.h"
#include "json/json.h"
#include "sdl/sdl.h"
#include "so/mem/mem.h"
#include "so/slices/slices.h"
#include "so/strings/strings.h"

// -- Types --

typedef struct cfg_ServerCfg cfg_ServerCfg;
typedef struct cfg_Config cfg_Config;

typedef struct cfg_ServerCfg {
    so_String Host;
    so_String Cmd;
} cfg_ServerCfg;

// Configuration file struct
typedef struct cfg_Config {
    cfg_ServerCfg Servers[25];
} cfg_Config;

// -- Result types --

typedef struct cfg_ConfigResult {
    cfg_Config val;
    so_Error err;
} cfg_ConfigResult;

// -- Variables and constants --

// this is what holds all the memory for this package
extern mem_Arena cfg_Arena;
static const int64_t cfg_MAX_SERVERS = 25;

/*
	{
		"servers": [
			{"host":"", cmd":""}
		]
	}
*/
extern cfg_Config cfg_DefaultConfig;

// -- Functions and methods --

// creates clone using Arena
cfg_ServerCfg cfg_ServerCfg_Clone(cfg_ServerCfg s);

// Allocates copy of Config on the Arena
cfg_Config cfg_Config_Clone(cfg_Config c);
cfg_ConfigResult cfg_Parse(so_Slice b);
so_Slice cfg_Config_Marshal(cfg_Config c);

// Loads a config file from SDL user storage.
cfg_ConfigResult cfg_LoadConfigFile(so_String ORG, so_String APP, so_String filePath);

// Save a config file to SDL user storage
so_Error cfg_SaveConfigFile(so_String ORG, so_String APP, so_String filePath, cfg_Config c);
