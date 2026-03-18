#include "raylib.h"

// raygui requires implementation in exactly one translation unit
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main(void)
{
    // init window
    InitWindow(400, 200, "hello world");
    SetTargetFPS(60);

    bool should_close = false;

    while (!WindowShouldClose() && !should_close) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // centered label
        DrawText("Hello, World!", 120, 60, 20, DARKGRAY);

        // ok button -- closes the window when pressed
        if (GuiButton((Rectangle){ 150, 120, 100, 40 }, "OK")) {
            should_close = true;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
