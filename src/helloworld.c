#include "core.h"
#include "helloworld.h"

int app_helloworld(int argc, char **argv)
{
    (void)argc; (void)argv;
    core_init("helloworld", 400, 200);

    bool should_close = false;
    while (!WindowShouldClose() && !should_close) {
        BeginDrawing();
        ClearBackground(g_theme.bg);

        DrawTextEx(g_font, "Hello, World!", (Vector2){ 110, 60 }, 24, 1, g_theme.fg);

        if (GuiButton((Rectangle){ 150, 120, 100, 40 }, "OK"))
            should_close = true;

        EndDrawing();
    }

    core_close();
    return 0;
}
