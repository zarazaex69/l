#include "core.h"
#include "autorun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ENTRIES    64
#define MAX_LINE_LEN   512
#define CONFIG_PATH_SUFFIX "/.config/sway/conf.d/autostart"

typedef struct {
    char command[MAX_LINE_LEN];  // full line like "exec_always autotiling"
    char display[MAX_LINE_LEN];  // short name for display
    bool enabled;
} AutoEntry;

static AutoEntry entries[MAX_ENTRIES];
static int entry_count = 0;

// input buffer for adding new entries
static char add_buf[MAX_LINE_LEN] = {0};
static bool add_editing = false;

// scroll offset for the list
static float scroll_y = 0.0f;

// status message
static char status_msg[256] = {0};
static int status_timer = 0;

static void set_status(const char *msg)
{
    strncpy(status_msg, msg, sizeof(status_msg) - 1);
    status_msg[sizeof(status_msg) - 1] = '\0';
    status_timer = 180; // ~3 seconds at 60fps
}

// build full config path
static void get_config_path(char *out, int size)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, size, "%s%s", home, CONFIG_PATH_SUFFIX);
}

// extract display name from command line
// "exec_always --no-startup-id foo bar" -> "foo bar"
// "exec waybar" -> "waybar"
static void extract_display_name(const char *cmd, char *out, int size)
{
    const char *p = cmd;

    // skip leading whitespace
    while (*p == ' ' || *p == '\t') p++;

    // skip exec / exec_always
    if (strncmp(p, "exec_always", 11) == 0) p += 11;
    else if (strncmp(p, "exec", 4) == 0) p += 4;

    // skip whitespace
    while (*p == ' ' || *p == '\t') p++;

    // skip --no-startup-id
    if (strncmp(p, "--no-startup-id", 15) == 0) {
        p += 15;
        while (*p == ' ' || *p == '\t') p++;
    }

    strncpy(out, p, size - 1);
    out[size - 1] = '\0';

    // trim trailing whitespace and backslashes
    int len = strlen(out);
    while (len > 0 && (out[len-1] == ' ' || out[len-1] == '\t' ||
                        out[len-1] == '\n' || out[len-1] == '\r'))
        out[--len] = '\0';
}

// parse the autostart config file
static void load_config(void)
{
    char path[512];
    get_config_path(path, sizeof(path));

    entry_count = 0;
    memset(entries, 0, sizeof(entries));

    FILE *f = fopen(path, "r");
    if (!f) {
        set_status("could not open config file");
        return;
    }

    char line[MAX_LINE_LEN];
    char accumulated[MAX_LINE_LEN * 4] = {0};
    bool accumulating = false;

    while (fgets(line, sizeof(line), f)) {
        // strip newline
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        // skip empty lines and comments
        const char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (!accumulating) {
            if (*trimmed == '\0' || *trimmed == '#') continue;

            // check if line continues (ends with backslash)
            if (len > 0 && line[len-1] == '\\') {
                strncpy(accumulated, line, sizeof(accumulated) - 1);
                accumulating = true;
                continue;
            }

            // single line entry
            if (entry_count < MAX_ENTRIES &&
                (strncmp(trimmed, "exec_always", 11) == 0 ||
                 strncmp(trimmed, "exec", 4) == 0))
            {
                strncpy(entries[entry_count].command, trimmed, MAX_LINE_LEN - 1);
                extract_display_name(trimmed, entries[entry_count].display, MAX_LINE_LEN);
                entries[entry_count].enabled = true;
                entry_count++;
            }
        } else {
            // continuation line
            strncat(accumulated, "\n", sizeof(accumulated) - strlen(accumulated) - 1);
            strncat(accumulated, line, sizeof(accumulated) - strlen(accumulated) - 1);

            // check if this line also continues
            if (len > 0 && line[len-1] == '\\') continue;

            // done accumulating
            accumulating = false;
            const char *atrim = accumulated;
            while (*atrim == ' ' || *atrim == '\t') atrim++;

            if (entry_count < MAX_ENTRIES &&
                (strncmp(atrim, "exec_always", 11) == 0 ||
                 strncmp(atrim, "exec", 4) == 0))
            {
                strncpy(entries[entry_count].command, accumulated, MAX_LINE_LEN - 1);
                extract_display_name(atrim, entries[entry_count].display, MAX_LINE_LEN);
                entries[entry_count].enabled = true;
                entry_count++;
            }
            accumulated[0] = '\0';
        }
    }

    fclose(f);
    set_status("config loaded");
}

// save config back to file
static void save_config(void)
{
    char path[512];
    get_config_path(path, sizeof(path));

    FILE *f = fopen(path, "w");
    if (!f) {
        set_status("error: could not write config");
        return;
    }

    fprintf(f, "### AutoStart\n");

    for (int i = 0; i < entry_count; i++) {
        if (entries[i].enabled) {
            fprintf(f, "%s\n\n", entries[i].command);
        } else {
            // write disabled entries as comments
            // handle multiline commands
            char tmp[MAX_LINE_LEN];
            strncpy(tmp, entries[i].command, MAX_LINE_LEN - 1);
            tmp[MAX_LINE_LEN - 1] = '\0';

            char *saveptr = NULL;
            char *tok = strtok_r(tmp, "\n", &saveptr);
            while (tok) {
                fprintf(f, "# %s\n", tok);
                tok = strtok_r(NULL, "\n", &saveptr);
            }
            fprintf(f, "\n");
        }
    }

    fclose(f);
    set_status("config saved");
}

