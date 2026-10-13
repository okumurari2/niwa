/* SPDX-License-Identifier: GPL-3.0-only */
/* Four volumes and one focus (docs/UI.md 2): KNOB 1..4 are the volumes of tracks 0..3, and the knob turned last is the
 * focused track. SELECT / PRESETS / ALGORITHM then belong to that track: the app reads trk_focus to know whose parameters
 * they move. No sound here; what a track is (a voice, a drum, a loop) is the app's. Volumes are 0..127, 2 per detent.
 *
 * The logic needs nothing (tests/tracks_test.c compiles it alone with TRACKS_LOGIC_ONLY). The strip needs gfx.c.
 * Volumes are a preference and may be saved; the focus is not, it is where the hands are. */
#ifndef TRACKS_H
#define TRACKS_H

#define TRK_N 4u

/* detents[i]: what KNOB i+1 turned since the last call. The volumes move; the focus follows the knob that was turned
 * (the highest, if two at once). Returns a bit mask of the tracks whose volume changed (0 = nothing: skip the redraw).
 * A knob already at its end still moves the focus: the hand is on that track. */
static uint32_t trk_turn(uint8_t vol[TRK_N], const int32_t detents[TRK_N], uint32_t *focus)
{
    uint32_t i, moved = 0;
    for (i = 0; i < TRK_N; i++) {
        int32_t v;
        if (!detents[i])
            continue;
        v = (int32_t)vol[i] + detents[i] * 2;
        v = v < 0 ? 0 : v > 127 ? 127 : v;
        if (v != vol[i])
            moved |= 1u << i;
        vol[i] = (uint8_t)v;
        *focus = i;
    }
    return moved;
}

#ifndef TRACKS_LOGIC_ONLY
/* The strip: four bars side by side in the canvas, x .. x + w. The focused track in its colour with a 2 px line under it,
 * the others sunk toward the background, not hidden (UI.md 5.2). col[i]: the track's colour (T_THEME for none).
 * Draw it into the canvas of the top left quadrant (cv_begin first, cv_blit after). */
static void trk_strip(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t vol[TRK_N], const uint16_t col[TRK_N],
                      uint32_t focus)
{
    int32_t gap = 6, bw = (w - gap * (int32_t)(TRK_N - 1u)) / (int32_t)TRK_N, i;
    for (i = 0; i < (int32_t)TRK_N; i++) {
        int32_t bx = x + i * (bw + gap), fill = (h - 6) * vol[i] / 127;
        uint16_t c = (uint32_t)i == focus ? col[i] : ux_mix(col[i], T_BG, 55);
        cv_rect(bx, y, bw, h - 6, T_LINE);
        cv_rect(bx, y + h - 6 - fill, bw, fill, c);
        if ((uint32_t)i == focus)
            cv_rect(bx, y + h - 2, bw, 2, c);
    }
}
#endif

#endif
