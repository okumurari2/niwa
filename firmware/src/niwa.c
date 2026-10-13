/* SPDX-License-Identifier: GPL-3.0-only */
/* niwa: the base the FM-1 firmwares of this family stand on. As an app it is a sine wave on the keys, and that is all.
 * This is the file a firmware built on niwa replaces. Everything below it (platform.c) is done: updates, the USB rescue,
 * USB-MIDI, USB audio, keys, knobs, LEDs, the screen, a settings sector.
 *
 *   keys       play sines, 4 voices; the notes go out over USB-MIDI too
 *   OCT- OCT+  the octave (kept in flash)
 *   MASTER     the volume (platform.c)
 *   KNOB 1..4  four track volumes and the focus (tracks.h): the knob turned last is the focused track. Shown only: no
 *              sound belongs to them here, an app gives its tracks theirs
 *   USB-MIDI   notes in play the sines, with their velocity
 *
 * Rules of the road (the chip has no FPU and no libgcc): integers only, no float, no 64-bit division.
 * tests/host.c runs this file on a computer: the screen as a PNG, the sound as a WAV.
 * What platform.c offers: docs/API.md. The chip's rules: docs/RULES.md. */
#define APP_STEPPED                             /* app_init / app_step instead of app_main: the browser build runs the loop from outside (docs/EMULATOR.md) */
#include "platform.c"

/* ---- the sound (runs in the audio ISR: keep it short, no flash, no screen) */
#define NV 4u                                   /* voices */
static volatile int32_t rel_dec = 6;            /* how fast a released voice dies (per frame, of 11430): the RELEASE knob */
static struct {
    volatile uint32_t inc;                      /* phase step per frame: f * 2^32 / 44100 */
    volatile int32_t gate;                      /* target level, Q15: 0 = released */
    int32_t amp;                                /* the level now: glides to gate, no clicks */
    uint32_t ph, age;
    uint8_t note;
} voice[NV];

static int32_t sine(uint32_t ph)                /* a full cycle over 2^32 -> -32767..32767 (a refined parabola, ~0.1 %) */
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;               /* 4x(1-|x|), Q15 */
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15; /* + 0.225 (y|y| - y) */
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t v, i;
    for (v = 0; v < NV; v++) {
        uint32_t ph = voice[v].ph, step = voice[v].inc;
        int32_t amp = voice[v].amp, g = voice[v].gate;
        if (!amp && !g)
            continue;                           /* silent: costs nothing */
        for (i = 0; i < n; i++) {
            int32_t s;
            if (amp < g) { amp += 48; if (amp > g) amp = g; }        /* attack ~5 ms */
            else if (amp > g) { amp -= rel_dec; if (amp < g) amp = g; }  /* release */
            ph += step;
            s = (sine(ph) * amp) >> 15;
            out[2u * i] += s;                   /* L */
            out[2u * i + 1u] += s;              /* R */
        }
        voice[v].ph = ph;
        voice[v].amp = amp;
    }
}

/* ---- notes (main loop) */
static const uint32_t INC_C9[12] = {            /* MIDI 120..131 (C9..B9); lower octaves shift right */
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};
static uint32_t ages, last_note = 255, level = 127;   /* level: the LEVEL knob, 0..127 */

static void play(uint32_t note, uint32_t vel)
{
    uint32_t v, pick = 0, best = 0xFFFFFFFFu;
    if (note > 127u || !vel) return;
    for (v = 0; v < NV; v++) {                  /* the same note, else a free voice, else the oldest */
        uint32_t score = voice[v].note == note && voice[v].gate ? 0u
                       : voice[v].gate ? 0x80000000u + voice[v].age : (uint32_t)voice[v].amp + 1u;
        if (score < best) { best = score; pick = v; }
    }
    voice[pick].note = (uint8_t)note;
    voice[pick].age = ++ages;
    voice[pick].inc = INC_C9[note % 12u] >> (10u - note / 12u);
    voice[pick].gate = (int32_t)(vel * 90u * level / 127u);   /* 127 -> 11430: four voices stay under full scale */
    last_note = note;
}
static void stop(uint32_t note)
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (voice[v].note == note)
            voice[v].gate = 0;
}
static int quiet(void)                          /* nothing sounds, not even a release */
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (voice[v].gate || voice[v].amp) return 0;
    return 1;
}

