#include "raylib.h"

// raygui requires implementation in exactly one translation unit
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// gruvbox dark palette
#define GRV_BG      (Color){ 40,  40,  40,  255 }  // #282828
#define GRV_BG1     (Color){ 60,  56,  54,  255 }  // #3c3836
#define GRV_BG2     (Color){ 80,  73,  69,  255 }  // #504945
#define GRV_FG      (Color){ 235, 219, 178, 255 }  // #ebdbb2
#define GRV_FG0     (Color){ 251, 241, 199, 255 }  // #fbf1c7
#define GRV_ORANGE  (Color){ 214, 93,  14,  255 }  // #d65d0e
#define GRV_GREEN   (Color){ 152, 151, 26,  255 }  // #98971a
#define GRV_AQUA    (Color){ 104, 157, 106, 255 }  // #689d6a

// pack color into raygui int format (0xRRGGBBAA)
static int grv_color(Color c)
{
    return ((int)c.r << 24) | ((int)c.g << 16) | ((int)c.b << 8) | (int)c.a;
}

// apply gruvbox theme to raygui
static void apply_gruvbox_style(void)
{
    // global defaults
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR,  grv_color(GRV_BG1));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,  grv_color(GRV_FG));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,  grv_color(GRV_BG2));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, grv_color(GRV_BG2));

    // focused
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,  grv_color(GRV_FG0));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,  grv_color(GRV_ORANGE));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, grv_color(GRV_ORANGE));

    // pressed
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,  grv_color(GRV_BG));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,  grv_color(GRV_GREEN));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, grv_color(GRV_GREEN));

    // button specifics
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_BG2));
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,    grv_color(GRV_FG));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,  grv_color(GRV_AQUA));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,   grv_color(GRV_ORANGE));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,    grv_color(GRV_FG0));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,    grv_color(GRV_GREEN));
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,    grv_color(GRV_BG));
}

int main(void)
{
    // init window
    InitWindow(400, 200, "hello world");
    SetTargetFPS(60);

    apply_gruvbox_style();

    bool should_close = false;

    while (!WindowShouldClose() && !should_close) {
        BeginDrawing();
        ClearBackground(GRV_BG);

        // centered label
        DrawText("Hello, World!", 120, 60, 20, GRV_FG);

        // ok button -- closes the window when pressed
        if (GuiButton((Rectangle){ 150, 120, 100, 40 }, "OK")) {
            should_close = true;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
