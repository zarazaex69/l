#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include "core.h"
#include "rlgl.h"
#include "paint.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    TOOL_BRUSH,
    TOOL_ERASER,
} PaintTool;

#define PAINT_PALETTE_SIZE 12

typedef struct {
    float r, g, b;
} PColor;

static bool color_eq(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// ---- wallgen helpers (mirror wallsee/wallcreate) ----

static PColor pcolor_lerp(PColor a, PColor b, float t)
{
    return (PColor){
        a.r + t * (b.r - a.r),
        a.g + t * (b.g - a.g),
        a.b + t * (b.b - a.b),
    };
}

static void paint_time_colors(float time_val, PColor *top, PColor *bot)
{
    PColor c_night_t = { 10,  10,  25  };
    PColor c_night_b = { 5,   5,   15  };
    PColor c_dawn_t  = { 200, 100, 80  };
    PColor c_dawn_b  = { 100, 50,  60  };
    PColor c_noon_t  = { 100, 200, 255 };
    PColor c_noon_b  = { 180, 230, 255 };
    PColor c_dusk_t  = { 80,  40,  100 };
    PColor c_dusk_b  = { 250, 120, 80  };

    if (time_val < 6.0f) {
        float t = time_val / 6.0f;
        *top = pcolor_lerp(c_night_t, c_dawn_t, t);
        *bot = pcolor_lerp(c_night_b, c_dawn_b, t);
    } else if (time_val < 12.0f) {
        float t = (time_val - 6.0f) / 6.0f;
        *top = pcolor_lerp(c_dawn_t, c_noon_t, t);
        *bot = pcolor_lerp(c_dawn_b, c_noon_b, t);
    } else if (time_val < 18.0f) {
        float t = (time_val - 12.0f) / 6.0f;
        *top = pcolor_lerp(c_noon_t, c_dusk_t, t);
        *bot = pcolor_lerp(c_noon_b, c_dusk_b, t);
    } else {
        float t = (time_val - 18.0f) / 6.0f;
        *top = pcolor_lerp(c_dusk_t, c_night_t, t);
        *bot = pcolor_lerp(c_dusk_b, c_night_b, t);
    }
}

static inline unsigned char clamp_u8(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

// generate gradient into rgb buffer (3 bytes per pixel)
static void paint_gen_gradient(unsigned char *buf, int w, int h,
                               float time_val, int grain, float angle_deg)
{
    srand(1337);

    PColor top, bot;
    paint_time_colors(time_val, &top, &bot);

    float angle_rad = angle_deg * (float)M_PI / 180.0f;
    float dx = cosf(angle_rad);
    float dy = sinf(angle_rad);

    float pw0 = w * dx;
    float p0h = h * dy;
    float pwh = w * dx + h * dy;

    float pmin = 0, pmax = 0;
    if (pw0 < pmin) pmin = pw0;
    if (p0h < pmin) pmin = p0h;
    if (pwh < pmin) pmin = pwh;
    if (pw0 > pmax) pmax = pw0;
    if (p0h > pmax) pmax = p0h;
    if (pwh > pmax) pmax = pwh;

    float prange = pmax - pmin;
    if (prange < 0.0001f) prange = 1.0f;

    int idx = 0;
    for (int y = 0; y < h; y++) {
        float yp = y * dy;
        for (int x = 0; x < w; x++) {
            float pv = x * dx + yp;
            float t = (pv - pmin) / prange;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float cr = top.r + t * (bot.r - top.r);
            float cg = top.g + t * (bot.g - top.g);
            float cb = top.b + t * (bot.b - top.b);

            int noise = (grain > 0) ? (rand() % (grain * 2 + 1)) - grain : 0;

            buf[idx++] = clamp_u8((int)cr + noise);
            buf[idx++] = clamp_u8((int)cg + noise);
            buf[idx++] = clamp_u8((int)cb + noise);
        }
    }
}

// fill rgb buffer with single color
static void paint_fill_solid(unsigned char *buf, int w, int h, Color c)
{
    int n = w * h;
    for (int i = 0; i < n; i++) {
        buf[i * 3 + 0] = c.r;
        buf[i * 3 + 1] = c.g;
        buf[i * 3 + 2] = c.b;
    }
}

// ensure ~/Wallpapers/ exists
static void paint_ensure_wallpapers_dir(char *out, int outsz)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, outsz, "%s/Wallpapers", home);
    mkdir(out, 0755);
}