#define KEY0 53u                                /* key 0 = F3 */

/* ---- the screen: docs/UI.md. Top left is "now" (the note, the octave); SELECT, PRESETS and ALGORITHM are the three knobs
 * at the left of the panel, each showing what it moves (quad.h). A knob moves a value 0..127, right = more. */
#include "quad.h"
#include "tracks.h"
static const uint16_t TRK_COL[TRK_N] = {0xFB86, 0xFE60, 0x4E8C, 0x5C7F};   /* the tracks' colours (UI.md 4): fixed per track, whatever the palette */
static const char *msg;                         /* a message in the place of the name, for ~2 s (UI.md 6): 0 = none */
static uint32_t msg_until;
static void draw_now(uint32_t note, int32_t oct, const uint8_t *vol, uint32_t focus)
{
    static const char *const NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    char s[8] = "-", o[10] = "OCTAVE +0";
    cv_begin(QUAD, QUAD, T_BG);
    if (msg)
        cv_text(6, 6, &AF_S, msg, T_ACCENT);
    else
        cv_text(6, 6, &AF_S, APP_NAME, T_DIM);
    if (note < 128u) {
        const char *p = NAME[note % 12u];
        uint32_t i = 0;
        while (*p) s[i++] = *p++;
        if (note >= 12u) s[i++] = (char)('0' + note / 12u - 1u);
        s[i] = 0;
    }
    cv_text_in(0, 24, QUAD, &AF_L, s, T_TEXT, T_BG);
    o[7] = oct < 0 ? '-' : '+';
    o[8] = (char)('0' + (oct < 0 ? -oct : oct));
    cv_text_in(0, 58, QUAD, &AF_S, o, T_MID, T_BG);
    trk_strip(8, 76, QUAD - 16, 36, vol, TRK_COL, focus);
    cv_blit(0, 0);
}

static int32_t knob(int32_t v, uint32_t enc)    /* a knob moves a value 0..127: 2 per detent */
{
    v += fm1_enc_take(enc) * 2;
    return v < 0 ? 0 : v > 127 ? 127 : v;
}

/* what survives a power-off (add yours, up to 256 bytes) */
static struct { int8_t oct; uint8_t level, release; uint8_t vol[TRK_N]; } set = {0, 127, 113, {100, 100, 100, 100}};
static uint32_t focus, prev, t_change, dirty, shown_now = ~0u, shown_quads = ~0u;
static uint8_t held[27];                        /* the note each key plays */

static void app_init(void)
{
    uint32_t k;
    settings_load(&set, sizeof set);
    if (set.oct < -2 || set.oct > 2) set.oct = 0;
    if (set.level > 127u || set.release > 127u) { set.level = 127; set.release = 113; }
    for (k = 0; k < TRK_N; k++)
        if (set.vol[k] > 127u) set.vol[k] = 100;

    palette_set(UI_DEFAULT_PALETTE);
    lcd_fill(0, 0, 240, 240, T_BG);
    quad_dividers();
}

