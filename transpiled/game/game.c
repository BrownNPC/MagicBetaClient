#include "game.h"

// -- Forward declarations --
static void game_ScreenInGameState_dispatchPacketHandler(void* self, so_byte id, mc_Decoder data);
static MIX_Audio* game_State_getAudio(void* self, assets_ID audio);

// -- Variables and constants --

// memory for storing Maps in DefaultTexturePack
static so_byte _defaultTexturePackScratchBuffer[20480] = {};
so_Error game_NoDecoderForPacketErr = errors_New("No handler implemented for packet");
game_ThingRef game_NilRef = (game_ThingRef){};

// -- default_texture_pack.go --

void game_DefaultTexturePack_Unload(void* self) {
    game_DefaultTexturePack* p = self;
    maps_Iter iter = maps_Map_Iter(assets_ID, gfx_Texture, (&p->Textures));
    for (; maps_Iter_Next(assets_ID, gfx_Texture, (&iter));) {
        gfx_UnloadTexture(maps_Iter_Value(assets_ID, gfx_Texture, (&iter)));
    }
    maps_Map_Clear(assets_ID, gfx_Texture, (&p->Textures));
}

// Destroy implements [TexturePack].
void game_DefaultTexturePack_Destroy(void* self) {
    game_DefaultTexturePack* p = self;
    maps_Iter iter = maps_Map_Iter(assets_ID, gfx_Texture, (&p->Textures));
    for (; maps_Iter_Next(assets_ID, gfx_Texture, (&iter));) {
        gfx_UnloadTexture(maps_Iter_Value(assets_ID, gfx_Texture, (&iter)));
    }
    maps_Map_Free(assets_ID, gfx_Texture, (&p->Textures));
    gfx_Font_Destroy(&p->font);
}

// Description implements [TexturePack].
so_String game_DefaultTexturePack_Description(void* self) {
    game_DefaultTexturePack* p = self;
    return so_str("The default look of Minecraft");
}

// GetTexture implements [TexturePack].
gfx_Texture game_DefaultTexturePack_GetTexture(void* self, assets_ID asset) {
    game_DefaultTexturePack* p = self;
    if (maps_Map_Has(assets_ID, gfx_Texture, (&p->Textures), (asset))) {
        return maps_Map_Get(assets_ID, gfx_Texture, (&p->Textures), (asset));
    }
    mem_Arena_Reset(&p->scratch);
    gfx_TextureResult _res1 = gfx_LoadTexture(path_Join((mem_Allocator){.self = &p->scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (so_Slice){(so_String[2]){gfx_AssetsPath, assets_ID_String(asset)}, 2, 2}));
    gfx_Texture t = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (gfx_Texture){};
    }
    maps_Map_Set(assets_ID, gfx_Texture, (&p->Textures), (asset), (t));
    return t;
}

// Icon implements [TexturePack].
gfx_Texture game_DefaultTexturePack_Icon(void* self) {
    game_DefaultTexturePack* p = self;
    {
        gfx_Texture tex = game_DefaultTexturePack_GetTexture(p, assets_Pack);
        if (tex.ID != 0) {
            return tex;
        }
    }
    so_panic("pack.png not found. assets are missing.");
}

