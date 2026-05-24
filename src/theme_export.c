#include "theme_export.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

static const char *home_dir(void)
{
    const char *h = getenv("HOME");
    return (h && *h) ? h : "/tmp";
}

static char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    if (out_len) *out_len = (size_t)sz;
    return buf;
}

static int write_file(const char *path, const char *data, size_t len)
{
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return -1;
    if (fwrite(data, 1, len, f) != len) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return rename(tmp, path);
}

static void mkdir_p(const char *path)
{
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static void hex_str(char *out, Color c)
{
    snprintf(out, 8, "%02x%02x%02x", c.r, c.g, c.b);
}

static void hex_upper(char *out, Color c)
{
    snprintf(out, 8, "%02X%02X%02X", c.r, c.g, c.b);
}

int theme_export_sway(const Theme *t)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.config/sway/conf.d", home_dir());
    mkdir_p(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/theme-colors", dir);

    char hbg[8], hfg[8], hacc[8];
    hex_str(hbg, t->bg);
    hex_str(hfg, t->fg);
    hex_str(hacc, t->orange);

    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "set $bg #%s\n"
        "set $fg #%s\n"
        "set $accent #%s\n"
        "\n"
        "client.focused          $accent $accent $fg $accent $accent\n"
        "client.unfocused        $bg $bg $fg $bg $bg\n",
        hbg, hfg, hacc);

    return write_file(path, buf, (size_t)n);
}

int theme_export_waybar(const Theme *t)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.config/waybar", home_dir());
    mkdir_p(dir);

    char colors_path[512];
    snprintf(colors_path, sizeof(colors_path), "%s/theme-colors.css", dir);

    char hbg0[8], hbg1[8], hbg2[8], hfg0[8], hfg1[8], hgray[8];
    char hred[8], hgreen[8], hyellow[8], hblue[8], hpurple[8], haqua[8], horange[8];
    hex_str(hbg0, t->bg);
    hex_str(hbg1, t->bg1);
    hex_str(hbg2, t->bg2);
    hex_str(hfg0, t->fg);
    hex_str(hfg1, t->fg0);
    hex_str(hgray, t->gray);
    hex_str(hred, t->red);
    hex_str(hgreen, t->green);
    hex_str(hyellow, t->yellow);
    hex_str(hblue, t->blue);
    hex_str(hpurple, t->purple);
    hex_str(haqua, t->aqua);
    hex_str(horange, t->orange);

    char buf[2048];
    int n = snprintf(buf, sizeof(buf),
        "@define-color bg0 #%s;\n"
        "@define-color bg1 #%s;\n"
        "@define-color bg2 #%s;\n"
        "@define-color fg0 #%s;\n"
        "@define-color fg1 #%s;\n"
        "@define-color gray #%s;\n"
        "@define-color red #%s;\n"
        "@define-color green #%s;\n"
        "@define-color yellow #%s;\n"
        "@define-color blue #%s;\n"
        "@define-color purple #%s;\n"
        "@define-color aqua #%s;\n"
        "@define-color orange #%s;\n",
        hbg0, hbg1, hbg2, hfg0, hfg1, hgray,
        hred, hgreen, hyellow, hblue, hpurple, haqua, horange);

    if (write_file(colors_path, buf, (size_t)n) != 0) return -1;

    char style_path[512];
    snprintf(style_path, sizeof(style_path), "%s/style.css", dir);

    size_t src_len = 0;
    char *src = read_file(style_path, &src_len);
    if (!src) return 0;

    if (strstr(src, "theme-colors.css")) {
        free(src);
        return 0;
    }

    char *first_define = strstr(src, "@define-color");
    char *out;
    size_t out_len;

    const char import_line[] = "@import url(\"theme-colors.css\");\n";
    size_t import_len = strlen(import_line);

    if (first_define) {
        char *line_start = first_define;
        while (line_start > src && *(line_start - 1) != '\n') line_start--;

        char *p = first_define;
        char *block_end = first_define;
        while (*p) {
            char *line_end = strchr(p, '\n');
            char *ls = p;
            while (*ls == ' ' || *ls == '\t') ls++;
            if (strncmp(ls, "@define-color", 13) == 0) {
                block_end = line_end ? line_end + 1 : p + strlen(p);
                if (!line_end) break;
                p = line_end + 1;
            } else if (ls == line_end || *ls == '\n' || *ls == '\0') {
                if (!line_end) break;
                p = line_end + 1;
            } else {
                break;
            }
        }

        size_t prefix = (size_t)(line_start - src);
        size_t suffix = strlen(block_end);
        out_len = prefix + import_len + suffix;
        out = malloc(out_len + 1);
        memcpy(out, src, prefix);
        memcpy(out + prefix, import_line, import_len);
        memcpy(out + prefix + import_len, block_end, suffix);
        out[out_len] = '\0';
    } else {
        out_len = import_len + src_len;
        out = malloc(out_len + 1);
        memcpy(out, import_line, import_len);
        memcpy(out + import_len, src, src_len);
        out[out_len] = '\0';
    }

    int rc = write_file(style_path, out, out_len);
    free(out);
    free(src);
    return rc;
}

