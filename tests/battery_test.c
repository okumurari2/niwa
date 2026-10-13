/* SPDX-License-Identifier: GPL-3.0-only */
/* cc -O2 -Wall -Ifirmware/src -o build/battery_test tests/battery_test.c && build/battery_test */
#include <stdio.h>
#include "battery.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    uint32_t t;
    int n, got;
    CHECK(battery_level(600) == 3 && battery_level(590) == 2 && battery_level(561) == 2 && battery_level(560) == 1 &&
          battery_level(531) == 1 && battery_level(530) == 0);
    CHECK(battery_filter(0, 540) == 540 && battery_filter(540, 0) == 540 && battery_filter(540, 572) == 541);

    /* fine, or unknown, or a host powers the unit: nothing, ever */
    for (t = 1; t < 400000; t += 100)
        CHECK(battery_notice(t, 600, 0, 0) == BATT_NONE && battery_notice(t, 0, 0, 0) == BATT_NONE);
    for (t = 400001; t < 800000; t += 100)
        CHECK(battery_notice(t, 500, 1, 0) == BATT_NONE);

    /* 1 bar: after it has held 5 s, once; again after 2 minutes, not before */
    t = 1000000;
    got = 0;
    for (n = 0; n < 4000; n++, t += 100) {              /* 400 s */
        int r = battery_notice(t, 540, 0, 0);
        if (r) {
            CHECK(r == BATT_LOW);
            if (got == 0)
                CHECK(t - 1000000 >= 5000 && t - 1000000 < 5200);
            got++;
        }
    }
    CHECK(got == 4);                                    /* 5 s, then +120 s three times in 400 s */

    /* it drops a bar: at once (not waiting for the 2 minutes) */
    CHECK(battery_notice(t, 500, 0, 0) == BATT_EMPTY);
    CHECK(battery_notice(t + 100, 500, 0, 0) == BATT_NONE);

    /* it recovers and the clock starts again: 5 s of holding before the next */
    CHECK(battery_notice(t + 1000, 600, 0, 0) == BATT_NONE);
    CHECK(battery_notice(t + 2000, 540, 0, 0) == BATT_NONE);
    CHECK(battery_notice(t + 6900, 540, 0, 0) == BATT_NONE);
    CHECK(battery_notice(t + 7100, 540, 0, 1) == BATT_NONE);   /* another message is showing: wait */
    CHECK(battery_notice(t + 7200, 540, 0, 0) == BATT_LOW);

    /* the millisecond counter wraps: the 5 s and the 2 min are differences */
    {
        uint32_t w = 0xFFFFFF00u;
        battery_notice(w - 1000, 600, 0, 0);
        CHECK(battery_notice(w, 540, 0, 0) == BATT_NONE);
        CHECK(battery_notice(w + 5100u, 540, 0, 0) == BATT_LOW);        /* 5.1 s later, across the wrap */
    }

    printf(fails ? "battery_test: %d FAILED\n" : "battery_test: ok\n", fails);
    return fails != 0;
}