// Font implements [TexturePack].
gfx_Font* game_DefaultTexturePack_Font(void* self) {
    game_DefaultTexturePack* p = self;
    mem_Arena_Reset(&p->scratch);
    if (p->font.Atlas.ID != 0) {
        return &p->font;
    }
    so_Error err = {0};
    gfx_FontResult _res1 = gfx_LoadFont(path_Join((mem_Allocator){.self = &p->scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (so_Slice){(so_String[3]){gfx_AssetsPath, so_str("font"), so_str("default.png")}, 3, 3}));
    p->font = _res1.val;
    err = _res1.err;
    if (err.self != NULL) {
        so_panic(so_error_cstr(err));
    }
    return &p->font;
}

// Name implements [TexturePack].
so_String game_DefaultTexturePack_Name(void* self) {
    game_DefaultTexturePack* p = self;
    return so_str("Default");
}

gfx_TexturePack game_NewDefaultTexturePack(void) {
    mem_Arena parent = mem_NewArena(so_array_slice(so_byte, _defaultTexturePackScratchBuffer, 0, 20480, 20480));
    game_DefaultTexturePack* p = mem_Alloc(game_DefaultTexturePack, ((mem_Allocator){.self = &parent, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}));
    p->scratch = mem_NewArena(mem_AllocSlice(so_byte, ((mem_Allocator){.self = &parent, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}), (512), (512)));
    p->Textures = maps_New(assets_ID, gfx_Texture, ((mem_Allocator){.self = &parent, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}), (100));
    return (gfx_TexturePack){.self = p, .Description = game_DefaultTexturePack_Description, .Destroy = game_DefaultTexturePack_Destroy, .Font = game_DefaultTexturePack_Font, .GetTexture = game_DefaultTexturePack_GetTexture, .Icon = game_DefaultTexturePack_Icon, .Name = game_DefaultTexturePack_Name, .Unload = game_DefaultTexturePack_Unload};
}

// -- game.go --

void game_State_Init(void* self) {
    game_State* s = self;
    // init scratch arena
    s->Scratch = mem_NewArena(so_array_slice(so_byte, s->___scratchBuf, 0, 102400, 102400));
    s->TargetFPS = 60;
    // init title storage, opened once.
    s->Storage = SDL_OpenTitleStorage("", 0);
    if (s->Storage == NULL) {
        so_panic(so_error_cstr(sdl_GetError()));
    }
    for (; !SDL_Storage_Ready(s->Storage);) {
    }
    // hang while not ready
    // init default texture pack
    s->Pack = game_NewDefaultTexturePack();
    // pack.png should apply bilinear interpolation (TODO: implement a better way to do this)
    gfx_SetTextureConfig(s->Pack.GetTexture(s->Pack.self, assets_Pack), true, false);
    s->Audios = maps_New(assets_ID, MIX_Audio*, (mem_System), (game_MaxAudioLoaded));
    // create mixer device
    s->Mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (s->Mixer == NULL) {
        so_panic(so_error_cstr(sdl_GetError()));
    }
    // initialize audio tracks
    s->MusicTrack = MIX_CreateTrack(s->Mixer);
    if (s->MusicTrack == NULL) {
        so_panic(so_error_cstr(sdl_GetError()));
    }
    for (so_int i = 0; i < 10; i++) {
        MIX_Track* track = MIX_CreateTrack(s->Mixer);
        if (track == NULL) {
            so_panic(so_error_cstr(sdl_GetError()));
        }
        s->TracksPool[i] = track;
    }
    // load splash text for main menu screen
    s->SplashText = game_State_LoadRandomSplashText(s);
    // Load config.json file.
    so_Error err = {0};
    cfg_ConfigResult _res1 = cfg_LoadConfigFile(game_ORG, game_APP, game_CONFIG_FILE_PATH);
    s->Config = _res1.val;
    err = _res1.err;
    if (err.self != NULL) {
        so_panic(so_error_cstr(err));
    }
}

// return false to quit.
bool game_State_Update(void* self) {
    game_State* s = self;
    gui_Update(s->ScreenWidth, s->ScreenHeight, s->Pack);
    gfx_Rectangle screen = (gfx_Rectangle){.W = (float)(s->ScreenWidth), .H = (float)(s->ScreenHeight)};
    game_State_RollBackgroundMusic(s);
    gfx_BeginDrawing();
    gfx_ClearBackground(gfx_Black);
    if (s->CurrentScreeen == (game_SCREEN_MENU_MAIN)) {
        game_State_Screen_MenuMain(s, &s->ScreenMainMenuState, screen);
    } else if (s->CurrentScreeen == (game_SCREEN_MENU_SELECT_SERVER)) {
        game_State_Screen_SelectServer(s, &s->ScreenSelectServerState, screen);
    } else if (s->CurrentScreeen == (game_SCREEN_JOIN_SERVER)) {
        game_State_Screen_JoinServer(s, &s->ScreenJoinServerState, screen);
    } else if (s->CurrentScreeen == (game_SCREEN_MENU_TEXTURE_PACKS)) {
        s->CurrentScreeen = game_SCREEN_MENU_MAIN;
    } else if (s->CurrentScreeen == (game_SCREEN_MENU_OPTIONS)) {
        s->CurrentScreeen = game_SCREEN_MENU_MAIN;
    } else if (s->CurrentScreeen == (game_SCREEN_CONNECT_SERVER)) {
        game_State_Screen_ConnectServer(s, &s->ScreenConnectServerState, screen);
    } else if (s->CurrentScreeen == (game_SCREEN_INGAME)) {
        game_State_Screen_InGame(s, &s->ScreenInGameState, screen);
    }
    gfx_EndDrawing();
    return true;
}

// -- screen_connect_server.go --

void game_State_Screen_ConnectServer(void* self, game_ScreenConnectServerState* state, gfx_Rectangle screen) {
    game_State* s = self;
    if (state->ShouldTransision) {
        s->CurrentScreeen = state->TransisionTo;
        net_Conn_Close(&s->Conn);
        *state = (game_ScreenConnectServerState){};
        return;
    }
    // Draw dirt background
    gfx_Texture bg = s->Pack.GetTexture(s->Pack.self, assets_Gui_background);
    gfx_DrawTextureTiled(bg, gfx_NewRectangle(0, 0, (float)(s->ScreenWidth), (float)(s->ScreenHeight)), gui_Scale * 2, gfx_Color_Tint(gfx_White, gfx_Black, 75));
    // Drawing code
    // draw status text
    gfx_Font* fnt = gui_ActivePack.Font(gui_ActivePack.self);
    so_Slice runes = so_string_runes(state->Text);
    gfx_Vector2 size = gfx_Vector2_Scale(gfx_Font_TextSize(fnt, runes), gui_Scale);
    gfx_Rectangle bbox = gfx_Rectangle_Anchor((gfx_Rectangle){.W = size.X, .H = size.Y}, screen, .5, .5);
    gfx_Font_DrawRunes(fnt, runes, gfx_Rectangle_Position(bbox), gui_Scale, 0, gfx_White, false);
    // draw back button
    bbox.W = gui_ButtonSize.W * gui_Scale;
    bbox.H = gui_ButtonSize.H * gui_Scale;
    bbox = gfx_Rectangle_Anchor(bbox, screen, .5, .5);
    bbox.Y += bbox.H;
    bbox.Y += 4 * gui_Scale;
    bool clicked = s->Inputs[game_InputTap].Released;
    bool hovered = gfx_Rectangle_Contains(bbox, s->Cursor);
    if (clicked && hovered) {
        state->ShouldTransision = true;
        state->TransisionTo = game_SCREEN_MENU_SELECT_SERVER;
        game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        return;
    }
    gui_Button(so_str("Back"), bbox, hovered, true);
    // Update logic code
    // get selected server from config file
    cfg_ServerCfg* srv = &s->Config.Servers[so_min(s->SelectedServer, cfg_MAX_SERVERS - 1)];
    // start dialing
    if (!state->Dialed) {
        state->Arena = mem_NewArena(so_array_slice(so_byte, state->__ArenaBuf, 0, 512, 512));
        state->Dialed = true;
        // blocks
        net_ConnResult _res1 = net_Dial(srv->Host);
        net_Conn conn = _res1.val;
        so_Error err = _res1.err;
        if (err.self != NULL) {
            state->Text = err.Error(err.self);
            state->stage = -1;
        } else {
            s->Conn = conn;
            s->__arenaForServerbound = mem_NewArena(so_array_slice(so_byte, s->__bufioWriterBuffer, 0, 10240, 10240));
            s->ServerBound = bufio_NewWriter((mem_Allocator){.self = &s->__arenaForServerbound, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (io_Writer){.self = &s->Conn, .Write = net_Conn_Write});
            s->__arenaForClientbound = mem_NewArena(so_array_slice(so_byte, s->__bufioReaderBuffer, 0, 41960, 41960));
            s->ClientBound = net_NewBufferedReader((mem_Allocator){.self = &s->__arenaForClientbound, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (io_Reader){.self = &s->Conn, .Read = net_Conn_Read});
        }
    }
    // go back if back is pressed
    if (s->Inputs[game_InputClose].Pressed) {
        state->ShouldTransision = true;
        state->TransisionTo = game_SCREEN_JOIN_SERVER;
        return;
    }
    if (state->stage == (-1)) {
    } else if (state->stage == (0)) {
        // C -> S pre login
        state->Text = so_str("Authenticating");
        bufio_Writer_WriteByte(&s->ServerBound, mc_PKT_PreLogin);
        // prep payload
        state->serverbound_prelogin.Username = so_string_runes(so_str("magicbeta"));
        // write payload
        so_Error err = mc_ServerboundPreLogin_Write(state->serverbound_prelogin, (io_Writer){.self = &s->ServerBound, .Write = bufio_Writer_Write});
        if (err.self != NULL) {
            state->Text = err.Error(err.self);
        } else {
            state->stage++;
            bufio_Writer_Flush(&s->ServerBound);
        }
    } else if (state->stage == (1)) {
        // S -> C pre login
        // read packet id
        so_R_bool_err _res2 = net_SteppedReader_Step(&state->packetID, &s->ClientBound);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (err.self != NULL) {
            state->Text = err.Error(err.self);
        }
        // read payload
        if (ok) {
            so_R_bool_err _res3 = mc_ClientboundPreLogin_Step(&state->clientbound_prelogin, (mem_Allocator){.self = &state->Arena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, &s->ClientBound);
            bool ok = _res3.val;
            so_Error err = _res3.err;
            if (ok) {
                if (so_at(so_rune, state->clientbound_prelogin.ConnectionHash, 0) != U'-') {
                    state->Text = so_str("Only offline mode servers are supported");
                    state->stage = -1;
                } else {
                    state->stage++;
                    state->Text = so_str("Logging in");
                    net_SteppedReader_Reset(&state->packetID);
                }
            }
            if (err.self != NULL) {
                state->Text = err.Error(err.self);
            }
        }
    } else if (state->stage == (2)) {
        // C -> S login
        bufio_Writer_WriteByte(&s->ServerBound, mc_PKT_Login);
        // prep payload
        state->serverbound_login.ProtocolVersion = 14;
        state->serverbound_login.Username = so_string_runes(so_str("magicbeta"));
        // write payload
        so_Error err = mc_ServerboundLogin_Write(state->serverbound_login, (io_Writer){.self = &s->ServerBound, .Write = bufio_Writer_Write});
        if (err.self != NULL) {
            state->Text = err.Error(err.self);
        } else {
            state->stage++;
            bufio_Writer_Flush(&s->ServerBound);
        }
    } else if (state->stage == (3)) {
        // S -> C login
        // read packet id
        so_R_bool_err _res4 = net_SteppedReader_Step(&state->packetID, &s->ClientBound);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (err.self != NULL) {
            state->Text = err.Error(err.self);
        }
        if (ok) {
            so_R_bool_err _res5 = mc_ClientboundLogin_Step(&state->clientbound_login, (mem_Allocator){0}, &s->ClientBound);
            ok = _res5.val;
            err = _res5.err;
            if (ok) {
                state->stage++;
                net_SteppedReader_Reset(&state->packetID);
            }
            if (err.self != NULL) {
                state->Text = err.Error(err.self);
            }
        }
    } else if (state->stage == (4)) {
        state->Text = so_str("Connected");
        // dont do the "Sould Transition" thing yet.
        s->CurrentScreeen = game_SCREEN_INGAME;
        // state.ShouldTransision = true
        // state.TransisionTo = SCREEN_INGAME
        return;
    }
}

// -- screen_ingame.go --

void game_ScreenInGameState_Init(void* self, game_State* s) {
    game_ScreenInGameState* state = self;
    state->Cam = (gfx_Camera){.Position = (gfx_Vector3){.Y = 2}, .Target = (gfx_Vector3){.Z = 1}, .Up = (gfx_Vector3){.Y = 1}, .Fovy = 70};
    state->PacketDecodeArena = mem_NewArena(so_array_slice(so_byte, state->__PacketDecodeArenaMemory, 0, 102400, 102400));
    state->PersistentArena = mem_NewArena(so_array_slice(so_byte, state->__PersistentMemory, 0, 2097152, 2097152));
}

void game_State_Screen_InGame(void* self, game_ScreenInGameState* state, gfx_Rectangle screen) {
    game_State* s = self;
    if (!state->Initialized) {
        *state = (game_ScreenInGameState){};
        game_ScreenInGameState_Init(state, s);
        state->Initialized = true;
    }
    if (state->Disconnected) {
        game_ScreenInGameState_OnDisconnect(state, s, screen);
        return;
    }
    {
        state->Error = game_ScreenInGameState_DecodePackets(state, s);
        if (state->Error.self != NULL) {
            state->Disconnected = true;
            SDL_Log("Closing because decode error, state=%d", state->DecodeState);
            net_Conn_Close(&s->Conn);
        }
    }
}

so_Error game_ScreenInGameState_DecodePackets(void* self, game_State* s) {
    game_ScreenInGameState* state = self;
    const int64_t WAITING_PACKET = 0;
    const int64_t DECODING_PACKET = 1;
    const int64_t HANDLING_PACKET = 2;
    if (state->DecodeState == (WAITING_PACKET)) {
        so_Slice b = so_make_slice(so_byte, 1, 1);
        so_R_int_err _res1 = net_BufferedReader_Read(&s->ClientBound, b);
        so_int n = _res1.val;
        so_Error err = _res1.err;
        if (err.self != NULL) {
            return err;
        }
        so_byte id = 0;
        if (n == 1) {
            id = so_at(so_byte, b, 0);
        }
        if (id == 0) {
            // The vanilla server does not send them.
            SDL_Log("Keep alive packet detected. Is the stream corrupted?");
            return (so_Error){0};
        }
        // Got a real packet.
        state->PacketID = id;
        mem_Arena_Reset(&state->PacketDecodeArena);
        if (id == mc_PKT_SetChunkVisibility) {
            state->scv = (mc_ClientboundSetChunkVisibility){};
            state->Decoder = (mc_Decoder){.self = &state->scv, .Step = mc_ClientboundSetChunkVisibility_Step};
        } else {
            state->Decoder = mc_NewDecoder((mem_Allocator){.self = &state->PacketDecodeArena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, id);
        }
        if (state->Decoder.self == NULL) {
            return game_NoDecoderForPacketErr;
        }
        state->DecodeState = DECODING_PACKET;
    } else if (state->DecodeState == (DECODING_PACKET)) {
        if (state->Decoder.self == NULL) {
            so_panic("Decoder should not be nil at this stage");
        }
        so_R_bool_err _res2 = state->Decoder.Step(state->Decoder.self, (mem_Allocator){.self = &state->PacketDecodeArena, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, &s->ClientBound);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (err.self != NULL) {
            return err;
        }
        if (ok) {
            state->DecodeState = HANDLING_PACKET;
            return (so_Error){0};
        }
    } else if (state->DecodeState == (HANDLING_PACKET)) {
        game_ScreenInGameState_dispatchPacketHandler(state, state->PacketID, state->Decoder);
        state->DecodeState = WAITING_PACKET;
    }
    return (so_Error){0};
}

// Show disconnected screen.
void game_ScreenInGameState_OnDisconnect(void* self, game_State* s, gfx_Rectangle screen) {
    game_ScreenInGameState* state = self;
    // Draw dirt background
    gfx_Texture bg = s->Pack.GetTexture(s->Pack.self, assets_Gui_background);
    gfx_DrawTextureTiled(bg, gfx_NewRectangle(0, 0, (float)(s->ScreenWidth), (float)(s->ScreenHeight)), gui_Scale * 2, gfx_Color_Tint(gfx_White, gfx_Black, 75));
    gfx_Font* fnt = gui_ActivePack.Font(gui_ActivePack.self);
    so_Slice runes = so_string_runes(state->Error.Error(state->Error.self));
    if (state->Error.self == game_NoDecoderForPacketErr.self) {
        runes = so_string_runes(fmt_Sprintf(fmt_NewBuffer(100), "Cannot decode %s", so_cstr(mc_PacketIDString(state->PacketID))));
    }
    gfx_Vector2 size = gfx_Vector2_Scale(gfx_Font_TextSize(fnt, runes), gui_Scale);
    gfx_Rectangle bbox = gfx_Rectangle_Anchor((gfx_Rectangle){.W = size.X, .H = size.Y}, screen, .5, .5);
    gfx_Font_DrawRunes(fnt, runes, gfx_Rectangle_Position(bbox), gui_Scale, 0, gfx_White, false);
    // draw back button
    bbox.W = gui_ButtonSize.W * gui_Scale;
    bbox.H = gui_ButtonSize.H * gui_Scale;
    bbox = gfx_Rectangle_Anchor(bbox, screen, .5, .5);
    bbox.Y += bbox.H;
    bbox.Y += 4 * gui_Scale;
    bool clicked = s->Inputs[game_InputTap].Released;
    bool hovered = gfx_Rectangle_Contains(bbox, s->Cursor);
    if (clicked && hovered) {
        game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        s->CurrentScreeen = game_SCREEN_MENU_SELECT_SERVER;
        s->ScreenConnectServerState = (game_ScreenConnectServerState){};
        return;
    }
    gui_Button(so_str("Back"), bbox, hovered, true);
}

// -- screen_ingame_packetHandlers.go --

void game_ScreenInGameState_OnSetSpawnPosition(void* self, mc_Decoder data) {
    game_ScreenInGameState* state = self;
    mc_ClientboundSetSpawnPosition* pkt = (mc_ClientboundSetSpawnPosition*)data.self;
    state->SpawnPosition = gfx_NewVector3((float)(pkt->X), (float)(pkt->Y), (float)(pkt->Z));
}

void game_ScreenInGameState_OnSetTime(void* self, mc_Decoder data) {
    game_ScreenInGameState* state = self;
    mc_ClientboundSetTime* pkt = (mc_ClientboundSetTime*)data.self;
    state->Time = pkt->Time;
}

void game_ScreenInGameState_OnSpawnMob(void* self, mc_Decoder data) {
    game_ScreenInGameState* state = self;
    mc_ClientBoundSpawnMob* pkt = (mc_ClientBoundSpawnMob*)data.self;
    // nothing for now.
    (void)pkt;
}

// register packet handlers here
static void game_ScreenInGameState_dispatchPacketHandler(void* self, so_byte id, mc_Decoder data) {
    game_ScreenInGameState* state = self;
    if (id == (mc_PKT_SetSpawnPosition)) {
        game_ScreenInGameState_OnSetSpawnPosition(state, data);
    } else if (id == (mc_PKT_SetTime)) {
        game_ScreenInGameState_OnSetTime(state, data);
    } else if (id == (mc_PKT_SpawnMob)) {
        game_ScreenInGameState_OnSpawnMob(state, data);
    } else {
        SDL_Log("No handler registered for %s", so_cstr(mc_PacketIDString(state->PacketID)));
        return;
    }
}

// -- screen_join_server.go --

void game_State_Screen_JoinServer(void* self, game_ScreenJoinServerState* state, gfx_Rectangle screen) {
    game_State* s = self;
    // Draw dirt background
    gfx_Texture bg = s->Pack.GetTexture(s->Pack.self, assets_Gui_background);
    gfx_DrawTextureTiled(bg, gfx_NewRectangle(0, 0, (float)(s->ScreenWidth), (float)(s->ScreenHeight)), gui_Scale * 2, gfx_Color_Tint(gfx_White, gfx_Black, 75));
    s->InteractingWithUI = true;
    // get selected server from config file
    cfg_ServerCfg* srv = &s->Config.Servers[so_min(s->SelectedServer, cfg_MAX_SERVERS - 1)];
    if (state->ShouldTransition) {
        if (so_string_ne(srv->Host, game_TextInputBuffer_String(state->TextFields[1]))) {
            srv->Host = game_TextInputBuffer_String(state->TextFields[1]);
            *srv = cfg_ServerCfg_Clone(*srv);
        }
        if (so_string_ne(srv->Cmd, game_TextInputBuffer_String(state->TextFields[2]))) {
            srv->Cmd = game_TextInputBuffer_String(state->TextFields[2]);
            *srv = cfg_ServerCfg_Clone(*srv);
        }
        cfg_SaveConfigFile(game_ORG, game_APP, game_CONFIG_FILE_PATH, s->Config);
        s->TextInputActive = false;
        // reset state on switch
        s->CurrentScreeen = state->switchToScreen;
        *state = (game_ScreenJoinServerState){};
        return;
    }
    // init
    if (state->HaveInitialized == false) {
        game_TextInputBuffer_Init(&state->TextFields[1], srv->Host);
        game_TextInputBuffer_Init(&state->TextFields[2], srv->Cmd);
        state->HaveInitialized = true;
    }
    // go back if close input
    if (s->Inputs[game_InputClose].Pressed) {
        state->ShouldTransition = true;
        state->switchToScreen = game_SCREEN_MENU_SELECT_SERVER;
    }
    // Dpad Navigation (0: Hostname, 1: Cmd, 2: Connect, 3: Back)
    const int64_t NInteractables = 4;
    if (s->UIDpadMode && (s->Inputs[game_InputDown].Pressed || s->Inputs[game_InputRight].Pressed)) {
        state->selected = so_min(state->selected + 1, NInteractables - 1);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    if (s->UIDpadMode && (s->Inputs[game_InputUp].Pressed || s->Inputs[game_InputLeft].Pressed)) {
        state->selected = so_max(state->selected - 1, 0);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    // content bbox for this screen.
    gfx_Rectangle content = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W, .H = 160}, gui_Scale), screen, .5, .45);
    gfx_Vector2 vertical = (gfx_Vector2){.X = content.X, .Y = content.Y};
    gfx_Font* fnt = gui_ActivePack.Font(gui_ActivePack.self);
    // --- Layout Setup ---
    // Hostname text field header text
    vertical.Y += 10 * gui_Scale;
    gfx_Font_DrawRunes(fnt, so_string_runes(so_str("Hostname including port:")), vertical, gui_Scale, 0, gfx_White, false);
    vertical.Y += (float)(gfx_Font_TextHeight(fnt)) * gui_Scale + 2 * gui_Scale;
    // hostname text field rect
    gfx_Rectangle hostname = gfx_Rectangle_SetPosition(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), vertical);
    vertical.Y += hostname.H;
    // cmd text field header text
    vertical.Y += 10 * gui_Scale;
    gfx_Font_DrawRunes(fnt, so_string_runes(so_str("Command to run on join:")), vertical, gui_Scale, 0, gfx_White, false);
    vertical.Y += (float)(gfx_Font_TextHeight(fnt)) * gui_Scale + 2 * gui_Scale;
    // cmd text field rect
    gfx_Rectangle cmd = gfx_Rectangle_SetPosition(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), vertical);
    // --- Interaction Logic ---
    // Establish a unified click state for this frame
    bool clicked = s->Inputs[game_InputTap].Released;
    if (s->UIDpadMode) {
        clicked = s->Inputs[game_InputReturn].Released;
    }
    // Determine hover/visual selection states
    bool hostnameHovered = gfx_Rectangle_Contains(hostname, s->Cursor);
    bool cmdHovered = gfx_Rectangle_Contains(cmd, s->Cursor);
    if (s->UIDpadMode) {
        hostnameHovered = (state->selected == 0);
        cmdHovered = (state->selected == 1);
    }
    // Text Field Focus Logic
    if (clicked) {
        if (hostnameHovered) {
            state->TextFieldFocused = 1;
            s->TextInputActive = true;
        } else if (cmdHovered) {
            state->TextFieldFocused = 2;
            s->TextInputActive = true;
        } else {
            // Clicked elsewhere; unfocus
            state->TextFieldFocused = 0;
            s->TextInputActive = false;
        }
    } else if (!s->TextInputActive) {
        // Update visual focus based on hover (if not actively typing)
        if (hostnameHovered) {
            state->TextFieldFocused = 1;
        } else if (cmdHovered) {
            state->TextFieldFocused = 2;
        } else if (!s->UIDpadMode) {
            state->TextFieldFocused = 0;
        } else if (s->UIDpadMode && state->selected > 1) {
            state->TextFieldFocused = 0;
        }
    }
    // Draw Text Fields using the properly evaluated focus state
    gui_TextField(game_TextInputBuffer_String(state->TextFields[1]), so_str("example.com:25565"), hostname, state->TextFieldFocused == 1, s->TextInputActive && state->TextFieldFocused == 1);
    gui_TextField(game_TextInputBuffer_String(state->TextFields[2]), so_str("eg. /login password123"), cmd, state->TextFieldFocused == 2, s->TextInputActive && state->TextFieldFocused == 2);
    // --- Buttons ---
    gfx_Rectangle actionButtons = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W, .H = gui_ButtonSize.H * 2 + 2}, gui_Scale), content, .5, 1);
    // Connect button
    {
        gfx_Rectangle connectButton = gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), actionButtons, .5, 0);
        bool hovered = gfx_Rectangle_Contains(connectButton, s->Cursor);
        if (s->UIDpadMode) {
            hovered = (state->selected == 2);
        }
        bool enabled = so_string_ne(game_TextInputBuffer_String(state->TextFields[1]), so_str(""));
        if (hovered && clicked && enabled) {
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
            state->ShouldTransition = true;
            state->switchToScreen = game_SCREEN_CONNECT_SERVER;
        }
        gui_Button(so_str("Connect"), connectButton, hovered, enabled);
    }
    // Back button
    {
        gfx_Rectangle backButton = gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), actionButtons, .5, 1);
        bool hovered = gfx_Rectangle_Contains(backButton, s->Cursor);
        if (s->UIDpadMode) {
            hovered = (state->selected == 3);
        }
        gui_Button(so_str("Back"), backButton, hovered, true);
        if (hovered && clicked) {
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
            state->ShouldTransition = true;
            state->switchToScreen = game_SCREEN_MENU_SELECT_SERVER;
        }
    }
    // --- Text Typing Input Processing ---
    if ((state->TextFieldFocused == 1 || state->TextFieldFocused == 2) && s->TextInputActive) {
        game_TextInputBuffer* tf = &state->TextFields[state->TextFieldFocused];
        so_rune input = s->Inputs[game_InputTextInput].Text;
        if (input != 0 && tf->Len < 70) {
            game_TextInputBuffer_Add(tf, input);
        }
        if (s->Inputs[game_InputBackspace].Pressed) {
            game_TextInputBuffer_Pop(tf);
        }
    }
}