static int patch_section_replace(char *src, size_t src_len,
                                 const char *section_header,
                                 const char *new_block,
                                 char **out, size_t *out_len)
{
    char *start = strstr(src, section_header);
    if (!start) return 0;

    char *line_start = start;
    while (line_start > src && *(line_start - 1) != '\n') line_start--;

    char *p = start + strlen(section_header);
    char *end = p;
    while (*p) {
        if (*p == '\n') {
            char *next = p + 1;
            while (*next == ' ' || *next == '\t') next++;
            if (*next == '[') {
                end = p + 1;
                break;
            }
            p = next;
        } else {
            p++;
        }
    }
    if (!*p) end = p;

    size_t new_len = strlen(new_block);
    size_t prefix = (size_t)(line_start - src);
    size_t suffix = strlen(end);
    *out_len = prefix + new_len + suffix;
    *out = malloc(*out_len + 1);
    memcpy(*out, src, prefix);
    memcpy(*out + prefix, new_block, new_len);
    memcpy(*out + prefix + new_len, end, suffix);
    (*out)[*out_len] = '\0';
    return 1;
}

int theme_export_foot(const Theme *t)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/foot/foot.ini", home_dir());

    size_t src_len = 0;
    char *src = read_file(path, &src_len);
    if (!src) return 0;

    char hbg[8], hfg[8];
    char hred[8], hgreen[8], hyellow[8], hblue[8], hpurple[8], haqua[8];
    char hgray[8], hfg0[8], horange[8];

    hex_upper(hbg, t->bg);
    hex_upper(hfg, t->fg);
    hex_upper(hred, t->red);
    hex_upper(hgreen, t->green);
    hex_upper(hyellow, t->yellow);
    hex_upper(hblue, t->blue);
    hex_upper(hpurple, t->purple);
    hex_upper(haqua, t->aqua);
    hex_upper(hgray, t->gray);
    hex_upper(hfg0, t->fg0);
    hex_upper(horange, t->orange);

    char block[1024];
    snprintf(block, sizeof(block),
        "[colors-dark]\n"
        "foreground=%s\n"
        "background=%s\n"
        "regular0=%s\n"
        "regular1=%s\n"
        "regular2=%s\n"
        "regular3=%s\n"
        "regular4=%s\n"
        "regular5=%s\n"
        "regular6=%s\n"
        "regular7=%s\n"
        "bright0=%s\n"
        "bright1=%s\n"
        "bright2=%s\n"
        "bright3=%s\n"
        "bright4=%s\n"
        "bright5=%s\n"
        "bright6=%s\n"
        "bright7=%s\n"
        "cursor=%s %s\n"
        "selection-foreground=%s\n"
        "selection-background=%s\n"
        "\n",
        hfg, hbg,
        hbg, hred, hgreen, hyellow, hblue, hpurple, haqua, hfg,
        hgray, hred, hgreen, hyellow, hblue, hpurple, haqua, hfg0,
        hbg, hfg,
        hbg, hyellow);

    char *out = NULL;
    size_t out_len = 0;
    int rc;
    if (patch_section_replace(src, src_len, "[colors-dark]", block, &out, &out_len)) {
        rc = write_file(path, out, out_len);
        free(out);
    } else {
        size_t total = src_len + strlen(block) + 2;
        char *appended = malloc(total + 1);
        snprintf(appended, total + 1, "%s\n%s", src, block);
        rc = write_file(path, appended, strlen(appended));
        free(appended);
    }
    free(src);
    return rc;
}

