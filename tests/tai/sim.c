/* SPDX-License-Identifier: GPL-3.0-only */
/* TAI's body (firmware/src/tai/organs.h) on a computer, with no sound: scripted hands on the knobs, the state as CSV, and the
 * checks that must hold whatever the hands do.
 *   cc -O2 -Wall -Ifirmware/src -o build/tai/sim tests/tai/sim.c && build/tai/sim build/tai
 *   -> build/tai/<scenario>.csv, 10 rows a second: t, the 4 knobs, then the body (Q15 as 0..1, hr in bpm)
 * Checks: an arrest ends within ORG_ESCAPE_MS (the safety valve), every value stays in range, the body never settles into a
 * fixed point (it keeps moving with the knobs still). Prints a line of numbers per scenario to hear-check against. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tai/organs.h"

static int fails;
static void check(const char *what, int ok) { printf("tai: %-72s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; }
static double f(int32_t q) { return q / 32768.0; }

typedef struct { const char *name; uint32_t seconds; double k[4]; int wander; int hands; } scenario;

static const scenario S[] = {
    {"rest",      600, {0.5, 0.5, 0.5, 0.5}, 0, 0},      /* hands off: it must still move */
    {"pressure",  300, {1.0, 0.5, 0.5, 0.5}, 0, 0},      /* K1 up: the chain to the lung, the muscle, the fluid */
    {"ventilate", 300, {0.5, 1.0, 0.5, 0.3}, 0, 0},      /* K2 up: alkalosis, the muscle rings */
    {"tension",   300, {0.5, 0.5, 1.0, 0.3}, 0, 0},      /* K3 up: heat, the fluid swells */
    {"stasis",    300, {0.5, 0.5, 0.5, 1.0}, 0, 0},      /* K4 up: the brake, arrests, escapes */
    {"all-max",   300, {1.0, 1.0, 1.0, 1.0}, 0, 0},
    {"all-min",   300, {0.0, 0.0, 0.0, 0.0}, 0, 0},
    {"wander",   1800, {0.5, 0.5, 0.5, 0.5}, 1, 1},      /* 30 min of knobs and keys at random */
};

static void run(const scenario *s, const char *dir)
{
    char path[512], line[160];
    FILE *csv;
    org_body b;
    uint32_t steps = s->seconds * 1000u / ORG_DT_MS, i, arrest_max = 0, arrests = 0, osc_steps = 0, rng = 12345u, hold = 0;
    int32_t hr_lo = 1 << 30, hr_hi = 0, vol_lo = Q1, vol_hi = 0, in_range = 1, k;
    int was_arrest = 0;

    org_init(&b, 0xC0FFEEu);
    for (k = 0; k < 4; k++) b.knob[k] = q15_clamp((int32_t)(s->k[k] * 32768.0));
    snprintf(path, sizeof path, "%s/%s.csv", dir, s->name);
    csv = fopen(path, "w");
    if (!csv) { perror(path); exit(1); }
    fprintf(csv, "t,k1,k2,k3,k4,hr,co,attack,vent,co2,alk,q,osc,heat,vol,brake,arrest,beats,escapes,occlude\n");
    for (i = 0; i < steps; i++) {
        if (s->wander && i % 200u == 0) {                     /* each second: maybe nudge a knob */
            k = (int32_t)(q15_noise(&rng) & 3);
            if ((q15_noise(&rng) & 7) < 3) b.knob[k] = q15_clamp(b.knob[k] + (q15_noise(&rng) >> 2));
        }
        if (s->hands && i % 200u == 0) {                      /* and maybe a key */
            int32_t r = q15_noise(&rng) & 63;
            if (r == 0) org_pace(&b);
            if (r == 1) { org_occlude(&b, 1); hold = 400; }
            if (r == 2) org_cool(&b);
        }
        if (hold && !--hold) org_occlude(&b, 0);
        org_step(&b);

        if (b.arrest_ms > arrest_max) arrest_max = b.arrest_ms;
        if (b.arrest_ms && !was_arrest) arrests++;
        was_arrest = b.arrest_ms != 0;
        if (b.osc > Q15(0.5)) osc_steps++;
        if (i > 2000u) {                                      /* after the first 10 s */
            if (b.hr < hr_lo) hr_lo = b.hr;
            if (b.hr > hr_hi) hr_hi = b.hr;
            if (b.vol < vol_lo) vol_lo = b.vol;
            if (b.vol > vol_hi) vol_hi = b.vol;
        }
        {
            const int32_t v[] = {b.co, b.attack, b.vent, b.co2, b.alk, b.q, b.osc, b.heat, b.vol, b.brake};
            for (k = 0; k < 10; k++) if (v[k] < 0 || v[k] > 32767) in_range = 0;
            if (b.hr < 0 || b.hr > (ORG_HR_MIN + ORG_HR_SPAN + 40) << 8) in_range = 0;
        }
        if (i % 20u == 19u)
            fprintf(csv, "%.2f,%.3f,%.3f,%.3f,%.3f,%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u\n",
                    b.ms / 1000.0, f(b.knob[0]), f(b.knob[1]), f(b.knob[2]), f(b.knob[3]), b.hr / 256.0, f(b.co), f(b.attack),
                    f(b.vent), f(b.co2), f(b.alk), f(b.q), f(b.osc), f(b.heat), f(b.vol), f(b.brake), b.arrest_ms, b.beats,
                    b.escapes, b.occlude);
    }
    fclose(csv);
    printf("%-9s hr %3d..%3d bpm  vol %.2f..%.2f  arrests %3u (longest %4u ms, escapes %3u)  ringing %4.1f %%  -> %s\n", s->name,
           hr_lo >> 8, hr_hi >> 8, f(vol_lo), f(vol_hi), arrests, arrest_max, b.escapes, 100.0 * osc_steps / steps, path);
    snprintf(line, sizeof line, "%s: an arrest ends within %u ms (the safety valve), every value in range", s->name, ORG_ESCAPE_MS);
    check(line, arrest_max <= ORG_ESCAPE_MS && in_range);
    if (!s->wander) {
        snprintf(line, sizeof line, "%s: with the knobs still, it keeps moving (hr spread %d bpm or vol %.2f)", s->name,
                 (hr_hi - hr_lo) >> 8, f(vol_hi - vol_lo));
        check(line, hr_hi - hr_lo >= 2 << 8 || vol_hi - vol_lo >= Q15(0.05));
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "build/tai";
    uint32_t i;
    for (i = 0; i < sizeof S / sizeof S[0]; i++) run(&S[i], dir);
    printf(fails ? "tai: %d FAILED\n" : "tai: all ok\n", fails);
    return fails != 0;
}
