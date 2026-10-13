/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * The design (the device clock driven by the audio the worklet asks for, the exports below, the saved flash handed back
 * before boot) follows the browser builds of Felucca / MAKU / t8lab (web/emu/felucca_web.c), after X0X's web/emu/x0x_web.c of
 * charlesvestal/fm1-x0x (GPL-3.0). */
/* niwa in the browser: platform.c includes this file instead of the hardware (or tests/host.c) when NIWA_WEB is set, and the
 * app (firmware/src/niwa.c, or yours) runs on it unchanged, compiled to WebAssembly, in an AudioWorklet (web/emu/worklet.js).
 * It is the same fake FM-1 as tests/host.c (tests/sim.c: the clock, keys, knobs, screen, settings), with the page's side added:
 *   - the device clock advances with the audio the worklet asks for: web_render(n) runs the device 1 ms at a time, and each
 *     millisecond is one platform_poll() and one app_step(), as tests/host.c does;
 *   - the panel: buttons and encoders by their printed labels, the keys as fm1_in.notes, the LEDs read back;
 *   - the screen: a 240 x 240 RGB565 frame, byte-swapped as the ST7789 gets it;
 *   - the flash: a 1 MiB NOR image; the settings sector (0xFC000) is where settings_load / settings_save keep their bytes.
 * Not here: USB (MIDI out, audio, the updater), the battery (unknown: no notice), the 0.8 s power-on swell (soft.h).
 * The app needs APP_STEPPED (platform.c): the page runs its loop, one pass at a time. docs/EMULATOR.md.
 *   web/emu/build.sh  ->  build/emu/{index.html, worklet.js, app.js, <APP>.wasm, ...} */
#include <stdint.h>

#ifndef APP_STEPPED
#error "the browser build runs the app's loop from outside: define APP_STEPPED and write app_init() / app_step() (platform.c)"
#endif
#define SIM_LOG(...) ((void)0)
#define WEB_EXPORT(n) __attribute__((export_name(#n)))

#include "../../firmware/src/libc.c"             /* memcpy and friends (there is no C library here) */

static uint8_t nor[0x100000];                    /* the SPI NOR; erased: 0xFF */
#define SETTINGS_AT 0x000FC000u                  /* the settings sector, as platform.c */
#define SIM_STORE (nor + SETTINGS_AT)
#include "../../tests/sim.c"

static void app_init(void);
static void app_step(void);
static void platform_poll(void) {}               /* (the page's MASTER knob is web_master) */

static uint32_t web_booted, web_flash_writes;

/* ------------------------------------------------------------- audio --- */
/* the MASTER knob as platform.c scales it: the ADC's 0..1023 squared, Q12 */
static uint32_t web_master_q12 = (700u * 700u) >> 8;
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r, web_owed;

static void web_block(void)                      /* one I2S half: what the audio ISR would render */
{
    static int32_t blk[2u * HALF_FRAMES];
    uint32_t i;
    memset(blk, 0, sizeof blk);
    app_audio(blk, HALF_FRAMES);
    for (i = 0; i < 2u * HALF_FRAMES; i++) {
        int32_t v = (blk[i] * (int32_t)web_master_q12) >> 12;
        v = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
        ring[(ring_w * 2u + i) % (RING * 2u)] = (float)v / 32768.0f;
    }
    ring_w += HALF_FRAMES;
}

/* one millisecond of the device: the clock, the audio blocks due by now, the main loop */
static void web_ms(void)
{
    fm1_ms++;
    web_owed += FS;
    while (web_owed >= 1000u * HALF_FRAMES) {
        web_owed -= 1000u * HALF_FRAMES;
        web_block();
    }
    if (web_booted) {
        uint32_t saves = host_saves;
        platform_poll();
        app_step();
        if (saves != host_saves)
            web_flash_writes++;                  /* the page keeps the settings sector when this counts up */
    }
}

static float out_l[1024], out_r[1024];
WEB_EXPORT(web_out_l) float *web_out_l(void) { return out_l; }
WEB_EXPORT(web_out_r) float *web_out_r(void) { return out_r; }
/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
WEB_EXPORT(web_render) void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        web_ms();
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1u];
    }
}