// -- screen_menu_main.go --

void game_State_Screen_MenuMain(void* self, game_ScreenMainMenuState* state, gfx_Rectangle screen) {
    game_State* s = self;
    s->InteractingWithUI = true;
    // draw background
    gfx_Texture bg = s->Pack.GetTexture(s->Pack.self, assets_Gui_background);
    // Draw dirt background
    gfx_DrawTextureTiled(bg, gfx_NewRectangle(0, 0, (float)(s->ScreenWidth), (float)(s->ScreenHeight)), gui_Scale * 2, gfx_Color_Tint(gfx_White, gfx_Black, 75));
    // bounding box that contains title screen and buttons
    gfx_Rectangle menuScreen = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.H = gui_Base.W * .75, .W = 200}, gui_Scale), screen, .5, .1);
    // Draw Minecraft logo
    gfx_Rectangle logo = gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_MinecraftLogoSize, gui_Scale), menuScreen, .50, .1);
    gui_MinecraftLogo(s->SplashText, logo);
    // Draw buttons
    so_String ButtonTitles[3] = {so_str("Join Server"), so_str("Texture Packs"), so_str("Options")};
    so_int NInteractables = 3;
    if (s->UIDpadMode && (s->Inputs[game_InputDown].Pressed || s->Inputs[game_InputRight].Pressed)) {
        state->selected = so_min(state->selected + 1, NInteractables - 1);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    if (s->UIDpadMode && (s->Inputs[game_InputUp].Pressed || s->Inputs[game_InputLeft].Pressed)) {
        state->selected = so_max(state->selected - 1, 0);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    gfx_Rectangle buttonSet = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W, .H = (gui_ButtonSize.H + 2) * 4}, gui_Scale), menuScreen, .5, .70);
    gfx_Rectangle btn = gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), buttonSet, .5, 0);
    for (so_int i = 0; i < 3; i++) {
        if (i != 0) {
            btn.Y += btn.H;
            //padding
            btn.Y += 2 * gui_Scale;
        }
        bool hovered = gfx_Rectangle_Contains(btn, s->Cursor);
        bool clicked = s->Inputs[game_InputTap].Released;
        if (s->UIDpadMode) {
            hovered = state->selected == i;
            clicked = s->Inputs[game_InputReturn].Released;
        }
        // selected :=
        if (hovered && clicked) {
            // Switch screen
            s->CurrentScreeen = game_SCREEN_MENU_MAIN + i + 1;
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        }
        gui_Button(ButtonTitles[i], btn, hovered, true);
    }
}