// create ~/Wallpapers/wallsee.lock (same convention as wallsee)
static bool paint_lock_create(void)
{
    char dir[512];
    paint_ensure_wallpapers_dir(dir, sizeof(dir));
    char path[640];
    snprintf(path, sizeof(path), "%s/wallsee.lock", dir);
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "locked by paint\n");
    fclose(f);
    return true;
}

// render gradient at WxH and composite the user's drawing scaled to fit,
// then export as PNG. returns true on success.
static bool paint_export_wallpaper(const char *path,
                                   RenderTexture2D draw_tex, int cw, int ch,
                                   float wg_time, float wg_grain, float wg_angle,
                                   int W, int H)
{
    unsigned char *wbuf = malloc(W * H * 3);
    if (!wbuf) return false;
    paint_gen_gradient(wbuf, W, H, wg_time, (int)wg_grain, wg_angle);

    Image wimg = { 0 };
    wimg.data    = wbuf;
    wimg.width   = W;
    wimg.height  = H;
    wimg.mipmaps = 1;
    wimg.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8;

    Texture2D wtex = LoadTextureFromImage(wimg);
    free(wbuf);

    RenderTexture2D out = LoadRenderTexture(W, H);
    BeginTextureMode(out);
    ClearBackground(WHITE);
    DrawTexture(wtex, 0, 0, WHITE);
    // scale user drawing to wallpaper size
    DrawTexturePro(draw_tex.texture,
                   (Rectangle){ 0, 0, (float)cw, -(float)ch },
                   (Rectangle){ 0, 0, (float)W,  (float)H },
                   (Vector2){ 0, 0 }, 0, WHITE);
    EndTextureMode();

    Image final = LoadImageFromTexture(out.texture);
    ImageFlipVertical(&final);
    bool ok = ExportImage(final, path);

    UnloadImage(final);
    UnloadRenderTexture(out);
    UnloadTexture(wtex);
    return ok;
}

