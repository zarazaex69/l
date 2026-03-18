#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include "raylib.h"
#include "raygui.h"

// gruvbox dark palette
#define GRV_BG      (Color){ 40,  40,  40,  255 }
#define GRV_BG1     (Color){ 60,  56,  54,  255 }
#define GRV_BG2     (Color){ 80,  73,  69,  255 }
#define GRV_BG3     (Color){ 102, 92,  84,  255 }
#define GRV_FG      (Color){ 235, 219, 178, 255 }
#define GRV_FG0     (Color){ 251, 241, 199, 255 }
#define GRV_RED     (Color){ 204, 36,  29,  255 }
#define GRV_ORANGE  (Color){ 214, 93,  14,  255 }
#define GRV_GREEN   (Color){ 152, 151, 26,  255 }
#define GRV_AQUA    (Color){ 104, 157, 106, 255 }
#define GRV_YELLOW  (Color){ 215, 153, 33,  255 }

// pack color into raygui int format
static inline int grv_color(Color c)
{
    return ((int)c.r << 24) | ((int)c.g << 16) | ((int)c.b << 8) | (int)c.a;
}

// apply gruvbox theme to raygui
static inline void apply_gruvbox_style(void)
{
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR,    grv_color(GRV_BG1));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,    grv_color(GRV_FG));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,    grv_color(GRV_BG2));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL,  grv_color(GRV_BG2));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,    grv_color(GRV_FG0));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,    grv_color(GRV_ORANGE));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED,  grv_color(GRV_ORANGE));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,    grv_color(GRV_BG));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,    grv_color(GRV_GREEN));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED,  grv_color(GRV_GREEN));

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,     grv_color(GRV_BG2));
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,      grv_color(GRV_FG));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,    grv_color(GRV_AQUA));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,     grv_color(GRV_ORANGE));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,      grv_color(GRV_FG0));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,      grv_color(GRV_GREEN));
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,      grv_color(GRV_BG));
}

// shared font handle
extern Font g_font;

// init raylib window + gruvbox + font
static inline void core_init(const char *title, int w, int h)
{
    InitWindow(w, h, title);
    SetTargetFPS(60);
    apply_gruvbox_style();
    // load at larger size for clean downscaling
    g_font = LoadFontEx("fonts/JetBrainsMono-Regular.ttf", 32, NULL, 0);
    // bilinear filter for smooth scaling
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(g_font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
}

// cleanup
static inline void core_close(void)
{
    UnloadFont(g_font);
    CloseWindow();
}

#endif
