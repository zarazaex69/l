#include "wallcreate.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float r, g, b;
} WcColor;

// interpolate between two colors
static WcColor wc_lerp(WcColor c1, WcColor c2, float t)
{
    return (WcColor){
        c1.r + t * (c2.r - c1.r),
        c1.g + t * (c2.g - c1.g),
        c1.b + t * (c2.b - c1.b),
    };
}

// pick gradient colors based on time of day
static void wc_time_colors(float time_val, WcColor *top, WcColor *bottom)
{
    WcColor c_night_t = { 10.0f,  10.0f,  25.0f  };
    WcColor c_night_b = { 5.0f,   5.0f,   15.0f  };
    WcColor c_dawn_t  = { 200.0f, 100.0f, 80.0f  };
    WcColor c_dawn_b  = { 100.0f, 50.0f,  60.0f  };
    WcColor c_noon_t  = { 100.0f, 200.0f, 255.0f };
    WcColor c_noon_b  = { 180.0f, 230.0f, 255.0f };
    WcColor c_dusk_t  = { 80.0f,  40.0f,  100.0f };
    WcColor c_dusk_b  = { 250.0f, 120.0f, 80.0f  };

    if (time_val < 6.0f) {
        float t = time_val / 6.0f;
        *top    = wc_lerp(c_night_t, c_dawn_t, t);
        *bottom = wc_lerp(c_night_b, c_dawn_b, t);
    } else if (time_val < 12.0f) {
        float t = (time_val - 6.0f) / 6.0f;
        *top    = wc_lerp(c_dawn_t, c_noon_t, t);
        *bottom = wc_lerp(c_dawn_b, c_noon_b, t);
    } else if (time_val < 18.0f) {
        float t = (time_val - 12.0f) / 6.0f;
        *top    = wc_lerp(c_noon_t, c_dusk_t, t);
        *bottom = wc_lerp(c_noon_b, c_dusk_b, t);
    } else {
        float t = (time_val - 18.0f) / 6.0f;
        *top    = wc_lerp(c_dusk_t, c_night_t, t);
        *bottom = wc_lerp(c_dusk_b, c_night_b, t);
    }
}

// clamp int to 0..255
static inline unsigned char wc_clamp(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

int app_wallcreate(int argc, char **argv)
{
    // argv[0] = "l", argv[1] = "wallcreate", real args start at [2]
    if (argc != 7) {
        fprintf(stderr,
            "usage: l wallcreate <width> <height> <time 0.0-24.0> <grain> <angle_deg>\n");
        return 1;
    }

    int width       = atoi(argv[2]);
    int height      = atoi(argv[3]);
    float time_val  = atof(argv[4]);
    int base_grain  = atoi(argv[5]);
    float angle_deg = atof(argv[6]);

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "error: invalid dimensions\n");
        return 1;
    }

    if (time_val < 0.0f || time_val > 24.0f)
        time_val = 0.0f;

    unsigned char *img = malloc(width * height * 3);
    if (!img) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    srand(1337);

    WcColor top_color, bottom_color;
    wc_time_colors(time_val, &top_color, &bottom_color);

    // project pixel coords onto gradient axis
    float angle_rad = angle_deg * M_PI / 180.0f;
    float dx = cos(angle_rad);
    float dy = sin(angle_rad);

    float pw0 = width * dx;
    float p0h = height * dy;
    float pwh = width * dx + height * dy;

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
    for (int y = 0; y < height; y++) {
        float y_proj = y * dy;
        for (int x = 0; x < width; x++) {
            float proj_val = x * dx + y_proj;
            float t = (proj_val - proj_min) / proj_range;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float cr = top_color.r + t * (bottom_color.r - top_color.r);
            float cg = top_color.g + t * (bottom_color.g - top_color.g);
            float cb = top_color.b + t * (bottom_color.b - top_color.b);

            int noise = (base_grain > 0)
                ? (rand() % (base_grain * 2 + 1)) - base_grain
                : 0;

            img[idx++] = wc_clamp((int)cr + noise);
            img[idx++] = wc_clamp((int)cg + noise);
            img[idx++] = wc_clamp((int)cb + noise);
        }
    }

    // output raw ppm to stdout
    printf("P6\n%d %d\n255\n", width, height);
    fwrite(img, 1, width * height * 3, stdout);

    free(img);
    return 0;
}
