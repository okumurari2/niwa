/* SPDX-License-Identifier: GPL-3.0-only */
/* TAI's body: four organs, coupled, that keep moving with no one at the controls. Pure C (core/q15.h only): the FM-1, the
 * browser and tests/tai/sim.c (the CSV) run this same file. One step = ORG_DT_MS of body time; call it from the main loop.
 *
 *   organ          state             KNOB (primary)                 spills into (secondary)
 *   1 heart        hr, phase, co     K1 PRESSURE: rate, attack      co -> lung: ventilation speeds up
 *   2 lung         vent, co2, alk    K2 VENT: grain, air            alk (CO2 lost) -> muscle: Q over-excited
 *   3 muscle       q, osc, heat      K3 TENSION: resonance purity   heat -> fluid: leaks, the space swells
 *   4 fluid        vol, brake        K4 STASIS: depth, murk         brake (vagal) -> heart: slows, stops
 *
 * The loop closes: heart -> lung -> muscle -> fluid -> heart, with one sign flip (the brake), so the body hunts for a balance and
 * overshoots it: arrest and recovery, self-oscillation and cooling, on their own time. Every value is Q15 (0..32767 = 0..1)
 * unless named otherwise. The numbers to tune by ear are in ORG_* below and nowhere else (docs/PHILOSOPHY.md 5).
 *
 * The safety valve: an arrest (no beat) never lasts longer than ORG_ESCAPE_MS. Then an escape rhythm: the brake is lifted and held
 * off for ORG_REFRACT_MS, half the fluid drains. tests/tai/sim.c holds this under every knob setting. */
#ifndef ORGANS_H
#define ORGANS_H
#include "core/q15.h"

#define ORG_DT_MS 5u                        /* one step: 200 steps a second */
#define ORG_HR_MIN 40                       /* bpm at K1 = 0 */
#define ORG_HR_SPAN 140                     /* + this at K1 = 1 */
#define ORG_HR_ARREST 22                    /* below this many bpm, no beat at all: the flat line */
#define ORG_ESCAPE_MS 6000u                 /* the longest an arrest lasts */
#define ORG_REFRACT_MS 3000u                /* after an escape, the brake stays off this long */
#define ORG_Q_OSC Q15(0.62)                 /* the muscle's Q above which it rings by itself */
#define ORG_BRAKE_LO Q15(0.62)              /* the fluid level where the vagal brake starts ... */
#define ORG_BRAKE_HI Q15(0.92)              /* ... and where it is full (the heart stops) */
#define ORG_DRIFT_MS 12000u                 /* the body's own slow tide: each organ picks a new lean about this often */
#define ORG_DRIFT Q15(0.18)                 /* how far it leans (+-) */

typedef struct {
    /* in: set these any time (Q15) */
    int32_t knob[4];                        /* K1 PRESSURE, K2 VENT, K3 TENSION, K4 STASIS */
    int32_t compliance;                     /* stiff (0: fast, hard) .. soft (1: slow, smeared) */
    int32_t metabolism;                     /* slow (0: things linger) .. fast (1: everything clears) */
    int32_t electrolyte;                    /* (kept for the resonator's scale pull: not used by the body yet) */

    /* 1 heart */
    int32_t hr;                             /* beats per minute, Q8 (72 bpm = 72 << 8) */
    uint32_t phase;                         /* 0 .. 2^32 per beat; frozen in an arrest */
    int32_t co;                             /* cardiac output */
    int32_t attack;                         /* how hard each beat hits (K1, less when the heart is starved) */
    uint32_t beats;                         /* counts up on each beat (the DSP's pulse trigger: compare with your last) */
    uint32_t arrest_ms;                     /* how long the heart has been stopped; 0 = beating */
    uint32_t refract_ms;                    /* > 0: just escaped, the brake is held off */
    uint32_t escapes;                       /* how many times the safety valve fired */

    /* 2 lung */
    int32_t vent;                           /* ventilation: the grain scan speed */
    uint32_t rphase;                        /* 0 .. 2^32 per breath */
    int32_t co2;                            /* blood CO2; 0.5 is the set point */
    int32_t alk;                            /* alkalosis: how far CO2 has fallen below the set point */

    /* 3 muscle */
    int32_t q;                              /* the resonator's sharpness */
    int32_t osc;                            /* self-oscillation: grows while q > ORG_Q_OSC */
    int32_t heat;

    /* 4 fluid */
    int32_t vol;                            /* the fluid in the space: the reverb's fill and decay */
    int32_t brake;                          /* the vagal brake on the heart */

    /* hands (the keys, later the ampoules) */
    uint8_t occlude;                        /* held: the couplings are cut */
    int32_t reperf;                         /* after occlusion: the rebound, decays */
    int32_t cold;                           /* a cold shock, decays */

    int32_t drift[4], lean[4];              /* the slow tide per organ (-ORG_DRIFT .. ORG_DRIFT): drift glides to lean */
    uint32_t rng;
    uint32_t ms;                            /* body time */
} org_body;

