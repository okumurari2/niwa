/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (the layout: maku ui_maku.c mq_macro / mq_gauge)
 * Copyright (C) 2026 okumurari2 (made general) */
/* The 2 x 2 screen of docs/UI.md: 240 x 240 in four 119 x 119 quadrants, laid out like the four knobs at the left of the panel.
 *
 *   top left      "now": whatever the instrument is doing. The app draws it (any canvas of up to 119 x 119, blit at 0, 0)
 *   top right     SELECT      quad_draw(Q_SELECT, ..)
 *   bottom left   PRESETS     quad_draw(Q_PRESETS, ..)
 *   bottom right  ALGORITHM   quad_draw(Q_ALGORITHM, ..)
 *
 * One style for every parameter and every mode (UI.md 3): the knob's name, the parameter's name, the value, a gauge. What
 * changes is the content, never the layout. Include after gfx.c. Draw a quadrant only when something it shows changed
 * (UI.md 7: a signature of the shown values; quad_mix builds one). */
#ifndef QUAD_H
#define QUAD_H

#define QUAD 119                                 /* a quadrant is 119 x 119; the dividers are the 2 px between */
enum { Q_SELECT = 0, Q_PRESETS = 1, Q_ALGORITHM = 2 };
static const char *const QUAD_KNOB[3] = {"SELECT", "PRESETS", "ALGORITHM"};

typedef struct {
    const char *name;        /* the parameter: a capital English word of up to 7 letters; the label of a printed button as is */
    int32_t value;           /* 0..127 (docs/UI.md 1.6): the number shown and the gauge. Map others onto it first */
    const char *text;        /* what is shown instead of the number: letters of AF_L (" #+-./0-9A-Y", up to 7), or 0 */
    uint16_t col;            /* the value's and the gauge's colour: the track's colour, T_THEME if none */
    const char *mode;        /* the verb or mode held (SELECT's quadrant only), at the top right in T_ACCENT, or 0 */
} quad_param_t;

/* where quadrant q (Q_*) sits on the screen */
static inline uint32_t quad_x(uint32_t q) { return q == Q_PRESETS ? 0u : 121u; }
static inline uint32_t quad_y(uint32_t q) { return q == Q_SELECT ? 0u : 121u; }

static void quad_gauge(int32_t x, int32_t y, int32_t w, int32_t v, uint16_t c)
{
    cv_rect(x, y, w, 4, T_LINE);
    cv_rect(x, y, w * (v < 0 ? 0 : v > 127 ? 127 : v) / 127, 4, c);
}

/* the number v as text (0..999, a minus sign below 0) */
static const char *quad_num(char *b, int32_t v)       /* b: 5 bytes or more */
{
    uint32_t i = 0, u = (uint32_t)(v < 0 ? -v : v);
    if (v < 0)
        b[i++] = '-';
    if (u >= 100u)
        b[i++] = (char)('0' + u / 100u % 10u);
    if (u >= 10u)
        b[i++] = (char)('0' + u / 10u % 10u);
    b[i++] = (char)('0' + u % 10u);
    b[i] = 0;
    return b;
}

/* quadrant q (Q_*) with the parameter p, drawn off screen and sent at once */
static void quad_draw(uint32_t q, const quad_param_t *p)
{
    char b[6];
    cv_begin(QUAD, QUAD, T_BG);
    cv_text(8, 6, &AF_S, QUAD_KNOB[q], T_DIM);
    if (p->mode)
        cv_text(QUAD - 6 - text_w(&AF_S, p->mode), 6, &AF_S, p->mode, T_ACCENT);
    cv_text(8, 24, &AF_M, p->name, T_MID);
    cv_text_in(0, 48, QUAD, &AF_L, p->text ? p->text : quad_num(b, p->value), p->col, T_BG);
    quad_gauge(8, 92, QUAD - 16, p->value, p->col);
    cv_blit(quad_x(q), quad_y(q));
}

/* the two 2 px dividers; once after the screen was cleared (they are not part of a quadrant) */
static void quad_dividers(void)
{
    lcd_fill(119, 0, 2, 240, T_LINE);
    lcd_fill(0, 119, 240, 2, T_LINE);
}

/* a signature of what a quadrant shows: s = quad_mix(s, each value) */
static inline uint32_t quad_mix(uint32_t s, uint32_t v) { return s * 31u + v; }

#endif
