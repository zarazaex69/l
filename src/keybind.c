#include "core.h"
#include "keybind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define KB_MAX_VARS      64
#define KB_MAX_ENTRIES   512
#define KB_MAX_LINE      512
#define KB_MAX_SEARCH    128
#define KB_MAX_FILES     16

// sway variable ($name -> value)
typedef struct {
    char name[128];
    char value[256];
} KbVar;

// entry: either [L] header or [l] bind comment
typedef struct {
    bool is_header;          // [L] = header, [l] = bind description
    char text[512];          // resolved text (variables substituted inside {})
    char raw[512];           // original text before resolution
    char source[128];        // source filename
} KbEntry;

static KbVar   kb_vars[KB_MAX_VARS];
static int     kb_var_count = 0;
static KbEntry kb_entries[KB_MAX_ENTRIES];
static int     kb_entry_count = 0;

// config files to scan
static char    kb_files[KB_MAX_FILES][256];
static int     kb_file_count = 0;

// ui state
static char    kb_search[KB_MAX_SEARCH] = {0};
static bool    kb_search_editing = false;
static float   kb_scroll = 0.0f;
static char    kb_status[256] = {0};
static int     kb_status_timer = 0;

static void kb_set_status(const char *msg)
{
    strncpy(kb_status, msg, sizeof(kb_status) - 1);
    kb_status[sizeof(kb_status) - 1] = '\0';
    kb_status_timer = 180;
}

static void kb_config_path(char *out, int size, const char *filename)
{
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(out, size, "%s/.config/sway/conf.d/%s", home, filename);
}

// human-readable key name mapping
typedef struct {
    const char *from;
    const char *to;
} KbKeyMap;

static const KbKeyMap kb_keymap[] = {
    { "Mod4",                "Win" },
    { "Mod1",                "Alt" },
    { "Return",              "Enter" },
    { "Escape",              "Esc" },
    { "space",               "Space" },
    { "minus",               "-" },
    { "plus",                "+" },
    { "equal",               "=" },
    { "Print",               "PrtSc" },
    { "XF86AudioMute",       "AudioMute" },
    { "XF86AudioLowerVolume","Vol-" },
    { "XF86AudioRaiseVolume","Vol+" },
    { "XF86AudioMicMute",    "MicMute" },
    { "XF86MonBrightnessDown","Bright-" },
    { "XF86MonBrightnessUp", "Bright+" },
    { "Left",                "<-" },
    { "Right",               "->" },
    { "Up",                  "^" },
    { "Down",                "v" },
    { NULL, NULL }
};