int app_paint(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 1100;
    int win_h = 620;
    core_init("l paint", win_w, win_h);

    int toolbar_h = 56;
    int palette_w = 100;
    int wallgen_w = 220;

    Rectangle canvas_area = {
        (float)palette_w,
        (float)toolbar_h,
        (float)(win_w - palette_w - wallgen_w),
        (float)(win_h - toolbar_h),
    };

    int cw = (int)canvas_area.width;
    int ch = (int)canvas_area.height;

    // --- background layer (Texture2D updated via UpdateTexture) ---
    Color paper = GRV_FG0;
    unsigned char *bg_buf = malloc(cw * ch * 3);
    paint_fill_solid(bg_buf, cw, ch, paper);

    Image bg_img = { 0 };
    bg_img.data    = bg_buf;
    bg_img.width   = cw;
    bg_img.height  = ch;
    bg_img.mipmaps = 1;
    bg_img.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    Texture2D bg_tex = LoadTextureFromImage(bg_img);

    // --- drawing layer (transparent RGBA) ---
    RenderTexture2D draw_tex = LoadRenderTexture(cw, ch);
    BeginTextureMode(draw_tex);
    ClearBackground(BLANK);
    EndTextureMode();

    // palette
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

    // wallgen state
    bool wallgen_live = true;
    float wg_time  = 12.0f;
    float wg_grain = 5.0f;
    float wg_angle = 90.0f;
    float wg_prev_time  = -1.0f;
    float wg_prev_grain = -1.0f;
    float wg_prev_angle = -1.0f;
    bool  wallgen_active = false; // becomes true after first fill

    char status_msg[256] = "";
    double status_until  = 0.0;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // ---- shortcuts ----
        if (IsKeyPressed(KEY_B)) tool = TOOL_BRUSH;
        if (IsKeyPressed(KEY_E)) tool = TOOL_ERASER;
        if (IsKeyPressed(KEY_C)) {
            BeginTextureMode(draw_tex);
            ClearBackground(BLANK);
            EndTextureMode();
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET))  { brush_size -= 1.0f; if (brush_size < 1.0f) brush_size = 1.0f; }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) { brush_size += 1.0f; if (brush_size > 60.0f) brush_size = 60.0f; }

        // ---- wallgen live update ----
        if (wallgen_live && wallgen_active &&
            (wg_time != wg_prev_time || wg_grain != wg_prev_grain || wg_angle != wg_prev_angle)) {
            paint_gen_gradient(bg_buf, cw, ch, wg_time, (int)wg_grain, wg_angle);
            UpdateTexture(bg_tex, bg_buf);
            wg_prev_time  = wg_time;
            wg_prev_grain = wg_grain;
            wg_prev_angle = wg_angle;
        }

        // ---- canvas drawing ----
        bool in_canvas = CheckCollisionPointRec(mouse, canvas_area);
        Vector2 cp = {
            mouse.x - canvas_area.x,
            mouse.y - canvas_area.y,
        };

        if (in_canvas && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            BeginTextureMode(draw_tex);
            if (tool == TOOL_ERASER) {
                // punch transparent holes through draw layer
                BeginBlendMode(BLEND_CUSTOM_SEPARATE);
                rlSetBlendFactorsSeparate(RL_ZERO, RL_ZERO, RL_ZERO, RL_ZERO,
                                          RL_FUNC_ADD, RL_FUNC_ADD);
                if (was_drawing) DrawLineEx(last_canvas, cp, brush_size * 2.0f, WHITE);
                DrawCircleV(cp, brush_size, WHITE);
                EndBlendMode();
            } else {
                if (was_drawing) DrawLineEx(last_canvas, cp, brush_size * 2.0f, current_color);
                DrawCircleV(cp, brush_size, current_color);
            }
            EndTextureMode();
            was_drawing = true;
            last_canvas = cp;
        } else {
            was_drawing = false;
        }

        // ============ render ============
        BeginDrawing();
        ClearBackground(GRV_BG);

        // ---- toolbar ----
        DrawRectangle(0, 0, win_w, toolbar_h, GRV_BG1);
        DrawLineEx((Vector2){ 0, (float)toolbar_h }, (Vector2){ (float)win_w, (float)toolbar_h }, 1, GRV_BG3);

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,
            grv_color(tool == TOOL_BRUSH ? GRV_GREEN : GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,
            grv_color(tool == TOOL_BRUSH ? GRV_BG : GRV_FG));
        if (GuiButton((Rectangle){ 10, 12, 80, 32 }, "brush"))  tool = TOOL_BRUSH;

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,
            grv_color(tool == TOOL_ERASER ? GRV_GREEN : GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,
            grv_color(tool == TOOL_ERASER ? GRV_BG : GRV_FG));
        if (GuiButton((Rectangle){ 100, 12, 80, 32 }, "eraser")) tool = TOOL_ERASER;

        apply_gruvbox_style();

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_RED));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_FG0));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_RED));
        if (GuiButton((Rectangle){ 190, 12, 80, 32 }, "clear")) {
            BeginTextureMode(draw_tex);
            ClearBackground(BLANK);
            EndTextureMode();
        }

        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_AQUA));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_AQUA));
        if (GuiButton((Rectangle){ 280, 12, 80, 32 }, "save")) {
            // composite bg + draw into a temporary render texture, then export
            RenderTexture2D out = LoadRenderTexture(cw, ch);
            BeginTextureMode(out);
            ClearBackground(WHITE);
            DrawTexture(bg_tex, 0, 0, WHITE);
            DrawTextureRec(draw_tex.texture,
                           (Rectangle){ 0, 0, (float)cw, -(float)ch },
                           (Vector2){ 0, 0 }, WHITE);
            EndTextureMode();

            Image img = LoadImageFromTexture(out.texture);
            ImageFlipVertical(&img);

            char filename[64];
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            strftime(filename, sizeof(filename), "paint-%Y%m%d-%H%M%S.png", t);
            if (ExportImage(img, filename))
                snprintf(status_msg, sizeof(status_msg), "saved: %s", filename);
            else
                snprintf(status_msg, sizeof(status_msg), "save failed");
            status_until = GetTime() + 3.0;

            UnloadImage(img);
            UnloadRenderTexture(out);
        }

        apply_gruvbox_style();

        // brush size
        char size_label[16];
        snprintf(size_label, sizeof(size_label), "%d", (int)brush_size);
        DrawTextEx(g_font, "size", (Vector2){ 380, 20 }, 16, 1, GRV_FG);
        GuiSlider((Rectangle){ 420, 18, 140, 22 }, NULL, size_label, &brush_size, 1.0f, 60.0f);

        // hint / status
        if (status_until > GetTime() && status_msg[0]) {
            DrawTextEx(g_font, status_msg, (Vector2){ 580, 20 }, 16, 1, GRV_YELLOW);
        } else {
            const char *hint = "B brush  E eraser  C clear  [ ] size";
            DrawTextEx(g_font, hint, (Vector2){ 580, 22 }, 13, 1, GRV_BG3);
        }

        // ---- left palette ----
        DrawRectangle(0, toolbar_h, palette_w, win_h - toolbar_h, GRV_BG1);
        DrawLineEx((Vector2){ (float)palette_w, (float)toolbar_h },
                   (Vector2){ (float)palette_w, (float)win_h }, 1, GRV_BG3);

        int sw   = 36;
        int ppad = 10;
        int pcols = 2;
        for (int i = 0; i < PAINT_PALETTE_SIZE; i++) {
            int row = i / pcols;
            int col = i % pcols;
            float x = (float)(ppad + col * (sw + ppad));
            float y = (float)(toolbar_h + ppad + row * (sw + ppad));
            Rectangle r = { x, y, (float)sw, (float)sw };

            DrawRectangleRec(r, palette[i]);
            bool selected = (tool == TOOL_BRUSH) && color_eq(palette[i], current_color);
            DrawRectangleLinesEx(r, selected ? 3 : 1, selected ? GRV_YELLOW : GRV_BG3);

            if (CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                current_color = palette[i];
                tool = TOOL_BRUSH;
            }
        }

        // ---- canvas ----
        DrawTexture(bg_tex, (int)canvas_area.x, (int)canvas_area.y, WHITE);
        DrawTextureRec(
            draw_tex.texture,
            (Rectangle){ 0, 0, canvas_area.width, -canvas_area.height },
            (Vector2){ canvas_area.x, canvas_area.y },
            WHITE);
        DrawRectangleLinesEx(canvas_area, 1, GRV_BG3);

        // brush cursor preview
        if (in_canvas) {
            Color preview = (tool == TOOL_ERASER) ? (Color){ 200, 200, 200, 200 } : current_color;
            DrawCircleLines((int)mouse.x, (int)mouse.y, brush_size, GRV_BG);
            DrawCircleLines((int)mouse.x, (int)mouse.y, brush_size + 1, preview);
        }

        // ---- right wallgen panel ----
        int px = win_w - wallgen_w;
        DrawRectangle(px, toolbar_h, wallgen_w, win_h - toolbar_h, GRV_BG1);
        DrawLineEx((Vector2){ (float)px, (float)toolbar_h },
                   (Vector2){ (float)px, (float)win_h }, 1, GRV_BG3);

        DrawTextEx(g_font, "wallgen", (Vector2){ (float)(px + 12), (float)(toolbar_h + 10) }, 18, 1, GRV_AQUA);

        float wy = (float)(toolbar_h + 44);
        char buf[32];

        DrawTextEx(g_font, "time", (Vector2){ (float)(px + 12), wy }, 14, 1, GRV_FG);
        snprintf(buf, sizeof(buf), "%.1f", wg_time);
        DrawTextEx(g_font, buf, (Vector2){ (float)(px + wallgen_w - 50), wy }, 14, 1, GRV_YELLOW);
        wy += 18;
        GuiSliderBar((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 18 },
                     NULL, NULL, &wg_time, 0.0f, 24.0f);
        wy += 28;

        DrawTextEx(g_font, "grain", (Vector2){ (float)(px + 12), wy }, 14, 1, GRV_FG);
        snprintf(buf, sizeof(buf), "%d", (int)wg_grain);
        DrawTextEx(g_font, buf, (Vector2){ (float)(px + wallgen_w - 50), wy }, 14, 1, GRV_YELLOW);
        wy += 18;
        GuiSliderBar((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 18 },
                     NULL, NULL, &wg_grain, 0.0f, 30.0f);
        wy += 28;

        DrawTextEx(g_font, "angle", (Vector2){ (float)(px + 12), wy }, 14, 1, GRV_FG);
        snprintf(buf, sizeof(buf), "%.0f", wg_angle);
        DrawTextEx(g_font, buf, (Vector2){ (float)(px + wallgen_w - 50), wy }, 14, 1, GRV_YELLOW);
        wy += 18;
        GuiSliderBar((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 18 },
                     NULL, NULL, &wg_angle, 0.0f, 360.0f);
        wy += 32;

        GuiCheckBox((Rectangle){ (float)(px + 12), wy, 18, 18 }, "live preview", &wallgen_live);
        wy += 30;

        // fill button
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_GREEN));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_GREEN));
        if (GuiButton((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 32 }, "fill canvas")) {
            paint_gen_gradient(bg_buf, cw, ch, wg_time, (int)wg_grain, wg_angle);
            UpdateTexture(bg_tex, bg_buf);
            wallgen_active = true;
            wg_prev_time = wg_time; wg_prev_grain = wg_grain; wg_prev_angle = wg_angle;
        }
        wy += 40;

        // reset to paper
        apply_gruvbox_style();
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_BG2));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_FG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_AQUA));
        if (GuiButton((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 28 }, "reset to paper")) {
            paint_fill_solid(bg_buf, cw, ch, paper);
            UpdateTexture(bg_tex, bg_buf);
            wallgen_active = false;
        }
        wy += 36;

        // export wallpaper at 1920x1080
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_ORANGE));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_ORANGE));
        if (GuiButton((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 32 }, "save wallpaper")) {
            char dir[512];
            paint_ensure_wallpapers_dir(dir, sizeof(dir));
            char path[640];
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y%m%d-%H%M%S", t);
            snprintf(path, sizeof(path), "%s/paint-wall-%s.png", dir, ts);

            if (paint_export_wallpaper(path, draw_tex, cw, ch,
                                       wg_time, wg_grain, wg_angle, 1920, 1080))
                snprintf(status_msg, sizeof(status_msg), "wall saved: %s", path);
            else
                snprintf(status_msg, sizeof(status_msg), "wall save failed");
            status_until = GetTime() + 4.0;
        }
        wy += 40;

        // apply as wallpaper via swaymsg + lock
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,   grv_color(GRV_YELLOW));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,   grv_color(GRV_BG));
        GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_YELLOW));
        if (GuiButton((Rectangle){ (float)(px + 12), wy, (float)(wallgen_w - 24), 32 }, "set wallpaper")) {
            char dir[512];
            paint_ensure_wallpapers_dir(dir, sizeof(dir));
            char path[640];
            snprintf(path, sizeof(path), "%s/paint-wall.png", dir);

            if (paint_export_wallpaper(path, draw_tex, cw, ch,
                                       wg_time, wg_grain, wg_angle, 1920, 1080)) {
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "swaymsg 'output * bg %s fill'", path);
                int rc = system(cmd);
                paint_lock_create();
                if (rc == 0)
                    snprintf(status_msg, sizeof(status_msg), "wallpaper applied + locked");
                else
                    snprintf(status_msg, sizeof(status_msg), "saved, swaymsg failed");
            } else {
                snprintf(status_msg, sizeof(status_msg), "set wallpaper failed");
            }
            status_until = GetTime() + 4.0;
        }

        apply_gruvbox_style();

        EndDrawing();
    }

    free(bg_buf);
    UnloadTexture(bg_tex);
    UnloadRenderTexture(draw_tex);
    core_close();
    return 0;
}