static void org_init(org_body *b, uint32_t seed)
{
    int i;
    for (i = 0; i < 4; i++) b->knob[i] = Q15(0.5);
    b->compliance = b->metabolism = b->electrolyte = Q15(0.5);
    b->hr = 72 << 8;
    b->phase = b->rphase = 0;
    b->co = Q15(0.4);
    b->attack = Q15(0.5);
    b->beats = b->arrest_ms = b->refract_ms = b->escapes = 0;
    b->vent = Q15(0.5);
    b->co2 = Q15(0.5);
    b->alk = 0;
    b->q = Q15(0.4);
    b->osc = b->heat = 0;
    b->vol = Q15(0.2);
    b->brake = 0;
    b->occlude = 0;
    b->reperf = b->cold = 0;
    b->rng = seed ? seed : 0x7A1u;
    for (i = 0; i < 4; i++) b->drift[i] = b->lean[i] = 0;
    b->ms = 0;
}

static void org_step(org_body *b)
{
    const int32_t k1 = b->knob[0], k2 = b->knob[1], k3 = b->knob[2], k4 = b->knob[3];
    int32_t met = Q15(0.3) + q15_mul(b->metabolism, Q15(0.7));          /* 0.3 .. 1: scales every clearing rate */
    int32_t soft = b->compliance;
    int32_t pass = b->occlude ? Q15(0.1) : Q1 - 1;                      /* what gets through a coupling while occluded */
    int32_t n1 = q15_noise(&b->rng), n2 = q15_noise(&b->rng);
    int32_t t, hr_t;
    uint32_t old;

    b->ms += ORG_DT_MS;
    {                                                                          /* the tide: no two minutes alike */
        int i = (int)(b->ms / ORG_DT_MS & 3u);
        if ((uint32_t)(q15_noise(&b->rng) & 0xFFFF) < 65536u * ORG_DT_MS * 4u / ORG_DRIFT_MS)
            b->lean[i] = q15_mul(q15_noise(&b->rng), ORG_DRIFT);
        b->drift[i] = q15_leak(b->drift[i], b->lean[i], Q15(0.004));            /* (each organ every 4th step: ~5 s glide) */
    }

    /* ---- 4 fluid -> the brake (first: the heart reads it this step) */
    t = b->refract_ms ? 0 : q15_smooth(b->vol, ORG_BRAKE_LO, ORG_BRAKE_HI);
    b->brake = q15_leak(b->brake, q15_mul(t, pass), Q15(0.01));

    /* ---- 1 heart: K1 sets the rate, the brake pulls it down; a little wander (no two minutes alike) */
    hr_t = (ORG_HR_MIN << 8) + ((ORG_HR_SPAN * q15_clamp(k1 + b->drift[0])) >> 7) + (n1 >> 9);   /* bpm Q8; noise +-0.25 bpm */
    hr_t += (int32_t)(b->rphase >> 16) < 32768 ? 3 << 8 : -(3 << 8);           /* the breath tugs at the rate (+-3 bpm) */
    hr_t = (hr_t * (Q1 - b->brake)) >> 15;                                    /* (< 2^31: hr_t < 180 << 8) */
    hr_t += q15_mul(b->reperf, 30 << 8);                                       /* reperfusion: a rush */
    b->hr = q15_leak(b->hr, hr_t, Q15(0.02) - q15_mul(soft, Q15(0.016)));     /* stiff follows in ~0.25 s, soft in ~1.2 s */
    if (b->hr < (ORG_HR_ARREST << 8) && !b->refract_ms) {
        b->arrest_ms += ORG_DT_MS;
        if (b->arrest_ms >= ORG_ESCAPE_MS) {                                   /* the safety valve: an escape rhythm */
            b->arrest_ms = 0;
            b->refract_ms = ORG_REFRACT_MS;
            b->brake = 0;
            b->vol >>= 1;
            b->hr = ORG_HR_MIN << 8;
            b->phase = 0;
            b->beats++;
            b->escapes++;
        }
    } else {
        b->arrest_ms = 0;
        old = b->phase;
        b->phase += (uint32_t)b->hr * 1398u;                                   /* bpm/60 * 2^32 / 200, bpm in Q8 */
        if (b->phase < old) b->beats++;
    }
    if (b->refract_ms) b->refract_ms -= ORG_DT_MS;
    /* output: rate x stroke; the stroke shrinks as the fluid backs up */
    t = b->arrest_ms ? 0 : q15_mul(q15_clamp((b->hr * 182) >> 8), Q1 - q15_mul(b->vol, Q15(0.6)));   /* hr/180 */
    b->co = q15_leak(b->co, t, Q15(0.01));
    b->attack = q15_clamp(q15_mul(Q15(0.15) + q15_mul(k1, Q15(0.85)), Q15(0.4) + q15_mul(b->co, Q15(0.6)) + q15_mul(b->reperf, Q15(0.3))));

    /* ---- 2 lung: K2 + the heart's output (the tissue gasps) */
    t = Q15(0.1) + q15_mul(k2, Q15(0.55)) + b->drift[1] + q15_mul(q15_mul(b->co, Q15(0.7)), pass) + (n2 >> 7) + q15_mul(b->reperf, Q15(0.3));
    t -= q15_mul(b->cold, Q15(0.6));
    b->vent = q15_leak(b->vent, q15_clamp(t), Q15(0.008));
    b->rphase += (uint32_t)(Q15(0.2) + b->vent) * 327u;                        /* 6 .. 36 breaths a minute */
    /* CO2: made by the working body, blown off by ventilation */
    t = Q15(0.08) + q15_mul(b->co, Q15(0.5)) + q15_mul(b->heat, Q15(0.1));      /* production */
    t -= q15_mul(q15_mul(b->vent, b->co2), Q15(0.9));                          /* elimination */
    b->co2 = q15_clamp(b->co2 + (t >> 6));
    b->alk = q15_clamp((Q15(0.5) - b->co2) * 3);

    /* ---- 3 muscle: K3 sets the tension; alkalosis makes the membrane jumpy */
    t = q15_mul(k3, Q15(0.7)) + b->drift[2] + q15_mul(q15_mul(b->alk, Q15(0.6)), pass) + q15_mul(b->reperf, Q15(0.2)) - b->cold;
    b->q = q15_leak(b->q, q15_clamp(t), Q15(0.01));
    if (b->q > ORG_Q_OSC)                                                      /* rings by itself, up to a limit */
        b->osc += q15_mul(q15_mul(b->q - ORG_Q_OSC, Q15(0.08)), Q1 - b->osc) + 1;
    else
        b->osc -= q15_mul(b->osc, q15_mul(ORG_Q_OSC - b->q, Q15(0.03))) + 1;
    b->osc = q15_clamp(b->osc - q15_mul(b->osc, b->cold >> 4));
    t = q15_mul(b->osc, Q15(0.012)) + q15_mul(q15_mul(k3, k3), Q15(0.0008));   /* heat in */
    t -= q15_mul(b->heat, q15_mul(met, Q15(0.004)) + (b->cold >> 6));          /* heat out */
    b->heat = q15_clamp(b->heat + t);

    /* ---- 4 fluid: K4 pours in, heat leaks in; the kidney drains, by the heart's output and the metabolism */
    t = q15_mul(q15_clamp(k4 + b->drift[3]), Q15(0.0016)) + q15_mul(q15_mul(b->heat, Q15(0.003)), pass);
    t = q15_mul(t, Q1 - b->vol);
    t -= q15_mul(b->vol, q15_mul(met, Q15(0.001) + q15_mul(b->co, Q15(0.0016))));
    b->vol = q15_clamp(b->vol + t);

    b->reperf = q15_leak(b->reperf, 0, Q15(0.004));
    b->cold = q15_leak(b->cold, 0, Q15(0.006));
}

/* ---- the hands (the 25 keys are three bands: ORG_PACE low, ORG_OCCLUDE mid, ORG_COOL high) */
static void org_pace(org_body *b)                   /* a forced beat; each one loosens the brake */
{
    b->phase = 0;
    b->beats++;
    b->brake = q15_mul(b->brake, Q15(0.6));
    b->vol -= b->vol >> 5;
    if (b->hr < (ORG_HR_ARREST + 8) << 8) b->hr = (ORG_HR_ARREST + 8) << 8;
}
static void org_occlude(org_body *b, int on)        /* held: the couplings are cut; let go: the reperfusion rebound */
{
    if (b->occlude && !on) b->reperf = Q15(0.9);
    b->occlude = (uint8_t)(on != 0);
}
static void org_cool(org_body *b)                   /* cold water: the ringing and the breathing drop at once */
{
    b->cold = Q15(0.8);
}
#endif
