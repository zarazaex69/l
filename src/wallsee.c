#include "core.h"
#include "wallsee.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// preview dimensions (scaled down for realtime)
#define PREV_W 384
#define PREV_H 216

// wallpaper output dimensions
#define WALL_W 1920
#define WALL_H 1080

#define LOCK_FILENAME "wallsee.lock"

typedef struct {
    float r, g, b;
} WsColor;

// status message
static char ws_status[256] = {0};
static int ws_status_timer = 0;

static void ws_set_status(const char *msg)
{
    strncpy(ws_status, msg, sizeof(ws_status) - 1);
    ws_status[sizeof(ws_status) - 1] = '\0';
    ws_status_timer = 180;
}

// build path inside ~/Wallpapers/
static void ws_wallpapers_path(char *out, int size, const char *filename)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, size, "%s/Wallpapers/%s", home, filename);
}

// build lock file path
static void ws_lock_path(char *out, int size)
{
    ws_wallpapers_path(out, size, LOCK_FILENAME);
}

// check if lock file exists
static bool ws_lock_exists(void)
{
    char path[512];
    ws_lock_path(path, sizeof(path));
    return access(path, F_OK) == 0;
}

// create lock file
static bool ws_lock_create(void)
{
    char path[512];
    ws_lock_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "locked by wallsee\n");
    fclose(f);
    return true;
}

// remove lock file
static bool ws_lock_remove(void)
{
    char path[512];
    ws_lock_path(path, sizeof(path));
    return remove(path) == 0;
}

// interpolate between two colors
static WsColor ws_lerp(WsColor c1, WsColor c2, float t)
{
    return (WsColor){
        c1.r + t * (c2.r - c1.r),
        c1.g + t * (c2.g - c1.g),
        c1.b + t * (c2.b - c1.b),
    };
}

// pick gradient colors based on time of day (same logic as wallcreate)
static void ws_time_colors(float time_val, WsColor *top, WsColor *bottom)
{
    WsColor c_night_t = { 10.0f,  10.0f,  25.0f  };
    WsColor c_night_b = { 5.0f,   5.0f,   15.0f  };
    WsColor c_dawn_t  = { 200.0f, 100.0f, 80.0f  };
    WsColor c_dawn_b  = { 100.0f, 50.0f,  60.0f  };
    WsColor c_noon_t  = { 100.0f, 200.0f, 255.0f };
    WsColor c_noon_b  = { 180.0f, 230.0f, 255.0f };
    WsColor c_dusk_t  = { 80.0f,  40.0f,  100.0f };
    WsColor c_dusk_b  = { 250.0f, 120.0f, 80.0f  };

    if (time_val < 6.0f) {
        float t = time_val / 6.0f;
        *top    = ws_lerp(c_night_t, c_dawn_t, t);
        *bottom = ws_lerp(c_night_b, c_dawn_b, t);
    } else if (time_val < 12.0f) {
        float t = (time_val - 6.0f) / 6.0f;
        *top    = ws_lerp(c_dawn_t, c_noon_t, t);
        *bottom = ws_lerp(c_dawn_b, c_noon_b, t);
    } else if (time_val < 18.0f) {
        float t = (time_val - 12.0f) / 6.0f;
        *top    = ws_lerp(c_noon_t, c_dusk_t, t);
        *bottom = ws_lerp(c_noon_b, c_dusk_b, t);
    } else {
        float t = (time_val - 18.0f) / 6.0f;
        *top    = ws_lerp(c_dusk_t, c_night_t, t);
        *bottom = ws_lerp(c_dusk_b, c_night_b, t);
    }
}

