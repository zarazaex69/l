#include <stdio.h>
#include <string.h>

// raygui implementation in this translation unit
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// shared font
Font g_font = {0};

#include "helloworld.h"
#include "autorun.h"
#include "wallcreate.h"

// program registry
typedef struct {
    const char *name;
    int (*run)(int argc, char **argv);
    const char *description;
} Program;

static const Program programs[] = {
    { "helloworld",  app_helloworld,  "gruvbox hello world window" },
    { "autorun",     app_autorun,     "sway autorun manager" },
    { "wallcreate",  app_wallcreate,  "generate gradient wallpaper (ppm)" },
};

static const int program_count = sizeof(programs) / sizeof(programs[0]);

static void print_usage(void)
{
    fprintf(stderr, "usage: l <program>\n\navailable programs:\n");
    for (int i = 0; i < program_count; i++)
        fprintf(stderr, "  %-16s %s\n", programs[i].name, programs[i].description);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *name = argv[1];

    for (int i = 0; i < program_count; i++) {
        if (strcmp(name, programs[i].name) == 0)
            return programs[i].run(argc, argv);
    }

    fprintf(stderr, "error: unknown program '%s'\n\n", name);
    print_usage();
    return 1;
}
