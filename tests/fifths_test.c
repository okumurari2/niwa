/* SPDX-License-Identifier: GPL-3.0-only */
/* cc -O2 -Wall -Ifirmware/src -o build/fifths_test tests/fifths_test.c && build/fifths_test */
#include <stdio.h>
#include "fifths.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    uint32_t r, n, k, x;
    int dir;

    CHECK(fifths_step(0, 1) == 7 && fifths_step(0, -1) == 5);                  /* C -> G, C -> F */
    for (r = 0; r < 12; r++) {                                                 /* twelve steps are the identity; up then down too */
        x = r;
        for (k = 0; k < 12; k++)
            x = fifths_step(x, 1);
        CHECK(x == r);
        CHECK(fifths_step(fifths_step(r, 1), -1) == r);
    }
    CHECK(fifths_signature(0) == 0 && fifths_signature(7) == 1 && fifths_signature(2) == 2 && fifths_signature(5) == -1);
    CHECK(fifths_signature(10) == -2 && fifths_signature(6) == 6 && fifths_signature(11) == 5);
    CHECK(fifths_resig(65, 0, 7) == 66);                                       /* C -> G: F becomes F# */
    CHECK(fifths_resig(71, 0, 5) == 70);                                       /* C -> F: B becomes Bb */
    for (r = 0; r < 12; r++)
        for (dir = -1; dir <= 1; dir += 2) {
            uint32_t to = fifths_step(r, dir), moved = 0;
            for (n = 1; n < 127; n++) {                                   /* (0 and 127 cannot move outwards) */
                uint32_t m = fifths_resig(n, r, to);
                if (fifths_in_key(n, r))
                    CHECK(fifths_in_key(m, to));                               /* nothing in the old key is out of the new */
                else
                    CHECK(m == n);                                             /* a note off the scale stays as played */
                if (m != n) {
                    CHECK((m > n) == (dir > 0));                               /* moves the way the signature goes, a semitone */
                    CHECK((m > n ? m - n : n - m) == 1);
                    if (n >= 12 && n < 24)
                        moved++;
                }
            }
            CHECK(moved == 1);                                                 /* one pitch class changes per step */
        }
    for (r = 0; r < 12; r++)                                                   /* degrees are always in the key */
        for (k = 0; k < 40; k++)
            CHECK(fifths_in_key(fifths_degree(36 + r, k), r));
    CHECK(fifths_degree(48, 0) == 48 && fifths_degree(48, 2) == 52 && fifths_degree(48, 7) == 60 && fifths_degree(48, 9) == 64);

    printf(fails ? "fifths_test: %d FAILED\n" : "fifths_test: ok\n", fails);
    return fails != 0;
}
