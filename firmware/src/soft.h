/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (maku audio.c: SOFT) */
/* MANNER:fade-in MANNER:erase-dip (docs/UI.md 9). The FM-1 is a battery unit with a small speaker.
 *
 * SOFT: one gain on what the DAC gets (not USB audio: its copy is taken before), moved a little each block, on a squared
 * curve (a fade-in starts gently). Unity costs one compare per block. Two users:
 *  - power-on (platform.c): soft_fade_in, the first 0.8 s swell from silence, so a MASTER knob left up does not shout out
 *    of the small speaker the moment the plug goes in;
 *  - a flash erase (settings_save): soft_down_wait fades out and waits until the DAC has played the silence, the erase
 *    then stops the audio for its ~40 ms with nothing cut, and soft_up brings it back: a short dip instead of a click at
 *    both edges.
 * Q24: SOFT_ONE is unity. soft_g is the audio ISR's, soft_tgt / soft_step the main loop's.
 *
 * Needs before it: FS (the sample rate), fm1_ms. audio_halves counts the halves the ISR has rendered (platform.c).
 * SOFT_SPIN() runs inside the waits (nothing on the device; the host test advances a simulated ISR there). */
#ifndef SOFT_H
#define SOFT_H
#include <stdint.h>

#define SOFT_ONE (1u << 24)
#define SOFT_IN_MS 800u           /* power-on swell */
#define SOFT_OUT_MS 10u           /* before an erase: out ... */
#define SOFT_UP_MS 25u            /* ... and back in */
#ifndef SOFT_SPIN
#define SOFT_SPIN() ((void)0)
#endif
static volatile uint32_t soft_g = SOFT_ONE, soft_tgt = SOFT_ONE, soft_step = SOFT_ONE;
static volatile uint32_t audio_halves;
static volatile uint8_t audio_live;                    /* audio_init ran: the ISR renders (a wait on it would never end) */

static void soft_set(uint32_t tgt, uint32_t ms)        /* ms: the whole 0..unity sweep */
{
    uint32_t n = (uint32_t)FS * ms / 1000u;
    soft_step = n ? SOFT_ONE / n + 1u : SOFT_ONE;
    soft_tgt = tgt;
}
static void soft_fade_in(void)                         /* before the audio starts */
{
    soft_g = 0;
    soft_set(SOFT_ONE, SOFT_IN_MS);
}
static void soft_down_wait(void)                       /* returns when the DAC plays silence (or after 60 ms) */
{
    uint32_t t = fm1_ms, h;
    soft_set(0, SOFT_OUT_MS);
    while (soft_g != 0 && (int32_t)(fm1_ms - t) < 40)
        SOFT_SPIN();
    h = audio_halves;                                  /* (the halves already rendered still play: 2 more) */
    while (audio_halves - h < 3u && (int32_t)(fm1_ms - t) < 60)
        SOFT_SPIN();
}
static void soft_up(void) { soft_set(SOFT_ONE, SOFT_UP_MS); }
static inline int32_t soft_q(uint32_t g) { uint32_t u = g >> 12; return (int32_t)((u * u) >> 12); }   /* 0..4096 */
/* the audio ISR, per block: n stereo frames in out (L R L R) */
static __attribute__((noinline)) void soft_apply(int32_t *out, uint32_t n)
{
    uint32_t g0 = soft_g, tgt = soft_tgt, g1, st = soft_step * n, i;
    int32_t q0, dq;
    if (g0 == tgt && g0 == SOFT_ONE)
        return;
    g1 = g0 < tgt ? (tgt - g0 < st ? tgt : g0 + st) : (g0 - tgt < st ? tgt : g0 - st);
    q0 = soft_q(g0);
    dq = ((soft_q(g1) - q0) * 256) / (int32_t)n;      /* Q8 per sample */
    for (i = 0; i < n; i++) {
        int32_t m = q0 + ((dq * (int32_t)i) >> 8);
        out[2u * i] = (out[2u * i] * m) >> 12;
        out[2u * i + 1u] = (out[2u * i + 1u] * m) >> 12;
    }
    soft_g = g1;
}
#endif
