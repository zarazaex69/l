#include <stdlib.h>
#include "core.h"
#include "menu.h"

#define BTN_W  220
#define BTN_H  48
#define PAD    16

int app_menu(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 260;
    int win_h = 260;
    core_init("l menu", win_w, win_h);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GRV_BG);

        float x = (float)(win_w - BTN_W) / 2;
        float y = 20.0f;

        DrawTextEx(g_font, "power menu", (Vector2){ x + 40, y }, 20, 1, GRV_AQUA);
        y += 36.0f;

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_RED));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,    grv_color(GRV_FG0));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,  grv_color(GRV_RED));
        if (GuiButton((Rectangle){ x, y, BTN_W, BTN_H }, "Shutdown")) {
            core_close();
            system("systemctl poweroff");
            return 0;
        }
        y += BTN_H + PAD;

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_ORANGE));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,    grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,  grv_color(GRV_ORANGE));
        if (GuiButton((Rectangle){ x, y, BTN_W, BTN_H }, "Reboot")) {
            core_close();
            system("systemctl reboot");
            return 0;
        }
        y += BTN_H + PAD;

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,    grv_color(GRV_FG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,  grv_color(GRV_AQUA));
        if (GuiButton((Rectangle){ x, y, BTN_W, BTN_H }, "Exit")) {
            break;
        }

        EndDrawing();
    }

    core_close();
    return 0;
}
