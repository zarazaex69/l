#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include <stdlib.h>
#include "raylib.h"
#include "raygui.h"
#include "font_embed.h"
#include "theme.h"

extern Font g_font;

static inline int *core_build_codepoints(int *count)
{
    int total = 95 + 256;
    int *cp = (int *)malloc(total * sizeof(int));
    int n = 0;
    for (int i = 0x20; i <= 0x7E; i++) cp[n++] = i;
    for (int i = 0x400; i <= 0x4FF; i++) cp[n++] = i;
    *count = n;
    return cp;
}

static inline void core_init(const char *title, int w, int h)
{
    InitWindow(w, h, title);
    SetTargetFPS(60);

    theme_load();
    theme_apply_style();

    int cp_count = 0;
    int *codepoints = core_build_codepoints(&cp_count);
    g_font = LoadFontFromMemory(".ttf", embedded_font_data, embedded_font_size, 32, codepoints, cp_count);
    free(codepoints);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(g_font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
}

static inline void core_close(void)
{
    UnloadFont(g_font);
    CloseWindow();
}

#endif