/* ------------------------------------------------------------- flash --- */
/* the sectors the page keeps in the browser: {count, start, end, start, end ...} (worklet.js asks) */
static const uint32_t web_areas[] = {1u, SETTINGS_AT, 0x100000u};
WEB_EXPORT(web_store_areas) const uint32_t *web_store_areas(void) { return web_areas; }
WEB_EXPORT(web_nor) uint8_t *web_nor(void) { return nor; }
WEB_EXPORT(web_nor_size) uint32_t web_nor_size(void) { return sizeof nor; }
WEB_EXPORT(web_nor_erase) void web_nor_erase(void) { memset(nor, 0xFF, sizeof nor); }
WEB_EXPORT(web_flash_writes_count) uint32_t web_flash_writes_count(void) { return web_flash_writes; }

/* power on: after the page has put its saved sectors in the flash */
WEB_EXPORT(web_boot) void web_boot(void)
{
    if (web_booted)
        return;
    app_init();
    web_booted = 1;
}

/* ------------------------------------------------------------- panel --- */
/* The page's labels (web/emu/index.html B; the same on MAKU and t8lab) are not niwa's button ids (platform.c BTN_*):
 * FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+ = 0..13  ->  id (label + 2) % 14.
 * Its encoder roles are SELECT ALGORITHM PRESETS KNOB1..4 = 0..6  ->  ENC_*. */
static uint32_t web_id(uint32_t label) { return (label + 2u) % 14u; }
static const uint8_t WEB_ENC[7] = {ENC_SELECT, ENC_ALGORITHM, ENC_PRESETS, ENC_KNOB1, ENC_KNOB2, ENC_KNOB3, ENC_KNOB4};

WEB_EXPORT(web_buttons) void web_buttons(uint32_t labels)
{
    uint32_t m = 0, b, old = fm1_in.buttons;
    for (b = 0; b < 14u; b++)
        if (labels >> b & 1u)
            m |= 1u << web_id(b);
    host_edges |= m & ~old;
    host_released |= old & ~m;
    fm1_in.buttons = m;
}
WEB_EXPORT(web_keys) void web_keys(uint32_t mask) { fm1_in.notes = mask & ((1u << 27) - 1u); }
WEB_EXPORT(web_enc) void web_enc(uint32_t role, int32_t n)
{
    if (role < 7u)
        host_enc[WEB_ENC[role]] += n;
}
/* MASTER, 0..1023 as the ADC reads it */
WEB_EXPORT(web_master) void web_master(uint32_t v)
{
    v = v > 1023u ? 1023u : v;
    web_master_q12 = (v * v) >> 8;
}
/* a channel message from Web MIDI: 0 = the queue was full */
WEB_EXPORT(web_midi) uint32_t web_midi(uint32_t status, uint32_t d1, uint32_t d2)
{
    if (status < 0x80u || status >= 0xF0u)
        return 1;
    if (mq_w - mq_r >= 64u)
        return 0;
    mq[mq_w % 64u][0] = (uint8_t)status;
    mq[mq_w % 64u][1] = (uint8_t)(d1 & 127u);
    mq[mq_w % 64u][2] = (uint8_t)(d2 & 127u);
    mq_w++;
    return 1;
}

/* the LEDs: bit b = the button labelled b, bit 14 = PLAY's green (none here); keys: bit k = key k */
WEB_EXPORT(web_lit_buttons) uint32_t web_lit_buttons(void)
{
    uint32_t b, m = 0;
    for (b = 0; b < 14u; b++)
        m |= (sim_led_btn >> web_id(b) & 1u) << b;
    return m;
}
WEB_EXPORT(web_lit_keys) uint32_t web_lit_keys(void) { return sim_led_key; }

/* ------------------------------------------------------------ screen --- */
static uint16_t web_fb[240 * 240];               /* RGB565 with the bytes swapped, as the panel gets it */
WEB_EXPORT(web_screen) uint16_t *web_screen(void)
{
    uint32_t i;
    for (i = 0; i < 240u * 240u; i++)
        web_fb[i] = (uint16_t)(fb[i] >> 8 | fb[i] << 8);
    return web_fb;
}
WEB_EXPORT(web_screen_draws) uint32_t web_screen_draws(void) { return sim_draws; }
WEB_EXPORT(web_now_ms) uint32_t web_now_ms(void) { return fm1_ms; }
WEB_EXPORT(web_sample_rate) uint32_t web_sample_rate(void) { return FS; }