so_String game_State_LoadRandomSplashText(void* self) {
    game_State* s = self;
    mem_Arena_Reset(&s->Scratch);
    // get size of file in bytes
    so_R_slice_err _res1 = SDL_Storage_ReadFile(s->Storage, (mem_Allocator){.self = &s->Scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, so_str("assets/title/splashes.txt"));
    so_Slice file = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        SDL_LogError(1, "Failed to open file %s", so_cstr(err.Error(err.self)));
        return err.Error(err.self);
    }
    bytes_Buffer r = bytes_NewBuffer((mem_Allocator){.self = &s->Scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, file);
    const int64_t TotalSplashes = 226;
    so_int n = rand_IntN(TotalSplashes - 1);
    so_String final = so_str("");
    for (so_int _i = 0; _i < n; _i++) {
        so_R_str_err _res2 = bytes_Buffer_ReadString(&r, '\n');
        final = _res2.val;
        err = _res2.err;
        if (err.self != NULL) {
            so_panic(so_error_cstr(err));
        }
    }
    final = strings_TrimSpace(final);
    return strings_Clone(mem_System, final);
}

// -- screen_select_server.go --

void game_State_Screen_SelectServer(void* self, game_ScreenSelectServerState* state, gfx_Rectangle screen) {
    game_State* s = self;
    s->InteractingWithUI = true;
    // draw background
    gfx_Texture bg = s->Pack.GetTexture(s->Pack.self, assets_Gui_background);
    // Draw dirt background
    gfx_DrawTextureTiled(bg, gfx_NewRectangle(0, 0, (float)(s->ScreenWidth), (float)(s->ScreenHeight)), gui_Scale * 2, gfx_Color_Tint(gfx_White, gfx_Black, 75));
    // go to main menu if close input
    if (s->Inputs[game_InputClose].Pressed) {
        s->CurrentScreeen = game_SCREEN_MENU_MAIN;
    }
    const int64_t NInteractables = 5 + 2 + 1;
    if (s->UIDpadMode && (s->Inputs[game_InputDown].Pressed || s->Inputs[game_InputRight].Pressed)) {
        state->selected = so_min(state->selected + 1, NInteractables - 1);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    if (s->UIDpadMode && (s->Inputs[game_InputUp].Pressed || s->Inputs[game_InputLeft].Pressed)) {
        state->selected = so_max(state->selected - 1, 0);
        game_State_PlaySoundEffect(s, assets_Newsound_step_stone3);
        // Stop typing if focus moves
        s->TextInputActive = false;
    }
    gfx_Rectangle list = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W, .H = 160}, gui_Scale), screen, .5, .45);
    gfx_Rectangle btn = gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), list, .5, 0);
    const int64_t MaxPerScreen = 5;
    // rounds down
    so_int pageCount = cfg_MAX_SERVERS / MaxPerScreen;
    // always round up the number of pages if needed.
    if (cfg_MAX_SERVERS % MaxPerScreen != 0) {
        pageCount++;
    }
    so_int maxPage = so_max(0, pageCount - 1);
    for (so_int i = 0; i < MaxPerScreen; i++) {
        so_int idx = state->PageIndex * MaxPerScreen + i;
        if (i != 0) {
            btn.Y += btn.H;
            //padding
            btn.Y += 2 * gui_Scale;
        }
        bool hovered = gfx_Rectangle_Contains(btn, s->Cursor);
        bool clicked = s->Inputs[game_InputTap].Released;
        if (s->UIDpadMode) {
            hovered = state->selected == i;
            clicked = s->Inputs[game_InputReturn].Released;
        }
        if (idx >= 25) {
            so_panic("screen_join_server: how is this possible?");
        }
        // Set selected server
        cfg_ServerCfg srv = s->Config.Servers[idx];
        if (so_string_eq(s->Config.Servers[idx].Host, so_str(""))) {
            gui_Button(so_str("[EMPTY]"), btn, hovered, true);
        } else {
            gui_Button(srv.Host, btn, hovered, true);
        }
        if (clicked && hovered) {
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
            s->SelectedServer = (so_uint)(idx);
            s->CurrentScreeen = game_SCREEN_JOIN_SERVER;
        }
    }
    // next/prev buttons + pager bounding box
    gfx_Rectangle navGroup = gfx_Rectangle_Anchor(gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W, .H = gui_ButtonSize.H * 2}, gui_Scale), list, .5, .95);
    // half width button
    gfx_Rectangle halfBtn = gfx_Rectangle_Scale((gfx_Rectangle){.W = gui_ButtonSize.W / 2 - 1, .H = gui_ButtonSize.H}, gui_Scale);
    // anchored left
    gfx_Rectangle prevBtn = gfx_Rectangle_Anchor(halfBtn, navGroup, 0, 1);
    // anchored right
    gfx_Rectangle nextBtn = gfx_Rectangle_Anchor(halfBtn, navGroup, 1, 1);
    // Page number
    so_Slice tmp = so_make_slice(so_byte, strconv_MaxIntBase10Len * 10, strconv_MaxIntBase10Len * 10);
    mem_Arena_Reset(&s->Scratch);
    // Page n/pageCount
    strings_Builder sb = strings_NewBuilder((mem_Allocator){.self = &s->Scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc});
    strings_Builder_WriteString(&sb, so_str("Page "));
    strings_Builder_WriteString(&sb, strconv_Itoa(tmp, state->PageIndex + 1));
    strings_Builder_WriteRune(&sb, U'/');
    strings_Builder_WriteString(&sb, strconv_Itoa(tmp, pageCount));
    gui_Button(strings_Builder_String(&sb), gfx_Rectangle_Anchor(gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale), navGroup, .5, 0), false, false);
    {
        // previous button click
        bool hovered = gfx_Rectangle_Contains(prevBtn, s->Cursor);
        bool clicked = s->Inputs[game_InputTap].Released;
        bool enabled = state->PageIndex != 0;
        if (s->UIDpadMode) {
            hovered = state->selected == 5;
            clicked = s->Inputs[game_InputReturn].Released;
        }
        if (enabled && hovered && clicked) {
            state->PageIndex = so_max(state->PageIndex - 1, 0);
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        }
        gui_Button(so_str("Prev"), prevBtn, hovered, enabled);
    }
    {
        // next button click
        bool hovered = gfx_Rectangle_Contains(nextBtn, s->Cursor);
        bool clicked = s->Inputs[game_InputTap].Released;
        if (s->UIDpadMode) {
            hovered = state->selected == 6;
            clicked = s->Inputs[game_InputReturn].Released;
        }
        bool enabled = state->PageIndex != maxPage;
        if (enabled && hovered && clicked) {
            state->PageIndex = so_min(state->PageIndex + 1, maxPage);
            game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        }
        gui_Button(so_str("Next"), nextBtn, hovered, enabled);
    }
    gfx_Rectangle backButton = gfx_Rectangle_Scale(gui_ButtonSize, gui_Scale);
    backButton.X = btn.X;
    backButton.Y = nextBtn.Y + nextBtn.H;
    backButton.Y += nextBtn.H;
    bool hovered = gfx_Rectangle_Contains(backButton, s->Cursor);
    bool clicked = s->Inputs[game_InputTap].Released;
    if (s->UIDpadMode) {
        hovered = state->selected == 7;
        clicked = s->Inputs[game_InputReturn].Released;
    }
    if (hovered && clicked) {
        s->CurrentScreeen = game_SCREEN_MENU_MAIN;
        game_State_PlaySoundEffect(s, assets_Newsound_random_click);
        return;
    }
    gui_Button(so_str("Back"), backButton, hovered, true);
}

