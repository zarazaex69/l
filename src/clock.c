#include <stdio.h>
#include <time.h>
#include <math.h>
#include "core.h"
#include "clock.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CLOCK_SIZE 400
#define CENTER_X   (CLOCK_SIZE / 2)
#define CENTER_Y   (CLOCK_SIZE / 2 - 20)
#define RADIUS     150

static void draw_hand(float cx, float cy, float angle_deg, float length, float thick, Color color)
{
    float rad = (angle_deg - 90.0f) * (float)(M_PI / 180.0);
    float ex = cx + cosf(rad) * length;
    float ey = cy + sinf(rad) * length;
    DrawLineEx((Vector2){ cx, cy }, (Vector2){ ex, ey }, thick, color);
}

static void draw_clock_face(float cx, float cy, float radius)
{
    DrawCircleV((Vector2){ cx, cy }, radius + 4, GRV_BG3);
    DrawCircleV((Vector2){ cx, cy }, radius, GRV_BG1);

    for (int i = 0; i < 60; i++) {
        float angle = (float)i * 6.0f - 90.0f;
        float rad = angle * (float)(M_PI / 180.0);
        float inner, outer;
        float thick;
        Color col;

        if (i % 5 == 0) {
            inner = radius - 18.0f;
            outer = radius - 4.0f;
            thick = 2.5f;
            col = GRV_FG;
        } else {
            inner = radius - 10.0f;
            outer = radius - 4.0f;
            thick = 1.0f;
            col = GRV_BG3;
        }

        float x1 = cx + cosf(rad) * inner;
        float y1 = cy + sinf(rad) * inner;
        float x2 = cx + cosf(rad) * outer;
        float y2 = cy + sinf(rad) * outer;
        DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, thick, col);
    }

    const char *numerals[] = {
        "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"
    };
    for (int i = 0; i < 12; i++) {
        float angle = (float)i * 30.0f - 90.0f;
        float rad = angle * (float)(M_PI / 180.0);
        float nr = radius - 34.0f;
        float nx = cx + cosf(rad) * nr;
        float ny = cy + sinf(rad) * nr;
        Vector2 ts = MeasureTextEx(g_font, numerals[i], 18, 1);
        DrawTextEx(g_font, numerals[i], (Vector2){ nx - ts.x / 2, ny - ts.y / 2 }, 18, 1, GRV_FG);
    }
}

int app_clock(int argc, char **argv)
{
    (void)argc; (void)argv;
    core_init("l clock", CLOCK_SIZE, CLOCK_SIZE);

    float cx = (float)CENTER_X;
    float cy = (float)CENTER_Y;

    while (!WindowShouldClose()) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        int hour = t->tm_hour;
        int min  = t->tm_min;
        int sec  = t->tm_sec;

        float sec_angle  = (float)sec * 6.0f;
        float min_angle  = (float)min * 6.0f + (float)sec * 0.1f;
        float hour_angle = (float)(hour % 12) * 30.0f + (float)min * 0.5f;

        BeginDrawing();
        ClearBackground(GRV_BG);

        draw_clock_face(cx, cy, (float)RADIUS);

        draw_hand(cx, cy, hour_angle, (float)RADIUS * 0.5f,  4.0f, GRV_AQUA);
        draw_hand(cx, cy, min_angle,  (float)RADIUS * 0.72f, 3.0f, GRV_FG);
        draw_hand(cx, cy, sec_angle,  (float)RADIUS * 0.85f, 1.5f, GRV_RED);

        DrawCircleV((Vector2){ cx, cy }, 5, GRV_ORANGE);

        char digital[16];
        snprintf(digital, sizeof(digital), "%02d:%02d:%02d", hour, min, sec);
        Vector2 ds = MeasureTextEx(g_font, digital, 24, 1);
        float dx = cx - ds.x / 2;
        float dy = (float)CLOCK_SIZE - 40.0f;
        DrawTextEx(g_font, digital, (Vector2){ dx, dy }, 24, 1, GRV_YELLOW);

        EndDrawing();
    }

    core_close();
    return 0;
}
