#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "raygui.h"

Theme g_theme;

const Theme builtin_themes[] = {
    {
        .name           = "gruvbox",
        .bg             = { 40,  40,  40,  255 },
        .bg1            = { 60,  56,  54,  255 },
        .bg2            = { 80,  73,  69,  255 },
        .bg3            = { 102, 92,  84,  255 },
        .fg             = { 235, 219, 178, 255 },
        .fg0            = { 251, 241, 199, 255 },
        .red            = { 204, 36,  29,  255 },
        .orange         = { 214, 93,  14,  255 },
        .green          = { 152, 151, 26,  255 },
        .aqua           = { 104, 157, 106, 255 },
        .yellow         = { 215, 153, 33,  255 },
        .blue           = { 69,  133, 136, 255 },
        .purple         = { 177, 98,  134, 255 },
        .gray           = { 146, 131, 116, 255 },
        .bright_red     = { 251, 73,  52,  255 },
        .bright_green   = { 184, 187, 38,  255 },
        .bright_yellow  = { 250, 189, 47,  255 },
        .bright_blue    = { 131, 165, 152, 255 },
        .bright_purple  = { 211, 134, 155, 255 },
        .bright_aqua    = { 142, 192, 124, 255 },
        .bright_orange  = { 254, 128, 25,  255 },
        .bg_hard        = { 29,  32,  33,  255 },
        .bg_soft        = { 50,  48,  47,  255 },
        .white          = { 168, 153, 132, 255 },
    },
    {
        .name           = "tokyo-night",
        .bg             = { 0x1a, 0x1b, 0x26, 255 },
        .bg1            = { 0x24, 0x28, 0x3b, 255 },
        .bg2            = { 0x2f, 0x33, 0x4d, 255 },
        .bg3            = { 0x41, 0x48, 0x68, 255 },
        .fg             = { 0xc0, 0xca, 0xf5, 255 },
        .fg0            = { 0xa9, 0xb1, 0xd6, 255 },
        .red            = { 0xf7, 0x76, 0x8e, 255 },
        .orange         = { 0xff, 0x9e, 0x64, 255 },
        .green          = { 0x9e, 0xce, 0x6a, 255 },
        .aqua           = { 0x7d, 0xcf, 0xff, 255 },
        .yellow         = { 0xe0, 0xaf, 0x68, 255 },
        .blue           = { 0x7a, 0xa2, 0xf7, 255 },
        .purple         = { 0xbb, 0x9a, 0xf7, 255 },
        .gray           = { 0x56, 0x5f, 0x89, 255 },
        .bright_red     = { 0xff, 0x7a, 0x93, 255 },
        .bright_green   = { 0xb9, 0xf2, 0x7c, 255 },
        .bright_yellow  = { 0xff, 0xd0, 0x7c, 255 },
        .bright_blue    = { 0x9a, 0xa5, 0xce, 255 },
        .bright_purple  = { 0xc0, 0xa4, 0xf8, 255 },
        .bright_aqua    = { 0xb4, 0xf9, 0xf8, 255 },
        .bright_orange  = { 0xff, 0x9e, 0x64, 255 },
        .bg_hard        = { 0x16, 0x16, 0x1e, 255 },
        .bg_soft        = { 0x1f, 0x23, 0x35, 255 },
        .white          = { 0xa9, 0xb1, 0xd6, 255 },
    },
    {
        .name           = "catppuccin-mocha",
        .bg             = { 0x1e, 0x1e, 0x2e, 255 },
        .bg1            = { 0x28, 0x28, 0x3c, 255 },
        .bg2            = { 0x31, 0x32, 0x44, 255 },
        .bg3            = { 0x45, 0x47, 0x5a, 255 },
        .fg             = { 0xcd, 0xd6, 0xf4, 255 },
        .fg0            = { 0xf5, 0xe0, 0xdc, 255 },
        .red            = { 0xf3, 0x8b, 0xa8, 255 },
        .orange         = { 0xfa, 0xb3, 0x87, 255 },
        .green          = { 0xa6, 0xe3, 0xa1, 255 },
        .aqua           = { 0x94, 0xe2, 0xd5, 255 },
        .yellow         = { 0xf9, 0xe2, 0xaf, 255 },
        .blue           = { 0x89, 0xb4, 0xfa, 255 },
        .purple         = { 0xcb, 0xa6, 0xf7, 255 },
        .gray           = { 0x6c, 0x70, 0x86, 255 },
        .bright_red     = { 0xeb, 0xa0, 0xac, 255 },
        .bright_green   = { 0xa6, 0xe3, 0xa1, 255 },
        .bright_yellow  = { 0xf9, 0xe2, 0xaf, 255 },
        .bright_blue    = { 0x74, 0xc7, 0xec, 255 },
        .bright_purple  = { 0xb4, 0xbe, 0xfe, 255 },
        .bright_aqua    = { 0x89, 0xdc, 0xeb, 255 },
        .bright_orange  = { 0xfa, 0xb3, 0x87, 255 },
        .bg_hard        = { 0x11, 0x11, 0x1b, 255 },
        .bg_soft        = { 0x18, 0x18, 0x25, 255 },
        .white          = { 0x7f, 0x84, 0x9c, 255 },
    },
    {
        .name           = "nord",
        .bg             = { 0x2e, 0x34, 0x40, 255 },
        .bg1            = { 0x3b, 0x42, 0x52, 255 },
        .bg2            = { 0x43, 0x4c, 0x5e, 255 },
        .bg3            = { 0x4c, 0x56, 0x6a, 255 },
        .fg             = { 0xd8, 0xde, 0xe9, 255 },
        .fg0            = { 0xec, 0xef, 0xf4, 255 },
        .red            = { 0xbf, 0x61, 0x6a, 255 },
        .orange         = { 0xd0, 0x87, 0x70, 255 },
        .green          = { 0xa3, 0xbe, 0x8c, 255 },
        .aqua           = { 0x88, 0xc0, 0xd0, 255 },
        .yellow         = { 0xeb, 0xcb, 0x8b, 255 },
        .blue           = { 0x81, 0xa1, 0xc1, 255 },
        .purple         = { 0xb4, 0x8e, 0xad, 255 },
        .gray           = { 0x4c, 0x56, 0x6a, 255 },
        .bright_red     = { 0xbf, 0x61, 0x6a, 255 },
        .bright_green   = { 0xa3, 0xbe, 0x8c, 255 },
        .bright_yellow  = { 0xeb, 0xcb, 0x8b, 255 },
        .bright_blue    = { 0x5e, 0x81, 0xac, 255 },
        .bright_purple  = { 0xb4, 0x8e, 0xad, 255 },
        .bright_aqua    = { 0x8f, 0xbc, 0xbb, 255 },
        .bright_orange  = { 0xd0, 0x87, 0x70, 255 },
        .bg_hard        = { 0x29, 0x2e, 0x39, 255 },
        .bg_soft        = { 0x36, 0x3c, 0x4a, 255 },
        .white          = { 0xd8, 0xde, 0xe9, 255 },
    },
};

