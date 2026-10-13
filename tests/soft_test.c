/* SPDX-License-Identifier: GPL-3.0-only */
/* SOFT (firmware/src/soft.h): the power-on swell and the dip around a flash erase. A simulated audio ISR: one block of 256
 * frames every 5.8 ms (6 ms of fm1_ms here). cc -O2 -Wall -Ifirmware/src -o build/soft_test tests/soft_test.c */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FS 44100u
static volatile uint32_t fm1_ms;
#define N 256u
static int32_t blk[2 * N];
static int32_t last_peak;
static void isr(void);
#define SOFT_SPIN() do { fm1_ms += 6; isr(); audio_halves++; } while (0)
#include "soft.h"

static void isr(void)                                   /* what fm1_alnk0_irq does with SOFT, on a full-scale input */
{
    uint32_t i;
    int32_t pk = 0;
    for (i = 0; i < 2 * N; i++)
        blk[i] = 20000;
    soft_apply(blk, N);
    for (i = 0; i < 2 * N; i++)
        if (blk[i] > pk)
            pk = blk[i];
    last_peak = pk;
}
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    uint32_t t, g, prev = 0;
    int32_t pk_prev = -1, first = -1;
    int mono = 1;

    /* unity: untouched, and free */
    memset(blk, 0, sizeof blk);
    isr();
    CHECK(last_peak == 20000);

    /* the swell: from silence, never down, squared (gentle start), unity after SOFT_IN_MS */
    soft_fade_in();
    for (t = 0; t < 1000; t += 6) {
        fm1_ms += 6;
        isr();
        if (first < 0)
            first = last_peak;
        if (last_peak < pk_prev)
            mono = 0;
        pk_prev = last_peak;
        if (t == 396)
            CHECK(last_peak > 3500 && last_peak < 7000);          /* half way in time: ~1/4 of the level (squared) */
        if (t > 840)
            CHECK(last_peak == 20000);
        g = soft_g;
        CHECK(g >= prev);
        prev = g;
    }
    CHECK(mono);
    CHECK(first < 2000);                                            /* the first block is nearly silent */

    /* the dip: out and played to silence, then back */
    audio_live = 1;
    audio_halves = 0;
    {
        uint32_t t0 = fm1_ms;
        soft_down_wait();
        CHECK(soft_g == 0);                                         /* silence reached ... */
        CHECK(audio_halves >= 3u);                                  /* ... and 3 more halves rendered at silence */
        CHECK(fm1_ms - t0 < 60u);                                   /* ... within the bound */
        isr();
        CHECK(last_peak == 0);
        soft_up();
        for (t = 0; t < 60; t += 6) { fm1_ms += 6; isr(); }
        CHECK(last_peak == 20000 && soft_g == SOFT_ONE);            /* back at unity within ~30 ms */
    }

    printf(fails ? "soft_test: %d FAILED\n" : "soft_test: ok\n", fails);
    return fails != 0;
}
