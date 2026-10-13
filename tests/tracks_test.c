/* SPDX-License-Identifier: GPL-3.0-only */
/* tracks.h (the logic): cc -O2 -Wall -DTRACKS_LOGIC_ONLY -Ifirmware/src -o build/tracks_test tests/tracks_test.c */
#include <stdint.h>
#include <stdio.h>
#include "tracks.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    uint8_t v[TRK_N] = {100, 100, 100, 100};
    int32_t none[TRK_N] = {0, 0, 0, 0}, d[TRK_N] = {0, 0, 0, 0};
    uint32_t f = 0, m;

    m = trk_turn(v, none, &f);                          /* nothing turned: nothing moves, the focus stays */
    CHECK(m == 0 && f == 0 && v[0] == 100);
    d[2] = -5;                                          /* KNOB 3: 2 per detent, and the focus goes there */
    m = trk_turn(v, d, &f);
    CHECK(m == 1u << 2 && f == 2 && v[2] == 90 && v[0] == 100 && v[1] == 100 && v[3] == 100);
    d[2] = 0;
    m = trk_turn(v, none, &f);                          /* turning stopped: the focus stays on track 3 */
    CHECK(m == 0 && f == 2);
    d[1] = 100;                                         /* far past the top: clamped at 127, focus still follows */
    m = trk_turn(v, d, &f);
    CHECK(v[1] == 127 && f == 1 && m == 1u << 1);
    m = trk_turn(v, d, &f);                             /* already at the end: no volume change, the hand is still there */
    CHECK(m == 0 && f == 1 && v[1] == 127);
    d[1] = -1000;
    trk_turn(v, d, &f);
    CHECK(v[1] == 0);
    d[0] = 1; d[3] = 1; d[1] = 0;                       /* two at once: the higher knob takes the focus */
    m = trk_turn(v, d, &f);
    CHECK(m == (1u | 1u << 3) && f == 3 && v[0] == 102 && v[3] == 102);
    if (!fails)
        printf("tracks: ok\n");
    return fails != 0;
}