const int builtin_theme_count = sizeof(builtin_themes) / sizeof(builtin_themes[0]);

const Theme *theme_find_builtin(const char *name)
{
    for (int i = 0; i < builtin_theme_count; i++) {
        if (strcmp(builtin_themes[i].name, name) == 0)
            return &builtin_themes[i];
    }
    return &builtin_themes[0];
}

int theme_color(Color c)
{
    return ((int)c.r << 24) | ((int)c.g << 16) | ((int)c.b << 8) | (int)c.a;
}

const char *theme_config_path(void)
{
    static char path[512];
    const char *home = getenv("HOME");
    if (!home || !*home) home = "/tmp";
    snprintf(path, sizeof(path), "%s/.config/sway/theme.conf", home);
    return path;
}

static Color hex_to_color(const char *s)
{
    while (*s == '#' || isspace((unsigned char)*s)) s++;
    unsigned int v = 0;
    if (sscanf(s, "%x", &v) != 1) return (Color){ 0, 0, 0, 255 };
    return (Color){
        (unsigned char)((v >> 16) & 0xFF),
        (unsigned char)((v >> 8) & 0xFF),
        (unsigned char)(v & 0xFF),
        255,
    };
}

static int parse_kv(const char *line, char *key, size_t key_sz,
                    char *val, size_t val_sz)
{
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line == '#' || *line == '\0' || *line == '\n') return 0;

    const char *eq = strchr(line, '=');
    if (!eq) return 0;

    size_t klen = 0;
    for (const char *p = line; p < eq && klen + 1 < key_sz; p++) {
        if (!isspace((unsigned char)*p)) key[klen++] = *p;
    }
    key[klen] = '\0';

    const char *p = eq + 1;
    while (*p && isspace((unsigned char)*p)) p++;
    size_t vlen = 0;
    while (*p && *p != '\n' && *p != '#' && vlen + 1 < val_sz) {
        val[vlen++] = *p++;
    }
    while (vlen > 0 && isspace((unsigned char)val[vlen - 1])) vlen--;
    val[vlen] = '\0';

    return klen > 0 && vlen > 0;
}