// -- sounds.go --

// should be ran every frame
void game_State_RollBackgroundMusic(void* self) {
    game_State* s = self;
    if (time_Since(s->TimeSinceLastBackgroundMusicRoll) < game_RollMusicEvery) {
        return;
    }
    s->TimeSinceLastBackgroundMusicRoll = time_Now();
    assets_ID musics[10] = {assets_Newmusic_hal1, assets_Newmusic_hal2, assets_Newmusic_hal3, assets_Newmusic_hal4, assets_Newmusic_hal4, assets_Newmusic_nuance1, assets_Newmusic_nuance2, assets_Newmusic_piano1, assets_Newmusic_piano2, assets_Newmusic_piano3};
    so_int n = rand_IntN(10 * 2);
    if (n > 10 - 1) {
        return;
    }
    mem_Arena_Reset(&s->Scratch);
    SDL_IOStream* f = SDL_IOFromFile(so_cstr(path_Join((mem_Allocator){.self = &s->Scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (so_Slice){(so_String[2]){gfx_AssetsPath, assets_ID_String(musics[n])}, 2, 2})), "rb");
    if (f == NULL) {
        // should always succeed because wtf
        so_panic(so_error_cstr(sdl_GetError()));
    }
    if (!MIX_SetTrackIOStream(s->MusicTrack, f, true)) {
        so_panic("game.RollBackgroundMusic: Failed to set music track");
    }
    MIX_PlayTrack(s->MusicTrack, 0);
}

static MIX_Audio* game_State_getAudio(void* self, assets_ID audio) {
    game_State* s = self;
    if (maps_Map_Has(assets_ID, MIX_Audio*, (&s->Audios), (audio))) {
        return maps_Map_Get(assets_ID, MIX_Audio*, (&s->Audios), (audio));
    }
    if (maps_Map_Len(assets_ID, MIX_Audio*, (&s->Audios)) > game_MaxAudioLoaded) {
        // delete one audio.
        maps_Iter i = maps_Map_Iter(assets_ID, MIX_Audio*, (&s->Audios));
        assets_ID id = maps_Iter_Key(assets_ID, MIX_Audio*, (&i));
        MIX_DestroyAudio(maps_Iter_Value(assets_ID, MIX_Audio*, (&i)));
        maps_Map_Delete(assets_ID, MIX_Audio*, (&s->Audios), (id));
    }
    mem_Arena_Reset(&s->Scratch);
    MIX_Audio* file = MIX_LoadAudio(s->Mixer, so_cstr(path_Join((mem_Allocator){.self = &s->Scratch, .Alloc = mem_Arena_Alloc, .Free = mem_Arena_Free, .Realloc = mem_Arena_Realloc}, (so_Slice){(so_String[2]){gfx_AssetsPath, assets_ID_String(audio)}, 2, 2})), true);
    maps_Map_Set(assets_ID, MIX_Audio*, (&s->Audios), (audio), (file));
    if (file == NULL) {
        so_panic(so_error_cstr(sdl_GetError()));
    }
    return file;
}

MIX_Track* game_State_PlaySoundEffect(void* self, assets_ID audio) {
    game_State* s = self;
    for (so_int _ = 0; _ < 10; _++) {
        MIX_Track* t = s->TracksPool[_];
        if (!MIX_TrackPlaying(t)) {
            MIX_SetTrackAudio(t, game_State_getAudio(s, audio));
            MIX_PlayTrack(t, 0);
            return t;
        }
    }
    c_Assert(false, so_cstr(fmt_Sprintf(fmt_NewBuffer(2048), "PlaySoundEffect: out of Tracks in the pool. trying to play %s", so_cstr(assets_ID_String(audio)))));
    return NULL;
}

// -- types.go --

void game_TextInputBuffer_Init(void* self, so_String s) {
    game_TextInputBuffer* t = self;
    *t = (game_TextInputBuffer){};
    for (so_int _ = 0; _ < so_len(so_string_runes(s)); _++) {
        so_rune r = so_at(so_rune, so_string_runes(s), _);
        if (t->Len == game_MAX_TEXT_INPUT) {
            break;
        }
        t->Text[t->Len] = r;
        t->Len++;
    }
}

void game_TextInputBuffer_Add(void* self, so_rune r) {
    game_TextInputBuffer* t = self;
    if (t->Len == game_MAX_TEXT_INPUT) {
        return;
    }
    t->Text[t->Len] = r;
    t->Len++;
}

void game_TextInputBuffer_Pop(void* self) {
    game_TextInputBuffer* t = self;
    if (t->Len == 0) {
        return;
    }
    t->Len--;
    t->Text[t->Len] = 0;
}

so_String game_TextInputBuffer_String(game_TextInputBuffer t) {
    return so_runes_string(so_array_slice(so_rune, t.Text, 0, t.Len, 256));
}

game_ThingRef game_ThingPool_New(void* self, game_Kind kind) {
    game_ThingPool* things = self;
    so_uint i = 0;
    if (things->FreeListLen > 0) {
        things->FreeListCursor--;
        things->FreeListLen--;
        i = things->FreeListMemory[things->FreeListCursor];
    } else {
        i = things->SlotsUsed + 1;
    }
    things->Things[i] = (game_Thing){};
    things->Things[i].Kind = kind;
    things->used[i] = true;
    things->SlotsUsed++;
    return (game_ThingRef){.idx = i, .gen = things->gen[i]};
}

void game_ThingPool_Delete(void* self, game_ThingRef ref) {
    game_ThingPool* things = self;
    if (ref.gen == things->gen[ref.idx]) {
        things->used[ref.idx] = false;
        things->gen[ref.idx] += 1;
        things->SlotsUsed--;
        things->FreeListMemory[things->FreeListCursor] = ref.idx;
        things->FreeListCursor++;
        things->FreeListLen++;
    }
}

game_Thing* game_ThingsIter_Thing(void* self) {
    game_ThingsIter* it = self;
    return &it->p->Things[it->idx];
}

game_ThingRef game_ThingsIter_Ref(void* self) {
    game_ThingsIter* it = self;
    return (game_ThingRef){.idx = it->idx, .gen = it->p->gen[it->idx]};
}

bool game_ThingsIter_Next(void* self) {
    game_ThingsIter* it = self;
    it->idx++;
    // Find the next valid item
    for (; it->idx < game_MAX_THINGS;) {
        if (it->p->used[it->idx]) {
            return true;
        }
        it->idx++;
    }
    return false;
}

game_ThingsIter game_ThingPool_Iter(void* self) {
    game_ThingPool* things = self;
    return (game_ThingsIter){.p = things};
}
