#ifndef THEME_H
#define THEME_H

#include "raylib.h"

typedef struct {
    char  name[64];
    Color bg;
    Color bg1;
    Color bg2;
    Color bg3;
    Color fg;
    Color fg0;
    Color red;
    Color orange;
    Color green;
    Color aqua;
    Color yellow;
} Theme;

extern Theme g_theme;
extern const Theme builtin_themes[];
extern const int   builtin_theme_count;

void theme_load(void);
int  theme_color(Color c);
void theme_apply_style(void);
const char *theme_config_path(void);
int  theme_write(const Theme *t);

#endif
