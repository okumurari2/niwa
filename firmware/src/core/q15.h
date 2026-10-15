/* SPDX-License-Identifier: GPL-3.0-only */
/* Q15 state variables for the main loop: a value 0..32767 (= 0..1), a leaky integrator that glides it to a target, and a cheap
 * noise. Integers only (no FPU, no libgcc: docs/RULES.md). Pure C, no platform: a host test includes it alone.
 * Ranges: a and b of q15_mul up to +-32768 (the product stays inside 31 bits). */
#ifndef Q15_H
#define Q15_H
#include <stdint.h>

#define Q1 32768
#define Q15(x) ((int32_t)((x) * 32768.0 + 0.5))        /* a constant, folded by the compiler: Q15(0.25) = 8192 */

static inline int32_t q15_mul(int32_t a, int32_t b) { return (a * b) >> 15; }
static inline int32_t q15_clamp(int32_t x) { return x < 0 ? 0 : x > 32767 ? 32767 : x; }
static inline int32_t q15_pos(int32_t x) { return x < 0 ? 0 : x; }

/* the leaky integrator: x moves the fraction a (Q15) of the way to t each step. a = Q1 / n: ~n steps to settle (63 %). */
static inline int32_t q15_leak(int32_t x, int32_t t, int32_t a) { return x + q15_mul(t - x, a); }

/* 0 below lo, 1 above hi, a smooth ramp between (3t^2 - 2t^3): a threshold that bends instead of breaking. lo < hi. */
static inline int32_t q15_smooth(int32_t x, int32_t lo, int32_t hi)
{
    int32_t t;
    if (x <= lo) return 0;
    if (x >= hi) return 32767;
    t = (int32_t)(((x - lo) << 15) / (hi - lo));
    return q15_clamp(q15_mul(q15_mul(t, t), 3 * Q1 - 2 * t));
}

/* xorshift32: a full-period noise, -32768..32767. One state per stream; never seed with 0. */
static inline int32_t q15_noise(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return (int32_t)(x >> 16) - 32768;
}
#endif