static void assign_color(const char *key, Color c)
{
    if      (strcmp(key, "bg")             == 0) g_theme.bg             = c;
    else if (strcmp(key, "bg1")            == 0) g_theme.bg1            = c;
    else if (strcmp(key, "bg2")            == 0) g_theme.bg2            = c;
    else if (strcmp(key, "bg3")            == 0) g_theme.bg3            = c;
    else if (strcmp(key, "fg")             == 0) g_theme.fg             = c;
    else if (strcmp(key, "fg0")            == 0) g_theme.fg0            = c;
    else if (strcmp(key, "red")            == 0) g_theme.red            = c;
    else if (strcmp(key, "orange")         == 0) g_theme.orange         = c;
    else if (strcmp(key, "green")          == 0) g_theme.green          = c;
    else if (strcmp(key, "aqua")           == 0) g_theme.aqua           = c;
    else if (strcmp(key, "yellow")         == 0) g_theme.yellow         = c;
    else if (strcmp(key, "blue")           == 0) g_theme.blue           = c;
    else if (strcmp(key, "purple")         == 0) g_theme.purple         = c;
    else if (strcmp(key, "gray")           == 0) g_theme.gray           = c;
    else if (strcmp(key, "bright_red")     == 0) g_theme.bright_red     = c;
    else if (strcmp(key, "bright_green")   == 0) g_theme.bright_green   = c;
    else if (strcmp(key, "bright_yellow")  == 0) g_theme.bright_yellow  = c;
    else if (strcmp(key, "bright_blue")    == 0) g_theme.bright_blue    = c;
    else if (strcmp(key, "bright_purple")  == 0) g_theme.bright_purple  = c;
    else if (strcmp(key, "bright_aqua")    == 0) g_theme.bright_aqua    = c;
    else if (strcmp(key, "bright_orange")  == 0) g_theme.bright_orange  = c;
    else if (strcmp(key, "bg_hard")        == 0) g_theme.bg_hard        = c;
    else if (strcmp(key, "bg_soft")        == 0) g_theme.bg_soft        = c;
    else if (strcmp(key, "white")          == 0) g_theme.white          = c;
}

void theme_load(void)
{
    g_theme = builtin_themes[0];

    const char *path = theme_config_path();
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[256];
    char key[64];
    char val[64];

    while (fgets(line, sizeof(line), f)) {
        if (!parse_kv(line, key, sizeof(key), val, sizeof(val))) continue;

        if (strcmp(key, "name") == 0) {
            strncpy(g_theme.name, val, sizeof(g_theme.name) - 1);
            g_theme.name[sizeof(g_theme.name) - 1] = '\0';
            continue;
        }

        assign_color(key, hex_to_color(val));
    }

    fclose(f);
}

