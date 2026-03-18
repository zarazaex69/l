#include <stdio.h>
#include <string.h>
#include <math.h>
#include "core.h"
#include "calc.h"

#define CALC_DISPLAY_MAX 64
#define CALC_INPUT_MAX   16

typedef enum {
    OP_NONE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
} CalcOp;

typedef struct {
    char display[CALC_DISPLAY_MAX];
    double accumulator;
    CalcOp pending_op;
    bool reset_on_next;
    bool has_dot;
    bool error;
} CalcState;

static void calc_reset(CalcState *s)
{
    s->display[0] = '0';
    s->display[1] = '\0';
    s->accumulator = 0.0;
    s->pending_op = OP_NONE;
    s->reset_on_next = false;
    s->has_dot = false;
    s->error = false;
}

static void calc_append_digit(CalcState *s, char digit)
{
    if (s->error) return;
    if (s->reset_on_next) {
        s->display[0] = '\0';
        s->reset_on_next = false;
        s->has_dot = false;
    }
    size_t len = strlen(s->display);
    if (len == 1 && s->display[0] == '0' && digit != '.') {
        s->display[0] = digit;
        return;
    }
    if (digit == '.') {
        if (s->has_dot) return;
        s->has_dot = true;
    }
    if (len >= CALC_INPUT_MAX) return;
    s->display[len] = digit;
    s->display[len + 1] = '\0';
}

static double calc_display_value(CalcState *s)
{
    double v = 0.0;
    sscanf(s->display, "%lf", &v);
    return v;
}

static void calc_apply_op(CalcState *s)
{
    double current = calc_display_value(s);
    switch (s->pending_op) {
    case OP_ADD: s->accumulator += current; break;
    case OP_SUB: s->accumulator -= current; break;
    case OP_MUL: s->accumulator *= current; break;
    case OP_DIV:
        if (current == 0.0) { s->error = true; return; }
        s->accumulator /= current;
        break;
    case OP_NONE: s->accumulator = current; break;
    }
}

static void calc_set_op(CalcState *s, CalcOp op)
{
    if (s->error) return;
    calc_apply_op(s);
    s->pending_op = op;
    s->reset_on_next = true;
}

static void calc_equals(CalcState *s)
{
    if (s->error) return;
    calc_apply_op(s);
    s->pending_op = OP_NONE;
    if (s->error) {
        snprintf(s->display, CALC_DISPLAY_MAX, "ERR");
    } else {
        if (s->accumulator == (double)(long long)s->accumulator &&
            fabs(s->accumulator) < 1e15) {
            snprintf(s->display, CALC_DISPLAY_MAX, "%lld", (long long)s->accumulator);
        } else {
            snprintf(s->display, CALC_DISPLAY_MAX, "%.10g", s->accumulator);
        }
    }
    s->reset_on_next = true;
    s->has_dot = (strchr(s->display, '.') != NULL);
}

static void calc_negate(CalcState *s)
{
    if (s->error) return;
    if (s->display[0] == '0' && s->display[1] == '\0') return;
    if (s->display[0] == '-') {
        memmove(s->display, s->display + 1, strlen(s->display));
    } else {
        size_t len = strlen(s->display);
        if (len < CALC_DISPLAY_MAX - 1) {
            memmove(s->display + 1, s->display, len + 1);
            s->display[0] = '-';
        }
    }
}

static void calc_percent(CalcState *s)
{
    if (s->error) return;
    double v = calc_display_value(s);
    v = s->accumulator * v / 100.0;
    snprintf(s->display, CALC_DISPLAY_MAX, "%.10g", v);
    s->has_dot = (strchr(s->display, '.') != NULL);
}

