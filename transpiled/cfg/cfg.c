#include "cfg.h"

// -- Forward declarations --
static cJSON* cfg_ServerCfg_encode(cfg_ServerCfg c);
static void cfg_ServerCfg_decode(void* self, cJSON* j);
static cJSON* cfg_Config_encode(void* self);
static void decodeConfig(cfg_Config* c, cJSON* j);

// -- Variables and constants --
static so_byte __GlobalMemoryForConfigFiles[102400] = {0};

// this is what holds all the memory for this package
mem_Arena cfg_Arena = {0};

/*
	{
		"servers": [
			{"host":"", cmd":""}
		]
	}
*/
cfg_Config cfg_DefaultConfig = (cfg_Config){.Servers = {[0] = (cfg_ServerCfg){.Host = so_str("localhost:25565"), .Cmd = so_str("")}}};

// -- Implementation --

// creates clone using Arena
cfg_ServerCfg cfg_ServerCfg_Clone(cfg_ServerCfg s) {
    return (cfg_ServerCfg){.Host = strings_Clone((mem_Allocator){.self = &cfg_Arena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, s.Host), .Cmd = strings_Clone((mem_Allocator){.self = &cfg_Arena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, s.Cmd)};
}

static cJSON* cfg_ServerCfg_encode(cfg_ServerCfg c) {
    cJSON* o = cJSON_CreateObject();
    cJSON_AddString(o, so_str("host"), c.Host);
    cJSON_AddString(o, so_str("cmd"), c.Cmd);
    return o;
}

static void cfg_ServerCfg_decode(void* self, cJSON* j) {
    cfg_ServerCfg* c = self;
    c->Host = cJSON_String(cJSON_Item(j, so_str("host")));
    c->Cmd = cJSON_String(cJSON_Item(j, so_str("cmd")));
}

// Allocates copy of Config on the Arena
cfg_Config cfg_Config_Clone(cfg_Config c) {
    cfg_Config c2 = {0};
    for (so_int i = 0; i < 25; i++) {
        cfg_ServerCfg srv = c.Servers[i];
        c2.Servers[i] = cfg_ServerCfg_Clone(srv);
    }
    return c2;
}

cfg_ConfigResult cfg_Parse(so_Slice b) {
    so_R_ptr_err _res1 = json_Parse(b);
    cJSON* j = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (cfg_ConfigResult){.val = (cfg_Config){}, .err = err};
    }
    cfg_Config c = {0};
    decodeConfig(&c, j);
    cfg_ConfigResult _res2 = (cfg_ConfigResult){.val = cfg_Config_Clone(c), .err = (so_Error){0}};
    cJSON_Delete(j);
    return _res2;
}

so_Slice cfg_Config_Marshal(cfg_Config c) {
    cJSON* encoded = cfg_Config_encode(&c);
    so_Slice b = cJSON_Marshal(encoded);
    so_Slice b2 = slices_Clone(so_byte, ((mem_Allocator){.self = &cfg_Arena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}), (b));
    so_Slice _res1 = b2;
    mem_FreeSlice(so_byte, (mem_System), (b));
    cJSON_Delete(encoded);
    return _res1;
}

static cJSON* cfg_Config_encode(void* self) {
    cfg_Config* c = self;
    cJSON* parent = cJSON_CreateObject();
    cJSON* servers = cJSON_AddArray(parent, so_str("servers"));
    for (so_int _ = 0; _ < 25; _++) {
        cfg_ServerCfg s = c->Servers[_];
        if (!cJSON_AddItem(servers, cfg_ServerCfg_encode(s))) {
            so_panic(so_error_cstr(json_GetError()));
        }
    }
    return parent;
}

static void decodeConfig(cfg_Config* c, cJSON* j) {
    cJSON* servers = cJSON_Item(j, so_str("servers"));
    so_int length = so_min(cJSON_Len(servers), cfg_MAX_SERVERS);
    for (so_int i = 0; i < length; i++) {
        cfg_ServerCfg_decode(&c->Servers[i], cJSON_Index(servers, i));
    }
}

// Loads a config file from SDL user storage.
cfg_ConfigResult cfg_LoadConfigFile(so_String ORG, so_String APP, so_String filePath) {
    mem_Arena_Reset(&cfg_Arena);
    SDL_Storage* user = SDL_OpenUserStorage(so_cstr(ORG), so_cstr(APP), 0);
    if (user == NULL) {
        return (cfg_ConfigResult){.val = (cfg_Config){}, .err = sdl_GetError()};
    }
    so_R_slice_err _res1 = SDL_Storage_ReadFile(user, mem_System, filePath);
    so_Slice cfgFileMem = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        so_Error err = SDL_Storage_WriteFile(user, filePath, cfg_Config_Marshal(cfg_DefaultConfig));
        cfg_ConfigResult _res2 = (cfg_ConfigResult){.val = cfg_DefaultConfig, .err = err};
        SDL_Storage_Close(user);
        return _res2;
    }
    cfg_ConfigResult _res3 = cfg_Parse(cfgFileMem);
    mem_FreeSlice(so_byte, (mem_System), (cfgFileMem));
    SDL_Storage_Close(user);
    return _res3;
}

// Save a config file to SDL user storage
so_Error cfg_SaveConfigFile(so_String ORG, so_String APP, so_String filePath, cfg_Config c) {
    SDL_Storage* user = SDL_OpenUserStorage(so_cstr(ORG), so_cstr(APP), 0);
    if (user == NULL) {
        return sdl_GetError();
    }
    so_Error _res1 = SDL_Storage_WriteFile(user, filePath, cfg_Config_Marshal(c));
    SDL_Storage_Close(user);
    return _res1;
}

static void __attribute__((constructor)) cfg_init() {
    cfg_Arena = mem_NewArena(so_array_slice(so_byte, __GlobalMemoryForConfigFiles, 0, 102400, 102400));
}