static void app_step(void)                      /* one pass of the main loop */
{
    uint32_t keys, down, up, rel, b, k, lit = 0, base = (uint32_t)(KEY0 + 12 * set.oct);
    uint8_t st, d1, d2;

    keys = fm1_in.notes;                    /* bit k: key k (0 = F3 .. 26 = G5) */
    down = keys & ~prev;
    up = prev & ~keys;
    prev = keys;
    for (k = 0; k < 27u; k++) {                 /* (a key stops the note it started: the octave may change meanwhile) */
        if (down >> k & 1u) { held[k] = (uint8_t)(base + k); play(held[k], 100); midi_send(0x90, held[k], 100); }
        if (up >> k & 1u) { stop(held[k]); midi_send(0x80, held[k], 0); }
    }

    b = fm1_input_edges(&rel);              /* buttons pressed since the last call (bit = button id) */
    if (b & (1u << BTN_OCT_DOWN) && set.oct > -2) { set.oct--; dirty = 1; t_change = fm1_ms; }
    if (b & (1u << BTN_OCT_UP) && set.oct < 2) { set.oct++; dirty = 1; t_change = fm1_ms; }
    k = (uint32_t)knob(set.level, ENC_SELECT);
    if (k != set.level) { set.level = (uint8_t)k; level = k; dirty = 1; t_change = fm1_ms; }
    k = (uint32_t)knob(set.release, ENC_PRESETS);
    if (k != set.release) { set.release = (uint8_t)k; dirty = 1; t_change = fm1_ms; }
    { int32_t d[TRK_N]; for (k = 0; k < TRK_N; k++) d[k] = fm1_enc_take(ENC_KNOB1 + k);
      if (trk_turn(set.vol, d, &focus)) { dirty = 1; t_change = fm1_ms; } }
    rel_dec = 48 - (int32_t)set.release * 47 / 127;           /* 0 -> 48 (a click), 127 -> 1 (a long tail) */

    while (midi_get(&st, &d1, &d2)) {       /* USB-MIDI in, any channel */
        if ((st & 0xF0u) == 0x90u && d2) play(d1, d2);
        else if ((st & 0xF0u) == 0x80u || (st & 0xF0u) == 0x90u) stop(d1);
    }

    base = (uint32_t)(KEY0 + 12 * set.oct);
    for (k = 0; k < NV; k++)                /* what sounds, on the keys of this octave */
        if (voice[k].gate && voice[k].note >= base && voice[k].note < base + 27u)
            lit |= 1u << (voice[k].note - base);
    for (k = 0; k < 27u; k++)
        fm1_led_key(KEY_LED(k), lit >> k & 1u);
    fm1_led_key(BTN_OCT_DOWN, set.oct < 0);
    fm1_led_key(BTN_OCT_UP, set.oct > 0);

    if (msg && (int32_t)(fm1_ms - msg_until) >= 0) msg = 0;
    k = battery_notice(fm1_ms, fm1_battery(), fm1_usb_powered(), msg != 0);           /* MANNER:battery: only when it is low */
    if (k) { msg = k == BATT_LOW ? "BATTERY LOW" : "BATTERY EMPTY"; msg_until = fm1_ms + 2000u; }
    k = quad_mix(quad_mix(quad_mix(last_note, (uint32_t)set.oct + 8u), msg != 0), msg ? (uint32_t)msg[8] : 0u);
    k = quad_mix(quad_mix(quad_mix(quad_mix(k, set.vol[0]), set.vol[1]), quad_mix(set.vol[2], set.vol[3])), focus);   /* what the screen shows, as a number: redraw on change */
    if (k != shown_now) { shown_now = k; draw_now(last_note, set.oct, set.vol, focus); }
    k = quad_mix(quad_mix(0u, set.level), set.release);
    if (k != shown_quads) {
        quad_param_t sel = {"LEVEL", set.level, 0, T_THEME, 0}, pre = {"RELEASE", set.release, 0, T_THEME, 0};
        shown_quads = k;
        quad_draw(Q_SELECT, &sel);
        quad_draw(Q_PRESETS, &pre);              /* ALGORITHM: free for your third knob */
    }

    if (dirty && quiet() && fm1_ms - t_change > 2000u) {   /* 2 s after the change, when silent (an erase stalls the sound) */
        settings_save(&set, sizeof set);
        dirty = 0;
    }
}