static inline unsigned char ws_clamp(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

// generate gradient into pixel buffer (rgb, 3 bytes per pixel)
static void ws_generate(unsigned char *buf, int w, int h,
                        float time_val, int grain, float angle_deg)
{
    srand(1337);

    WsColor top, bottom;
    ws_time_colors(time_val, &top, &bottom);

    float angle_rad = angle_deg * (float)M_PI / 180.0f;
    float dx = cosf(angle_rad);
    float dy = sinf(angle_rad);

    // compute projection range
    float pw0 = w * dx;
    float p0h = h * dy;
    float pwh = w * dx + h * dy;

    float proj_min = 0, proj_max = 0;
    if (pw0 < proj_min) proj_min = pw0;
    if (p0h < proj_min) proj_min = p0h;
    if (pwh < proj_min) proj_min = pwh;
    if (pw0 > proj_max) proj_max = pw0;
    if (p0h > proj_max) proj_max = p0h;
    if (pwh > proj_max) proj_max = pwh;

    float proj_range = proj_max - proj_min;
    if (proj_range < 0.0001f) proj_range = 1.0f;

    int idx = 0;
    for (int y = 0; y < h; y++) {
        float y_proj = y * dy;
        for (int x = 0; x < w; x++) {
            float proj_val = x * dx + y_proj;
            float t = (proj_val - proj_min) / proj_range;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float cr = top.r + t * (bottom.r - top.r);
            float cg = top.g + t * (bottom.g - top.g);
            float cb = top.b + t * (bottom.b - top.b);

            int noise = (grain > 0)
                ? (rand() % (grain * 2 + 1)) - grain
                : 0;

            buf[idx++] = ws_clamp((int)cr + noise);
            buf[idx++] = ws_clamp((int)cg + noise);
            buf[idx++] = ws_clamp((int)cb + noise);
        }
    }
}

// write ppm file to disk
static bool ws_write_ppm(const char *path, const unsigned char *buf, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    fwrite(buf, 1, w * h * 3, f);
    fclose(f);
    return true;
}

// ensure ~/Wallpapers/ directory exists
static void ws_ensure_dir(void)
{
    char dir[512];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(dir, sizeof(dir), "%s/Wallpapers", home);
    mkdir(dir, 0755);
}

// save wallpaper ppm to ~/Wallpapers/wallsee.ppm
static bool ws_save_wallpaper(float time_val, int grain, float angle_deg)
{
    ws_ensure_dir();

    unsigned char *buf = malloc(WALL_W * WALL_H * 3);
    if (!buf) return false;

    ws_generate(buf, WALL_W, WALL_H, time_val, grain, angle_deg);

    char path[512];
    ws_wallpapers_path(path, sizeof(path), "wallsee.ppm");

    bool ok = ws_write_ppm(path, buf, WALL_W, WALL_H);
    free(buf);
    return ok;
}

// set wallpaper via swaymsg + create lock
static bool ws_set_wallpaper(float time_val, int grain, float angle_deg)
{
    // save the image first
    if (!ws_save_wallpaper(time_val, grain, angle_deg))
        return false;

    // apply via swaymsg
    char cmd[1024];
    char ppm_path[512];
    ws_wallpapers_path(ppm_path, sizeof(ppm_path), "wallsee.ppm");
    snprintf(cmd, sizeof(cmd),
        "swaymsg 'output * bg %s fill'", ppm_path);
    system(cmd);

    // create lock file
    ws_lock_create();
    return true;
}

int app_wallsee(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 440;
    int win_h = 420;
    core_init("l wallsee", win_w, win_h);

    // slider values
    float time_val  = 12.0f;
    float grain_f   = 5.0f;
    float angle_f   = 90.0f;

    // preview texture
    Image prev_img = GenImageColor(PREV_W, PREV_H, BLACK);
    Texture2D prev_tex = LoadTextureFromImage(prev_img);
    unsigned char *prev_buf = malloc(PREV_W * PREV_H * 3);

    bool needs_update = true;

    while (!WindowShouldClose()) {
        // regenerate preview when sliders change
        if (needs_update) {
            ws_generate(prev_buf, PREV_W, PREV_H, time_val, (int)grain_f, angle_f);

            // convert rgb to raylib rgba
            Color *pixels = LoadImageColors(prev_img);
            for (int i = 0; i < PREV_W * PREV_H; i++) {
                pixels[i].r = prev_buf[i * 3 + 0];
                pixels[i].g = prev_buf[i * 3 + 1];
                pixels[i].b = prev_buf[i * 3 + 2];
                pixels[i].a = 255;
            }
            UpdateTexture(prev_tex, pixels);
            UnloadImageColors(pixels);
            needs_update = false;
        }

        BeginDrawing();
        ClearBackground(GRV_BG);

        // title
        DrawTextEx(g_font, "wallsee", (Vector2){ 16, 10 }, 20, 1, GRV_AQUA);

        // lock indicator
        if (ws_lock_exists()) {
            DrawTextEx(g_font, "[locked]", (Vector2){ 340, 10 }, 15, 1, GRV_GREEN);
        } else {
            DrawTextEx(g_font, "[unlocked]", (Vector2){ 330, 10 }, 15, 1, GRV_RED);
        }

        // preview
        DrawTexture(prev_tex, (win_w - PREV_W) / 2, 36, WHITE);

        // sliders
        float y = 264.0f;
        float old_time  = time_val;
        float old_grain = grain_f;
        float old_angle = angle_f;

        DrawTextEx(g_font, "time", (Vector2){ 16, y }, 15, 1, GRV_FG);
        GuiSliderBar((Rectangle){ 80, y, 260, 20 }, NULL, NULL, &time_val, 0.0f, 24.0f);
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%.1f", time_val);
        DrawTextEx(g_font, time_str, (Vector2){ 350, y }, 15, 1, GRV_YELLOW);

        y += 32.0f;
        DrawTextEx(g_font, "grain", (Vector2){ 16, y }, 15, 1, GRV_FG);
        GuiSliderBar((Rectangle){ 80, y, 260, 20 }, NULL, NULL, &grain_f, 0.0f, 30.0f);
        char grain_str[32];
        snprintf(grain_str, sizeof(grain_str), "%d", (int)grain_f);
        DrawTextEx(g_font, grain_str, (Vector2){ 350, y }, 15, 1, GRV_YELLOW);

        y += 32.0f;
        DrawTextEx(g_font, "angle", (Vector2){ 16, y }, 15, 1, GRV_FG);
        GuiSliderBar((Rectangle){ 80, y, 260, 20 }, NULL, NULL, &angle_f, 0.0f, 360.0f);
        char angle_str[32];
        snprintf(angle_str, sizeof(angle_str), "%.0f", angle_f);
        DrawTextEx(g_font, angle_str, (Vector2){ 350, y }, 15, 1, GRV_YELLOW);

        // detect slider changes
        if (old_time != time_val || old_grain != grain_f || old_angle != angle_f)
            needs_update = true;

        // buttons row
        y += 40.0f;
        if (GuiButton((Rectangle){ 16, y, 120, 32 }, "Save")) {
            if (ws_save_wallpaper(time_val, (int)grain_f, angle_f))
                ws_set_status("saved to ~/Wallpapers/");
            else
                ws_set_status("error: could not save");
        }

        if (GuiButton((Rectangle){ 152, y, 120, 32 }, "Set")) {
            if (ws_set_wallpaper(time_val, (int)grain_f, angle_f))
                ws_set_status("wallpaper set + locked");
            else
                ws_set_status("error: could not set");
        }

        if (GuiButton((Rectangle){ 288, y, 120, 32 }, "Deset")) {
            if (ws_lock_exists()) {
                if (ws_lock_remove())
                    ws_set_status("lock removed");
                else
                    ws_set_status("error: could not remove lock");
            } else {
                ws_set_status("no lock to remove");
            }
        }

        // status bar
        if (ws_status_timer > 0) {
            ws_status_timer--;
            DrawTextEx(g_font, ws_status, (Vector2){ 16, y + 40 }, 15, 1, GRV_YELLOW);
        }

        EndDrawing();
    }

    free(prev_buf);
    UnloadTexture(prev_tex);
    UnloadImage(prev_img);
    core_close();
    return 0;
}
