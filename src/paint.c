#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "core.h"
#include "paint.h"

typedef enum {
    TOOL_BRUSH,
    TOOL_ERASER,
} PaintTool;

#define PAINT_PALETTE_SIZE 12

static bool color_eq(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

int app_paint(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 900;
    int win_h = 600;
    core_init("l paint", win_w, win_h);

    int toolbar_h  = 60;
    int palette_w  = 110;

    Rectangle canvas_area = {
        (float)palette_w,
        (float)toolbar_h,
        (float)(win_w - palette_w),
        (float)(win_h - toolbar_h),
    };

    // canvas backing surface
    RenderTexture2D canvas = LoadRenderTexture((int)canvas_area.width, (int)canvas_area.height);
    Color paper = GRV_FG0;

    BeginTextureMode(canvas);
    ClearBackground(paper);
    EndTextureMode();

    Color palette[PAINT_PALETTE_SIZE] = {
        GRV_BG,    GRV_BG1,   GRV_BG2,   GRV_BG3,
        GRV_FG,    GRV_FG0,   GRV_RED,   GRV_ORANGE,
        GRV_YELLOW, GRV_GREEN, GRV_AQUA, (Color){ 0, 0, 0, 255 },
    };

    Color current_color = GRV_BG;
    PaintTool tool      = TOOL_BRUSH;
    float brush_size    = 4.0f;

    Vector2 last_canvas = { 0, 0 };
    bool was_drawing    = false;

    char status_msg[128] = "";
    double status_until  = 0.0;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // keyboard shortcuts
        if (IsKeyPressed(KEY_B)) tool = TOOL_BRUSH;
        if (IsKeyPressed(KEY_E)) tool = TOOL_ERASER;
        if (IsKeyPressed(KEY_C)) {
            BeginTextureMode(canvas);
            ClearBackground(paper);
            EndTextureMode();
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET))  { brush_size -= 1.0f; if (brush_size < 1.0f) brush_size = 1.0f; }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) { brush_size += 1.0f; if (brush_size > 60.0f) brush_size = 60.0f; }

        // canvas drawing
        bool in_canvas = CheckCollisionPointRec(mouse, canvas_area);
        Vector2 cp = {
            mouse.x - canvas_area.x,
            mouse.y - canvas_area.y,
        };

        if (in_canvas && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Color c = (tool == TOOL_ERASER) ? paper : current_color;
            BeginTextureMode(canvas);
            if (was_drawing) {
                // smooth stroke between samples
                DrawLineEx(last_canvas, cp, brush_size * 2.0f, c);
            }
            DrawCircleV(cp, brush_size, c);
            EndTextureMode();
            was_drawing = true;
            last_canvas = cp;
        } else {
            was_drawing = false;
        }

        BeginDrawing();
        ClearBackground(GRV_BG);

        // ---- toolbar ----
        DrawRectangle(0, 0, win_w, toolbar_h, GRV_BG1);
        DrawLineEx((Vector2){ 0, (float)toolbar_h }, (Vector2){ (float)win_w, (float)toolbar_h }, 1, GRV_BG3);

        // tool buttons (highlight active)
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,
            grv_color(tool == TOOL_BRUSH ? GRV_GREEN : GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,
            grv_color(tool == TOOL_BRUSH ? GRV_BG : GRV_FG));
        if (GuiButton((Rectangle){ 10, 12, 80, 36 }, "brush"))  tool = TOOL_BRUSH;

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,
            grv_color(tool == TOOL_ERASER ? GRV_GREEN : GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,
            grv_color(tool == TOOL_ERASER ? GRV_BG : GRV_FG));
        if (GuiButton((Rectangle){ 100, 12, 80, 36 }, "eraser")) tool = TOOL_ERASER;

        apply_gruvbox_style();

        // clear button (red)
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_RED));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_FG0));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_RED));
        if (GuiButton((Rectangle){ 190, 12, 80, 36 }, "clear")) {
            BeginTextureMode(canvas);
            ClearBackground(paper);
            EndTextureMode();
        }

        // save button (aqua)
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_AQUA));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_AQUA));
        if (GuiButton((Rectangle){ 280, 12, 80, 36 }, "save")) {
            Image img = LoadImageFromTexture(canvas.texture);
            ImageFlipVertical(&img);
            char filename[64];
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            strftime(filename, sizeof(filename), "paint-%Y%m%d-%H%M%S.png", t);
            if (ExportImage(img, filename)) {
                snprintf(status_msg, sizeof(status_msg), "saved: %s", filename);
            } else {
                snprintf(status_msg, sizeof(status_msg), "save failed");
            }
            status_until = GetTime() + 3.0;
            UnloadImage(img);
        }

        apply_gruvbox_style();

        // brush size slider
        char size_label[16];
        snprintf(size_label, sizeof(size_label), "%d", (int)brush_size);
        DrawTextEx(g_font, "size", (Vector2){ 380, 22 }, 16, 1, GRV_FG);
        GuiSlider((Rectangle){ 420, 18, 160, 24 }, NULL, size_label, &brush_size, 1.0f, 60.0f);

        // status / hint
        const char *hint = "B brush   E eraser   C clear   [ ] size";
        if (status_until > GetTime() && status_msg[0]) {
            DrawTextEx(g_font, status_msg, (Vector2){ 600, 22 }, 16, 1, GRV_YELLOW);
        } else {
            DrawTextEx(g_font, hint, (Vector2){ 600, 22 }, 14, 1, GRV_BG3);
        }

        // ---- color palette (left side) ----
        DrawRectangle(0, toolbar_h, palette_w, win_h - toolbar_h, GRV_BG1);
        DrawLineEx((Vector2){ (float)palette_w, (float)toolbar_h },
                   (Vector2){ (float)palette_w, (float)win_h }, 1, GRV_BG3);

        int sw   = 40;
        int ppad = 10;
        int cols = 2;
        for (int i = 0; i < PAINT_PALETTE_SIZE; i++) {
            int row = i / cols;
            int col = i % cols;
            float x = (float)(ppad + col * (sw + ppad));
            float y = (float)(toolbar_h + ppad + row * (sw + ppad));
            Rectangle r = { x, y, (float)sw, (float)sw };

            DrawRectangleRec(r, palette[i]);

            bool selected = (tool == TOOL_BRUSH) && color_eq(palette[i], current_color);
            if (selected) {
                DrawRectangleLinesEx(r, 3, GRV_YELLOW);
            } else {
                DrawRectangleLinesEx(r, 1, GRV_BG3);
            }

            if (CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                current_color = palette[i];
                tool = TOOL_BRUSH;
            }
        }

        // ---- canvas ----
        DrawTextureRec(
            canvas.texture,
            (Rectangle){ 0, 0, canvas_area.width, -canvas_area.height },
            (Vector2){ canvas_area.x, canvas_area.y },
            WHITE);
        DrawRectangleLinesEx(canvas_area, 1, GRV_BG3);

        // brush cursor preview
        if (in_canvas) {
            Color preview = (tool == TOOL_ERASER) ? paper : current_color;
            DrawCircleLines((int)mouse.x, (int)mouse.y, brush_size, GRV_BG);
            DrawCircleLines((int)mouse.x, (int)mouse.y, brush_size + 1, preview);
        }

        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    core_close();
    return 0;
}