int theme_write(const Theme *t)
{
    const char *path = theme_config_path();

    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        char parent[512];
        snprintf(parent, sizeof(parent), "%s", dir);
        char *pslash = strrchr(parent, '/');
        if (pslash) {
            *pslash = '\0';
            mkdir(parent, 0755);
        }
        mkdir(dir, 0755);
    }

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "name   = %s\n", t->name);
    fprintf(f, "bg     = %02x%02x%02x\n", t->bg.r,     t->bg.g,     t->bg.b);
    fprintf(f, "bg1    = %02x%02x%02x\n", t->bg1.r,    t->bg1.g,    t->bg1.b);
    fprintf(f, "bg2    = %02x%02x%02x\n", t->bg2.r,    t->bg2.g,    t->bg2.b);
    fprintf(f, "bg3    = %02x%02x%02x\n", t->bg3.r,    t->bg3.g,    t->bg3.b);
    fprintf(f, "fg     = %02x%02x%02x\n", t->fg.r,     t->fg.g,     t->fg.b);
    fprintf(f, "fg0    = %02x%02x%02x\n", t->fg0.r,    t->fg0.g,    t->fg0.b);
    fprintf(f, "red    = %02x%02x%02x\n", t->red.r,    t->red.g,    t->red.b);
    fprintf(f, "orange = %02x%02x%02x\n", t->orange.r, t->orange.g, t->orange.b);
    fprintf(f, "green  = %02x%02x%02x\n", t->green.r,  t->green.g,  t->green.b);
    fprintf(f, "aqua   = %02x%02x%02x\n", t->aqua.r,   t->aqua.g,   t->aqua.b);
    fprintf(f, "yellow = %02x%02x%02x\n", t->yellow.r, t->yellow.g, t->yellow.b);
    fprintf(f, "blue   = %02x%02x%02x\n", t->blue.r,   t->blue.g,   t->blue.b);
    fprintf(f, "purple = %02x%02x%02x\n", t->purple.r, t->purple.g, t->purple.b);
    fprintf(f, "gray   = %02x%02x%02x\n", t->gray.r,   t->gray.g,   t->gray.b);
    fprintf(f, "bright_red    = %02x%02x%02x\n", t->bright_red.r,    t->bright_red.g,    t->bright_red.b);
    fprintf(f, "bright_green  = %02x%02x%02x\n", t->bright_green.r,  t->bright_green.g,  t->bright_green.b);
    fprintf(f, "bright_yellow = %02x%02x%02x\n", t->bright_yellow.r, t->bright_yellow.g, t->bright_yellow.b);
    fprintf(f, "bright_blue   = %02x%02x%02x\n", t->bright_blue.r,   t->bright_blue.g,   t->bright_blue.b);
    fprintf(f, "bright_purple = %02x%02x%02x\n", t->bright_purple.r, t->bright_purple.g, t->bright_purple.b);
    fprintf(f, "bright_aqua   = %02x%02x%02x\n", t->bright_aqua.r,   t->bright_aqua.g,   t->bright_aqua.b);
    fprintf(f, "bright_orange = %02x%02x%02x\n", t->bright_orange.r, t->bright_orange.g, t->bright_orange.b);
    fprintf(f, "bg_hard       = %02x%02x%02x\n", t->bg_hard.r,       t->bg_hard.g,       t->bg_hard.b);
    fprintf(f, "bg_soft       = %02x%02x%02x\n", t->bg_soft.r,       t->bg_soft.g,       t->bg_soft.b);
    fprintf(f, "white         = %02x%02x%02x\n", t->white.r,         t->white.g,         t->white.b);

    fclose(f);
    return 0;
}

void theme_apply_style(void)
{
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR,    theme_color(g_theme.bg1));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   theme_color(g_theme.fg));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   theme_color(g_theme.bg2));
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, theme_color(g_theme.bg2));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,  theme_color(g_theme.fg0));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,  theme_color(g_theme.orange));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED,theme_color(g_theme.orange));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,  theme_color(g_theme.bg));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,  theme_color(g_theme.green));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED,theme_color(g_theme.green));

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,    theme_color(g_theme.bg2));
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,    theme_color(g_theme.fg));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,  theme_color(g_theme.aqua));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,   theme_color(g_theme.orange));
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,   theme_color(g_theme.fg0));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,   theme_color(g_theme.green));
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,   theme_color(g_theme.bg));
}