static int replace_kv_in_section(char *src, const char *section,
                                 const char *key, const char *new_value,
                                 char **out, size_t *out_len)
{
    char *sec = strstr(src, section);
    if (!sec) return 0;

    char *p = sec + strlen(section);
    char *next_sec = p;
    while (*next_sec) {
        if (*next_sec == '\n' && next_sec[1] == '[') break;
        next_sec++;
    }

    char *kp = NULL;
    char *scan = p;
    while (scan < next_sec) {
        char *line_start = scan;
        while (line_start > src && *(line_start - 1) != '\n' && line_start > p) {
            line_start--;
        }
        char *probe = scan;
        while (*probe == ' ' || *probe == '\t') probe++;
        if (strncmp(probe, key, strlen(key)) == 0) {
            char after = probe[strlen(key)];
            if (after == ' ' || after == '\t' || after == '=') {
                kp = scan;
                break;
            }
        }
        char *nl = strchr(scan, '\n');
        if (!nl || nl >= next_sec) break;
        scan = nl + 1;
    }
    if (!kp) return 0;

    char *line_end = strchr(kp, '\n');
    if (!line_end) line_end = kp + strlen(kp);

    char new_line[256];
    int nl_len = snprintf(new_line, sizeof(new_line), "    %s = %s", key, new_value);

    size_t prefix = (size_t)(kp - src);
    size_t suffix = strlen(line_end);
    *out_len = prefix + (size_t)nl_len + suffix;
    *out = malloc(*out_len + 1);
    memcpy(*out, src, prefix);
    memcpy(*out + prefix, new_line, (size_t)nl_len);
    memcpy(*out + prefix + nl_len, line_end, suffix);
    (*out)[*out_len] = '\0';
    return 1;
}

int theme_export_dunst(const Theme *t)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/.config/dunst/dunstrc", home_dir());

    size_t src_len = 0;
    char *src = read_file(path, &src_len);
    if (!src) return 0;

    char hbg[8], hbg1[8], hbg2[8], hbg3[8], hfg[8], hfg0[8], hgray[8], hred[8], horange[8];
    hex_str(hbg, t->bg);
    hex_str(hbg1, t->bg1);
    hex_str(hbg2, t->bg2);
    hex_str(hbg3, t->bg3);
    hex_str(hfg, t->fg);
    hex_str(hfg0, t->fg0);
    hex_str(hgray, t->gray);
    hex_str(hred, t->red);
    hex_str(horange, t->orange);

    struct {
        const char *section;
        const char *key;
        const char *value;
    } patches[] = {
        { "[global]",          "frame_color", NULL },
        { "[urgency_low]",     "background",  NULL },
        { "[urgency_low]",     "foreground",  NULL },
        { "[urgency_low]",     "frame_color", NULL },
        { "[urgency_normal]",  "background",  NULL },
        { "[urgency_normal]",  "foreground",  NULL },
        { "[urgency_normal]",  "frame_color", NULL },
        { "[urgency_critical]","background",  NULL },
        { "[urgency_critical]","foreground",  NULL },
        { "[urgency_critical]","frame_color", NULL },
    };

    char vbg1[16], vbg[16], vbg2[16], vfg[16], vgray[16], vbg3[16], vred[16], vorange[16], vfg0[16];
    snprintf(vbg1,    sizeof(vbg1),    "\"#%s\"", hbg1);
    snprintf(vbg,     sizeof(vbg),     "\"#%s\"", hbg);
    snprintf(vbg2,    sizeof(vbg2),    "\"#%s\"", hbg2);
    snprintf(vfg,     sizeof(vfg),     "\"#%s\"", hfg);
    snprintf(vgray,   sizeof(vgray),   "\"#%s\"", hgray);
    snprintf(vbg3,    sizeof(vbg3),    "\"#%s\"", hbg3);
    snprintf(vred,    sizeof(vred),    "\"#%s\"", hred);
    snprintf(vorange, sizeof(vorange), "\"#%s\"", horange);
    snprintf(vfg0,    sizeof(vfg0),    "\"#%s\"", hfg0);

    patches[0].value = vbg1;
    patches[1].value = vbg;
    patches[2].value = vgray;
    patches[3].value = vbg1;
    patches[4].value = vbg2;
    patches[5].value = vfg;
    patches[6].value = vbg3;
    patches[7].value = vred;
    patches[8].value = vfg0;
    patches[9].value = vorange;

    char *current = src;
    size_t current_len = src_len;
    int n = sizeof(patches) / sizeof(patches[0]);
    for (int i = 0; i < n; i++) {
        char *out = NULL;
        size_t out_len = 0;
        if (replace_kv_in_section(current, patches[i].section, patches[i].key,
                                  patches[i].value, &out, &out_len)) {
            if (current != src) free(current);
            current = out;
            current_len = out_len;
        }
    }

    int rc = write_file(path, current, current_len);
    if (current != src) free(current);
    free(src);
    return rc;
}

int theme_export_all(const Theme *t)
{
    int rc = 0;
    if (theme_export_sway(t)   != 0) rc = -1;
    if (theme_export_waybar(t) != 0) rc = -1;
    if (theme_export_foot(t)   != 0) rc = -1;
    if (theme_export_dunst(t)  != 0) rc = -1;
    return rc;
}

void theme_reload_system(void)
{
    int unused;
    unused = system("swaymsg reload >/dev/null 2>&1");
    unused = system("pkill -SIGUSR2 waybar >/dev/null 2>&1");
    (void)unused;
}
