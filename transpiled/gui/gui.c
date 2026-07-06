#include "gui.h"

// -- Variables and constants --
gfx_Rectangle gui_ButtonSize = (gfx_Rectangle){.W = 200, .H = 20};
gfx_Rectangle gui_Base = (gfx_Rectangle){.W = 320, .H = 180};
gfx_TexturePack gui_ActivePack = {0};
float gui_Scale = 0;

// size of Minecraft Logo
gfx_Rectangle gui_MinecraftLogoSize = (gfx_Rectangle){.W = 155 + 120, .H = 45};

// -- button.go --

void gui_Button(so_String Text, gfx_Rectangle bbox, bool Hovered, bool Enabled) {
    gfx_Texture GuiTexture = gui_ActivePack.GetTexture(gui_ActivePack.self, assets_Gui_gui);
    // atlas scale
    float as = (float)(GuiTexture.Width) / 256;
    float state = (float)(1);
    if (!Enabled) {
        state = 0;
    } else if (Hovered) {
        state = 2;
    }
    gfx_Rectangle src = gfx_Rectangle_Scale((gfx_Rectangle){.X = 0, .Y = 46 + state * 20, .W = 100, .H = 20}, as);
    // draw button in two halves, centered.
    gfx_Rectangle dst = bbox;
    dst.W *= .5;
    gfx_DrawTextureRec(GuiTexture, src, dst);
    // capture other half
    src.X += 100;
    dst.X += bbox.W / 2;
    gfx_DrawTextureRec(GuiTexture, src, dst);
    so_Slice runes = so_string_runes(Text);
    gfx_Font* font = gui_ActivePack.Font(gui_ActivePack.self);
    gfx_Rectangle tBB = gfx_Rectangle_Anchor((gfx_Rectangle){.W = (float)(gfx_Font_TextWidth(font, runes)) * gui_Scale, .H = (float)(gfx_Font_TextHeight(font)) * gui_Scale}, bbox, .5, .5);
    gfx_Color btnTextColor = gfx_White;
    if (!Enabled) {
        btnTextColor = gfx_Gray;
    } else if (Hovered) {
        btnTextColor = gfx_Yellow;
    }
    if (!Enabled && Hovered) {
        btnTextColor = gfx_DarkRed;
    }
    // shadow
    gfx_Font_DrawRunes(font, runes, gfx_Vector2_AddValue(gfx_Rectangle_Position(tBB), 1 * gui_Scale), gui_Scale, 0, btnTextColor, true);
    gfx_Font_DrawRunes(font, runes, gfx_Rectangle_Position(tBB), gui_Scale, 0, btnTextColor, false);
}

// -- gui.go --

// Must be called whenever screen size changes.
void gui_Update(float screenW, float screenH, gfx_TexturePack pack) {
    gui_ActivePack = pack;
    so_int scale = 0;
    float sx = screenW / gui_Base.W;
    float sy = screenH / gui_Base.H;
    scale = (so_int)(so_min(sx, sy));
    scale = so_max(1, scale);
    gui_Scale = so_min((float)(scale), gui_MaxAutoScale);
}

// -- minecraftLogo.go --

void gui_MinecraftLogo(so_String Splash, gfx_Rectangle bbox) {
    gfx_Texture logoTexture = gui_ActivePack.GetTexture(gui_ActivePack.self, assets_Title_mclogo);
    float as = (float)(logoTexture.Width) / 256;
    gfx_Rectangle logoSrc1 = gfx_Rectangle_Scale((gfx_Rectangle){.W = 155, .H = 45}, as);
    gfx_Rectangle logoSrc2 = gfx_Rectangle_Scale((gfx_Rectangle){.X = 0, .Y = 45, .W = 120, .H = 90}, as);
    gfx_Rectangle dst1 = gfx_Rectangle_SetPosition((gfx_Rectangle){.W = 155 * gui_Scale, .H = 45 * gui_Scale}, gfx_Rectangle_Position(bbox));
    gfx_DrawTextureEx(logoTexture, logoSrc1, dst1);
    gfx_Rectangle dst2 = gfx_Rectangle_Scale((gfx_Rectangle){.X = bbox.X + dst1.W, .Y = bbox.Y, .W = 120, .H = 90}, gui_Scale);
    gfx_DrawTextureEx(logoTexture, logoSrc2, dst2);
    gfx_Font* font = gui_ActivePack.Font(gui_ActivePack.self);
    double t = (double)(time_Time_UnixMilli(time_Now()) % 1000) / 1000.0;
    double wave = math_Sin(t * 2 * math_Pi) * 0.2;
    float scale = (float)((double)(gui_Scale) - math_Abs(wave)) + .5;
    gfx_Vector2 textSize = gfx_Vector2_Scale(gfx_Font_TextSize(font, so_string_runes(Splash)), scale);
    gfx_Rectangle anchor = (gfx_Rectangle){.X = bbox.X + bbox.W * .85, .Y = bbox.Y + bbox.H * .8};
    gfx_Vector2 pos = (gfx_Vector2){.X = anchor.X - textSize.X / 2, .Y = anchor.Y - textSize.Y / 2};
    gfx_Font_DrawRunes(font, so_string_runes(Splash), pos, (float)(scale), -20, gfx_Yellow, false);
}

// -- textField.go --

void gui_TextField(so_String Text, so_String placeholder, gfx_Rectangle bbox, bool Hovered, bool Enabled) {
    so_Slice runes = so_string_runes(Text);
    // blink every second
    bool blink = (time_Time_Second(time_Now())) % 2 == 0;
    const int64_t borderSize = 2;
    gfx_Rectangle border = gfx_Rectangle_Grow(bbox, borderSize);
    if (Hovered || Enabled) {
        gfx_DrawRectangle(border, gfx_White);
    } else {
        gfx_DrawRectangle(border, gfx_Gray);
    }
    gfx_DrawRectangle(bbox, gfx_Black);
    // align text
    gfx_Font* font = gui_ActivePack.Font(gui_ActivePack.self);
    gfx_Rectangle tBB = gfx_Rectangle_Anchor((gfx_Rectangle){.W = (float)(gfx_Font_TextWidth(font, runes)) * gui_Scale, .H = (float)(gfx_Font_TextHeight(font)) * gui_Scale}, bbox, 0, .5);
    tBB.X += 4 * gui_Scale;
    // draw placeholder
    if (!Enabled && so_len(Text) == 0) {
        gfx_Font_DrawRunes(font, so_string_runes(placeholder), gfx_Rectangle_Position(tBB), gui_Scale, 0, gfx_Gray, false);
    } else {
        // draw holding text
        gfx_Font_DrawRunes(font, runes, gfx_Rectangle_Position(tBB), gui_Scale, 0, gfx_White, false);
    }
    if (Enabled && blink) {
        tBB.X += tBB.W;
        gfx_Font_DrawRunes(font, (so_Slice){(so_rune[1]){U'_'}, 1, 1}, gfx_Rectangle_Position(tBB), gui_Scale, 0, gfx_White, false);
    }
}
