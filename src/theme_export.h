#ifndef THEME_EXPORT_H
#define THEME_EXPORT_H

#include "theme.h"

int theme_export_sway(const Theme *t);
int theme_export_waybar(const Theme *t);
int theme_export_foot(const Theme *t);
int theme_export_dunst(const Theme *t);
int theme_export_all(const Theme *t);
void theme_reload_system(void);

#endif
