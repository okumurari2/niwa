/* SPDX-License-Identifier: GPL-3.0-only */
/* The fake FM-1 that both computer builds share: the clock, the keys, the knobs, the screen (240 x 240 RGB565), the settings
 * and the MIDI queue, as platform.c's NIWA_HOST branch needs them. tests/host.c adds a script and the PNG / WAV files;
 * web/emu/web_host.c adds the browser's exports (docs/EMULATOR.md). The includer brings the system headers (or none) and may
 * define SIM_LOG(fmt, ...) (default: printf) and SIM_STORE (the bytes behind settings_load / settings_save). */
#ifndef SIM_LOG
#define SIM_LOG(...) printf(__VA_ARGS__)
#endif
#ifndef APP_NAME
#define APP_NAME "NIWA"
#endif
#ifndef APP_VER
#define APP_VER "0.0"
#endif
#define FS 44100u
#define HALF_FRAMES 256u

static volatile uint32_t fm1_ms;
static struct { uint32_t notes, buttons; } fm1_in;
static uint32_t host_edges, host_released;      /* buttons pressed / let go since the last take (bit = button id) */
static uint32_t fm1_input_edges(uint32_t *released) { uint32_t b = host_edges; host_edges = 0; *released = host_released; host_released = 0; return b; }
static uint32_t fm1_input_note_edges(void) { return 0; }   /* (the script sets fm1_in.notes) */
static int32_t host_enc[8];                     /* detents turned since the last take (the script's "enc" lines) */
static int32_t fm1_enc_take(uint32_t e) { int32_t d = host_enc[e & 7u]; host_enc[e & 7u] = 0; return d; }
static uint32_t sim_led_btn, sim_led_key;       /* the lit LEDs: bit = button id (0..13), bit = key (LED id 14 + key) */
static void fm1_led_key(uint32_t id, int on)
{
    uint32_t *m = id < 14u ? &sim_led_btn : &sim_led_key, b = 1u << (id < 14u ? id : id - 14u);
    if (id >= 14u + 27u) return;
    if (on) *m |= b; else *m &= ~b;
}

/* the screen: 240 x 240 RGB565, as the ST7789 gets it (gfx.c sends the bytes swapped) */
static uint16_t fb[240 * 240];
static uint32_t sim_draws;                      /* lcd_fill / lcd_blit calls: the screen changed when this did */
static void lcd_sync(void) {}
static uint32_t host_full;                      /* big fills after start-up: a knob must not blank the screen (docs/LESSONS.md 1) */
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    sim_draws++;
    if (w * h >= 240u * 120u && fm1_ms > 100u) host_full++;
    for (j = y; j < y + h && j < 240u; j++)
        for (i = x; i < x + w && i < 240u; i++)
            fb[j * 240u + i] = c;
}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, j;
    sim_draws++;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i < 240u && y + j < 240u) {
                uint16_t c = px[j * w + i];
                fb[(y + j) * 240u + x + i] = (uint16_t)(c >> 8 | c << 8);
            }
}
#include "gfx.c"

#ifndef SIM_STORE
static uint8_t sim_store_mem[256 + 4];
#define SIM_STORE sim_store_mem
#endif
static uint32_t host_saves;                     /* settings_save calls */
static int settings_load(void *p, uint32_t n)
{
    if (SIM_STORE[0] != 1 || n > 256u) return -1;
    memcpy(p, SIM_STORE + 4, n);
    return 0;
}
static int settings_save(const void *p, uint32_t n)
{
    SIM_STORE[0] = 1;
    memcpy(SIM_STORE + 4, p, n);
    host_saves++;
    SIM_LOG("%6u ms  settings saved (%u B)\n", fm1_ms, n);
    return 0;
}

static uint8_t mq[64][3];
static uint32_t mq_w, mq_r;
static int midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2)
{
    if (mq_r == mq_w) return 0;
    *st = mq[mq_r % 64u][0]; *d1 = mq[mq_r % 64u][1]; *d2 = mq[mq_r % 64u][2];
    mq_r++;
    return 1;
}
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2) { (void)st; (void)d1; (void)d2; SIM_LOG("%6u ms  MIDI out %02X %u %u\n", fm1_ms, st, d1, d2); }

static int32_t fm1_battery(void) { return 0; }  /* (unknown: no notice) */
static int fm1_usb_powered(void) { return 0; }
static void app_audio(int32_t *out, uint32_t n);
static void app_main(void);
