/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (the thresholds: Felucca; the notice: maku ui_input.c) */
/* MANNER:battery (docs/UI.md 9). The battery is not on the screen while it is fine (the top left stays "now": no icon). On the
 * battery (not while a USB host powers the unit) at 1 bar of 3 or less the app says BATTERY LOW, at 0 BATTERY EMPTY, in the
 * message place, for about 2 s. Only once the level has held for 5 s (the small speaker's peaks sag the cell), again at once
 * when it drops a bar, and every 2 minutes while it lasts. It waits for another message to end, and for the first reading.
 *
 * Pure functions: platform.c reads the ADC (fm1_battery(): the filtered raw value, 0 until the first reading). */
#ifndef BATTERY_H
#define BATTERY_H
#include <stdint.h>

#define BATT_EMPTY_RAW 531                       /* ADC channel 3: below this 0 bars, then 1 at 531, 2 at 561, 3 at 591 */
enum { BATT_NONE = 0, BATT_LOW = 1, BATT_EMPTY = 2 };

static int32_t battery_level(int32_t raw)         /* 0..3 bars of 3 */
{
    return raw >= 591 ? 3 : raw >= 561 ? 2 : raw >= 531 ? 1 : 0;
}

/* a slow IIR of the readings b (> 0), avg 0 = none yet */
static int32_t battery_filter(int32_t avg, int32_t b)
{
    return b <= 0 ? avg : avg ? avg + (b - avg) / 32 : b;
}

/* now: milliseconds. raw: battery_filter's value. usb_power: a host is connected. busy: another message is showing.
 * Returns BATT_LOW / BATT_EMPTY when the app should show it now (for ~2 s), else BATT_NONE. Call it every frame */
static int battery_notice(uint32_t now, int32_t raw, int usb_power, int busy)
{
    static uint32_t since, next;                   /* when the level began to be low (| 1; 0: it is not), the next reminder */
    static int32_t said = 3;                       /* the level last announced (3: none yet) */
    int32_t lv = battery_level(raw);
    if (!raw || usb_power || lv > 1) {
        since = 0;
        said = 3;
        return BATT_NONE;
    }
    if (!since)
        since = now | 1u;
    if ((int32_t)(now - since) < 5000 || busy || (lv >= said && (int32_t)(now - next) < 0))
        return BATT_NONE;
    said = lv;
    next = now + 120000u;
    return lv ? BATT_LOW : BATT_EMPTY;
}
#endif
