#include <stdio.h>
#include <string.h>
#include "core.h"
#include "themeapp.h"
#include "theme_export.h"

#define ROW_H     56
#define ROW_PAD   10
#define SWATCH_SZ 22
#define SWATCH_GAP 4

static void draw_swatches(float x, float y, const Theme *t)
{
    Color sw[5] = { t->red, t->orange, t->yellow, t->green, t->aqua };
    for (int i = 0; i < 5; i++) {
        float sx = x + i * (SWATCH_SZ + SWATCH_GAP);
        DrawRectangle((int)sx, (int)y, SWATCH_SZ, SWATCH_SZ, sw[i]);
        DrawRectangleLines((int)sx, (int)y, SWATCH_SZ, SWATCH_SZ, t->bg3);
    }
}

int app_theme(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 460;
    int win_h = 80 + builtin_theme_count * (ROW_H + ROW_PAD) + 100;
    core_init("l theme", win_w, win_h);

    int selected = 0;
    for (int i = 0; i < builtin_theme_count; i++) {
        if (strcmp(builtin_themes[i].name, g_theme.name) == 0) {
            selected = i;
            break;
        }
    }

    char status[128] = {0};
    double status_until = 0.0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_DOWN)) {
            selected = (selected + 1) % builtin_theme_count;
        }
        if (IsKeyPressed(KEY_UP)) {
            selected = (selected - 1 + builtin_theme_count) % builtin_theme_count;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (theme_write(&builtin_themes[selected]) == 0) {
                theme_load();
                theme_apply_style();
                theme_export_all(&g_theme);
                theme_reload_system();
                snprintf(status, sizeof(status), "applied: %s", g_theme.name);
            } else {
                snprintf(status, sizeof(status), "error writing theme.conf");
            }
            status_until = GetTime() + 3.0;
        }

        BeginDrawing();
        ClearBackground(g_theme.bg);

        DrawTextEx(g_font, "select theme", (Vector2){ 16, 14 }, 22, 1, g_theme.aqua);
        DrawTextEx(g_font, "current:", (Vector2){ 16, 44 }, 14, 1, g_theme.fg);
        DrawTextEx(g_font, g_theme.name, (Vector2){ 90, 44 }, 14, 1, g_theme.yellow);

        float y = 80.0f;
        for (int i = 0; i < builtin_theme_count; i++) {
            const Theme *t = &builtin_themes[i];
            Rectangle row = { 16, y, (float)(win_w - 32), (float)ROW_H };

            bool is_selected = (i == selected);
            bool is_active   = (strcmp(t->name, g_theme.name) == 0);

            DrawRectangleRec(row, is_selected ? t->bg2 : g_theme.bg1);
            DrawRectangleLinesEx(row, is_selected ? 2 : 1,
                                 is_selected ? t->orange : g_theme.bg3);

            DrawTextEx(g_font, t->name,
                       (Vector2){ row.x + 14, row.y + 8 }, 18, 1,
                       is_selected ? t->fg0 : t->fg);

            const char *tag = is_active ? "[active]" : "";
            DrawTextEx(g_font, tag,
                       (Vector2){ row.x + 14, row.y + 30 }, 12, 1, t->green);

            float sx = row.x + row.width - (SWATCH_SZ + SWATCH_GAP) * 5 - 8;
            float sy = row.y + (ROW_H - SWATCH_SZ) / 2.0f;
            draw_swatches(sx, sy, t);

            if (CheckCollisionPointRec(GetMousePosition(), row)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) selected = i;
            }

            y += ROW_H + ROW_PAD;
        }

        y += 10.0f;
        Rectangle apply_btn = { 16, y, (float)(win_w - 32), 36 };
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   theme_color(g_theme.green));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   theme_color(g_theme.bg));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, theme_color(g_theme.green));
        if (GuiButton(apply_btn, "apply (Enter)")) {
            if (theme_write(&builtin_themes[selected]) == 0) {
                theme_load();
                theme_apply_style();
                theme_export_all(&g_theme);
                theme_reload_system();
                snprintf(status, sizeof(status), "applied: %s", g_theme.name);
            } else {
                snprintf(status, sizeof(status), "error writing theme.conf");
            }
            status_until = GetTime() + 3.0;
        }

        theme_apply_style();

        if (status_until > GetTime() && status[0]) {
            DrawTextEx(g_font, status,
                       (Vector2){ 16, y + 44 }, 14, 1, g_theme.yellow);
        } else {
            DrawTextEx(g_font, theme_config_path(),
                       (Vector2){ 16, y + 44 }, 12, 1, g_theme.bg3);
        }

        EndDrawing();
    }

    core_close();
    return 0;
}