// add a new entry
static void add_entry(const char *cmd)
{
    if (entry_count >= MAX_ENTRIES) {
        set_status("error: max entries reached");
        return;
    }

    const char *trimmed = cmd;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    if (*trimmed == '\0') return;

    // auto-prepend exec if user didn't
    char full[MAX_LINE_LEN];
    if (strncmp(trimmed, "exec", 4) != 0) {
        snprintf(full, sizeof(full), "exec %s", trimmed);
    } else {
        strncpy(full, trimmed, sizeof(full) - 1);
        full[sizeof(full) - 1] = '\0';
    }

    strncpy(entries[entry_count].command, full, MAX_LINE_LEN - 1);
    extract_display_name(full, entries[entry_count].display, MAX_LINE_LEN);
    entries[entry_count].enabled = true;
    entry_count++;
    set_status("entry added");
}

// remove entry by index
static void remove_entry(int idx)
{
    if (idx < 0 || idx >= entry_count) return;
    for (int i = idx; i < entry_count - 1; i++)
        entries[i] = entries[i + 1];
    entry_count--;
    set_status("entry removed");
}

// count newlines in display text
static int count_lines(const char *s)
{
    int n = 1;
    for (; *s; s++)
        if (*s == '\n') n++;
    return n;
}

// height of one entry row based on line count
#define LINE_H      18
#define ROW_PAD     10

static float entry_height(int idx)
{
    int lines = count_lines(entries[idx].display);
    return lines * LINE_H + ROW_PAD;
}

// total height of all entries
static float total_list_height(void)
{
    float h = 0;
    for (int i = 0; i < entry_count; i++)
        h += entry_height(i);
    return h;
}

int app_autorun(int argc, char **argv)
{
    (void)argc; (void)argv;
    core_init("l autorun", 600, 500);
    load_config();

    int remove_idx = -1;

    while (!WindowShouldClose()) {
        // handle scroll
        float wheel = GetMouseWheelMove();
        scroll_y -= wheel * 30.0f;
        if (scroll_y < 0) scroll_y = 0;
        float max_scroll = total_list_height() - 340.0f;
        if (max_scroll < 0) max_scroll = 0;
        if (scroll_y > max_scroll) scroll_y = max_scroll;

        BeginDrawing();
        ClearBackground(GRV_BG);

        // title
        DrawTextEx(g_font, "sway autorun manager", (Vector2){ 16, 12 }, 20, 1, GRV_AQUA);

        // entry list area
        DrawRectangle(10, 40, 580, 340, (Color){ 50, 48, 47, 255 });

        // clipping via scissor
        BeginScissorMode(10, 40, 580, 340);

        remove_idx = -1;
        float y = 44.0f - scroll_y;
        for (int i = 0; i < entry_count; i++) {
            float h = entry_height(i);

            // skip if fully outside visible area
            if (y + h < 40 || y > 380) {
                y += h;
                continue;
            }

            // checkbox for enable/disable
            bool prev = entries[i].enabled;
            GuiCheckBox((Rectangle){ 18, y + 2, 16, 16 }, NULL, &entries[i].enabled);
            if (prev != entries[i].enabled) {
                set_status(entries[i].enabled ? "enabled" : "disabled");
            }

            // display name
            Color text_col = entries[i].enabled ? GRV_FG : GRV_BG3;
            DrawTextEx(g_font, entries[i].display, (Vector2){ 44, y }, 15, 1, text_col);

            // remove button (vertically centered in row)
            if (GuiButton((Rectangle){ 540, y + 2, 40, 20 }, "X")) {
                remove_idx = i;
            }

            y += h;
        }

        EndScissorMode();

        // add new entry section
        DrawTextEx(g_font, "add:", (Vector2){ 16, 392 }, 15, 1, GRV_FG);

        if (GuiTextBox((Rectangle){ 60, 388, 420, 24 }, add_buf, MAX_LINE_LEN, add_editing))
            add_editing = !add_editing;

        if (GuiButton((Rectangle){ 490, 388, 100, 24 }, "Add")) {
            add_entry(add_buf);
            add_buf[0] = '\0';
        }

        // bottom buttons
        if (GuiButton((Rectangle){ 16, 430, 120, 32 }, "Save")) {
            save_config();
        }

        if (GuiButton((Rectangle){ 150, 430, 120, 32 }, "Reload")) {
            load_config();
        }

        // status bar
        if (status_timer > 0) {
            status_timer--;
            DrawTextEx(g_font, status_msg, (Vector2){ 290, 438 }, 15, 1, GRV_YELLOW);
        }

        EndDrawing();

        // deferred removal (after drawing)
        if (remove_idx >= 0) remove_entry(remove_idx);
    }

    core_close();
    return 0;
}