int app_calc(int argc, char **argv)
{
    (void)argc; (void)argv;

    int win_w = 320;
    int win_h = 480;
    core_init("l calc", win_w, win_h);

    CalcState state;
    calc_reset(&state);

    int pad = 10;
    int display_h = 60;
    int btn_rows = 5;
    int btn_cols = 4;
    float btn_w = (float)(win_w - pad * (btn_cols + 1)) / btn_cols;
    float btn_h = (float)(win_h - display_h - pad * (btn_rows + 2)) / btn_rows;

    const char *labels[5][4] = {
        { "C",  "+/-", "%",  "/" },
        { "7",  "8",   "9",  "*" },
        { "4",  "5",   "6",  "-" },
        { "1",  "2",   "3",  "+" },
        { "0",  ".",   "=",  "<" },
    };

    while (!WindowShouldClose()) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= '0' && key <= '9') calc_append_digit(&state, (char)key);
            else if (key == '.') calc_append_digit(&state, '.');
            else if (key == '+') calc_set_op(&state, OP_ADD);
            else if (key == '-') calc_set_op(&state, OP_SUB);
            else if (key == '*') calc_set_op(&state, OP_MUL);
            else if (key == '/') calc_set_op(&state, OP_DIV);
            else if (key == '=' || key == '\r' || key == '\n') calc_equals(&state);
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) calc_equals(&state);
        if (IsKeyPressed(KEY_BACKSPACE)) {
            size_t len = strlen(state.display);
            if (len > 1) {
                if (state.display[len - 1] == '.') state.has_dot = false;
                state.display[len - 1] = '\0';
            } else {
                state.display[0] = '0';
                state.display[1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            calc_reset(&state);
        }

        BeginDrawing();
        ClearBackground(GRV_BG);

        Rectangle disp_rect = { (float)pad, (float)pad, (float)(win_w - pad * 2), (float)display_h };
        DrawRectangleRec(disp_rect, GRV_BG1);
        DrawRectangleLinesEx(disp_rect, 1, GRV_BG3);

        const char *disp_text = state.error ? "ERR: div/0" : state.display;
        float max_text_w = disp_rect.width - 20;
        int font_size = 28;
        Vector2 text_size = MeasureTextEx(g_font, disp_text, (float)font_size, 1);
        while (text_size.x > max_text_w && font_size > 10) {
            font_size--;
            text_size = MeasureTextEx(g_font, disp_text, (float)font_size, 1);
        }
        float text_x = disp_rect.x + disp_rect.width - text_size.x - 10;
        float text_y = disp_rect.y + (disp_rect.height - text_size.y) / 2;
        Color text_color = state.error ? GRV_RED : GRV_FG0;
        DrawTextEx(g_font, disp_text, (Vector2){ text_x, text_y }, (float)font_size, 1, text_color);

        float start_y = (float)(display_h + pad * 2);
        for (int row = 0; row < btn_rows; row++) {
            for (int col = 0; col < btn_cols; col++) {
                float x = (float)(pad + col * (int)(btn_w + pad));
                float y = start_y + (float)(row * (int)(btn_h + pad));
                Rectangle r = { x, y, btn_w, btn_h };

                bool is_op = (col == 3) || (row == 0 && col > 0);
                bool is_clear = (row == 0 && col == 0);
                bool is_equals = (row == 4 && col == 2);

                if (is_op) {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, grv_color(GRV_BG3));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, grv_color(GRV_ORANGE));
                    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_ORANGE));
                } else if (is_clear) {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, grv_color(GRV_RED));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, grv_color(GRV_FG0));
                    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_RED));
                } else if (is_equals) {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, grv_color(GRV_GREEN));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, grv_color(GRV_BG));
                    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_GREEN));
                } else {
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, grv_color(GRV_BG2));
                    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, grv_color(GRV_FG));
                    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, grv_color(GRV_AQUA));
                }

                if (GuiButton(r, labels[row][col])) {
                    const char *lbl = labels[row][col];
                    if (lbl[0] >= '0' && lbl[0] <= '9') calc_append_digit(&state, lbl[0]);
                    else if (lbl[0] == '.') calc_append_digit(&state, '.');
                    else if (lbl[0] == '+' && lbl[1] == '/') calc_negate(&state);
                    else if (lbl[0] == '%') calc_percent(&state);
                    else if (lbl[0] == 'C') calc_reset(&state);
                    else if (lbl[0] == '+') calc_set_op(&state, OP_ADD);
                    else if (lbl[0] == '-') calc_set_op(&state, OP_SUB);
                    else if (lbl[0] == '*') calc_set_op(&state, OP_MUL);
                    else if (lbl[0] == '/') calc_set_op(&state, OP_DIV);
                    else if (lbl[0] == '=') calc_equals(&state);
                    else if (lbl[0] == '<') {
                        size_t len = strlen(state.display);
                        if (len > 1) {
                            if (state.display[len - 1] == '.') state.has_dot = false;
                            state.display[len - 1] = '\0';
                        } else {
                            state.display[0] = '0';
                            state.display[1] = '\0';
                        }
                    }
                }
            }
        }

        apply_gruvbox_style();
        EndDrawing();
    }

    core_close();
    return 0;
}