// replace human-readable key names in a resolved string
// splits on + and replaces each segment
static void kb_humanize_keys(char *text, int size)
{
    char tmp[512];
    strncpy(tmp, text, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    text[0] = '\0';
    int oi = 0;

    char *saveptr = NULL;
    char *tok = strtok_r(tmp, "+", &saveptr);
    while (tok) {
        // find mapping
        const char *replacement = tok;
        for (int m = 0; kb_keymap[m].from; m++) {
            if (strcmp(tok, kb_keymap[m].from) == 0) {
                replacement = kb_keymap[m].to;
                break;
            }
        }

        int rlen = strlen(replacement);
        if (oi + rlen + 1 < size) {
            if (oi > 0) text[oi++] = '+';
            memcpy(text + oi, replacement, rlen);
            oi += rlen;
        }
        tok = strtok_r(NULL, "+", &saveptr);
    }
    text[oi] = '\0';
}

// resolve variables inside {} and strip braces from output
// then humanize key names in the resolved content
static void kb_resolve_braces(const char *input, char *output, int outsize)
{
    int oi = 0;
    int len = strlen(input);

    for (int i = 0; i < len && oi < outsize - 1; ) {
        if (input[i] == '{') {
            i++; // skip opening brace

            // collect resolved content into temp buffer
            char resolved[256];
            int ri = 0;

            while (i < len && input[i] != '}' && ri < (int)sizeof(resolved) - 1) {
                if (input[i] == '$') {
                    // extract variable name
                    int start = i + 1;
                    int end = start;
                    while (end < len && input[end] != '}' &&
                           input[end] != '+' && input[end] != ' ' &&
                           (isalnum(input[end]) || input[end] == '-' ||
                            input[end] == '_'))
                        end++;

                    char varname[128] = {0};
                    int vlen = end - start;
                    if (vlen > 0 && vlen < (int)sizeof(varname)) {
                        strncpy(varname, input + start, vlen);
                        varname[vlen] = '\0';

                        bool found = false;
                        for (int v = 0; v < kb_var_count; v++) {
                            if (strcmp(kb_vars[v].name, varname) == 0) {
                                int rlen = strlen(kb_vars[v].value);
                                if (ri + rlen < (int)sizeof(resolved) - 1) {
                                    memcpy(resolved + ri, kb_vars[v].value, rlen);
                                    ri += rlen;
                                }
                                found = true;
                                break;
                            }
                        }
                        if (found) {
                            i = end;
                            continue;
                        }
                    }
                    resolved[ri++] = input[i++];
                } else {
                    resolved[ri++] = input[i++];
                }
            }
            resolved[ri] = '\0';

            // skip closing brace
            if (i < len && input[i] == '}') i++;

            // humanize key names in resolved content
            kb_humanize_keys(resolved, sizeof(resolved));

            // copy resolved content without braces
            int rlen = strlen(resolved);
            if (oi + rlen < outsize - 1) {
                memcpy(output + oi, resolved, rlen);
                oi += rlen;
            }
        } else {
            output[oi++] = input[i++];
        }
    }
    output[oi] = '\0';
}

// parse variables file
static void kb_load_vars(void)
{
    char path[512];
    kb_config_path(path, sizeof(path), "variables");
    kb_var_count = 0;

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[KB_MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#') continue;

        // parse "set $name value"
        if (strncmp(p, "set ", 4) != 0) continue;
        p += 4;
        while (*p == ' ') p++;
        if (*p != '$') continue;
        p++;

        const char *name_start = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        int nlen = p - name_start;
        if (nlen <= 0 || nlen >= 128) continue;

        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') continue;

        if (kb_var_count < KB_MAX_VARS) {
            strncpy(kb_vars[kb_var_count].name, name_start, nlen);
            kb_vars[kb_var_count].name[nlen] = '\0';
            strncpy(kb_vars[kb_var_count].value, p, 255);
            kb_vars[kb_var_count].value[255] = '\0';
            kb_var_count++;
        }
    }
    fclose(f);
}

// scan a single config file for # [l] and # [L] comments
static void kb_parse_file(const char *filename)
{
    char path[512];
    kb_config_path(path, sizeof(path), filename);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[KB_MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        // look for # [L] or # [l] anywhere in the line
        // can be standalone comment or inline after a command
        const char *tag = strstr(p, "# [L]");
        if (!tag) tag = strstr(p, "# [l]");
        if (!tag) continue;

        bool is_header = (tag[3] == 'L');

        // extract text after "# [L] " or "# [l] "
        const char *text = tag + 5;
        while (*text == ' ') text++;
        if (*text == '\0') continue;

        if (kb_entry_count >= KB_MAX_ENTRIES) continue;

        KbEntry *e = &kb_entries[kb_entry_count++];
        memset(e, 0, sizeof(*e));
        e->is_header = is_header;
        strncpy(e->raw, text, sizeof(e->raw) - 1);
        strncpy(e->source, filename, sizeof(e->source) - 1);

        // resolve {$variables} in text
        kb_resolve_braces(text, e->text, sizeof(e->text));
    }
    fclose(f);
}

// discover config files in conf.d
static void kb_discover_files(void)
{
    kb_file_count = 0;
    char dir[512];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(dir, sizeof(dir), "%s/.config/sway/conf.d", home);

    // read directory
    char cmd[600];
    snprintf(cmd, sizeof(cmd),
        "ls -1 %s 2>/dev/null", dir);
    FILE *p = popen(cmd, "r");
    if (!p) return;

    char name[256];
    while (fgets(name, sizeof(name), p)) {
        int len = strlen(name);
        while (len > 0 && (name[len-1] == '\n' || name[len-1] == '\r'))
            name[--len] = '\0';
        if (len == 0) continue;
        // skip hidden files
        if (name[0] == '.') continue;
        if (kb_file_count < KB_MAX_FILES) {
            strncpy(kb_files[kb_file_count], name, 255);
            kb_files[kb_file_count][255] = '\0';
            kb_file_count++;
        }
    }
    pclose(p);
}

// load everything
static void kb_load_all(void)
{
    kb_entry_count = 0;
    kb_load_vars();
    kb_discover_files();

    for (int i = 0; i < kb_file_count; i++)
        kb_parse_file(kb_files[i]);

    char msg[64];
    snprintf(msg, sizeof(msg), "loaded %d entries from %d files",
             kb_entry_count, kb_file_count);
    kb_set_status(msg);
}

// case-insensitive substring match
static bool kb_match(const char *haystack, const char *needle)
{
    if (!needle[0]) return true;
    int nlen = strlen(needle);
    int hlen = strlen(haystack);
    for (int i = 0; i <= hlen - nlen; i++) {
        bool ok = true;
        for (int j = 0; j < nlen; j++) {
            if (tolower(haystack[i + j]) != tolower(needle[j])) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

static bool kb_entry_matches(const KbEntry *e, const char *search)
{
    if (!search[0]) return true;
    if (e->is_header) return true;
    return kb_match(e->text, search) ||
           kb_match(e->raw, search) ||
           kb_match(e->source, search);
}

#define KB_ROW_H   24
#define KB_HDR_H   30
#define KB_LIST_Y  44
#define KB_LIST_H  440

int app_keybind(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 700;
    int win_h = 540;
    core_init("l keybind", win_w, win_h);
    kb_load_all();

    while (!WindowShouldClose()) {
        // scroll
        float wheel = GetMouseWheelMove();
        kb_scroll -= wheel * 30.0f;
        if (kb_scroll < 0) kb_scroll = 0;

        // compute total visible height
        float total_h = 0;
        for (int i = 0; i < kb_entry_count; i++) {
            KbEntry *e = &kb_entries[i];
            if (e->is_header) {
                // show header only if it has visible children
                bool has_child = false;
                for (int j = i + 1; j < kb_entry_count; j++) {
                    if (kb_entries[j].is_header) break;
                    if (kb_entry_matches(&kb_entries[j], kb_search)) {
                        has_child = true;
                        break;
                    }
                }
                if (has_child || !kb_search[0])
                    total_h += KB_HDR_H;
            } else if (kb_entry_matches(e, kb_search)) {
                total_h += KB_ROW_H;
            }
        }
        float max_scroll = total_h - KB_LIST_H;
        if (max_scroll < 0) max_scroll = 0;
        if (kb_scroll > max_scroll) kb_scroll = max_scroll;

        BeginDrawing();
        ClearBackground(GRV_BG);

        // title
        DrawTextEx(g_font, "keybind", (Vector2){ 16, 10 }, 20, 1, GRV_AQUA);

        // search
        DrawTextEx(g_font, "search:", (Vector2){ 120, 14 }, 15, 1, GRV_FG);
        if (GuiTextBox((Rectangle){ 190, 10, 300, 24 }, kb_search,
                       KB_MAX_SEARCH, kb_search_editing))
            kb_search_editing = !kb_search_editing;

        // reload
        if (GuiButton((Rectangle){ 510, 10, 80, 24 }, "Reload")) {
            kb_load_all();
            kb_scroll = 0;
        }

        // count
        {
            int vis = 0;
            for (int i = 0; i < kb_entry_count; i++)
                if (!kb_entries[i].is_header &&
                    kb_entry_matches(&kb_entries[i], kb_search))
                    vis++;
            char buf[32];
            snprintf(buf, sizeof(buf), "[%d]", vis);
            DrawTextEx(g_font, buf, (Vector2){ 610, 14 }, 15, 1, GRV_BG3);
        }

        // list bg
        DrawRectangle(8, KB_LIST_Y, win_w - 16, KB_LIST_H,
                      (Color){ 50, 48, 47, 255 });

        BeginScissorMode(8, KB_LIST_Y, win_w - 16, KB_LIST_H);

        float y = KB_LIST_Y + 4 - kb_scroll;
        int row_alt = 0;

        for (int i = 0; i < kb_entry_count; i++) {
            KbEntry *e = &kb_entries[i];

            if (e->is_header) {
                bool has_child = false;
                for (int j = i + 1; j < kb_entry_count; j++) {
                    if (kb_entries[j].is_header) break;
                    if (kb_entry_matches(&kb_entries[j], kb_search)) {
                        has_child = true;
                        break;
                    }
                }
                if (!has_child && kb_search[0]) continue;

                if (y + KB_HDR_H > KB_LIST_Y && y < KB_LIST_Y + KB_LIST_H) {
                    DrawRectangle(12, (int)y, win_w - 24, KB_HDR_H - 4,
                                  (Color){ 60, 56, 54, 255 });
                    DrawTextEx(g_font, "[L]",
                               (Vector2){ 18, y + 5 }, 15, 1, GRV_ORANGE);
                    DrawTextEx(g_font, e->text,
                               (Vector2){ 54, y + 5 }, 15, 1, GRV_YELLOW);
                    // source on the right
                    DrawTextEx(g_font, e->source,
                               (Vector2){ (float)win_w - 180, y + 7 },
                               12, 1, GRV_BG3);
                }
                y += KB_HDR_H;
                row_alt = 0;
                continue;
            }

            if (!kb_entry_matches(e, kb_search)) continue;

            if (y + KB_ROW_H > KB_LIST_Y && y < KB_LIST_Y + KB_LIST_H) {
                if (row_alt % 2 == 0)
                    DrawRectangle(12, (int)y, win_w - 24, KB_ROW_H - 2,
                                  (Color){ 45, 43, 42, 255 });

                // [l] tag
                DrawTextEx(g_font, "[l]",
                           (Vector2){ 18, y + 3 }, 13, 1, GRV_AQUA);
                // resolved text
                DrawTextEx(g_font, e->text,
                           (Vector2){ 50, y + 3 }, 14, 1, GRV_FG);
            }
            y += KB_ROW_H;
            row_alt++;
        }

        EndScissorMode();

        // status
        if (kb_status_timer > 0) {
            kb_status_timer--;
            DrawTextEx(g_font, kb_status,
                       (Vector2){ 16, (float)win_h - 24 }, 15, 1, GRV_YELLOW);
        }

        EndDrawing();
    }

    core_close();
    return 0;
}
