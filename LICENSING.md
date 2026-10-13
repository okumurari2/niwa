# Licensing

niwa is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). You may use, study, change and share it. If you give someone a firmware built from it, give them its
complete source under the same licence too.

## Who made what

| | |
| --- | --- |
| The platform: `firmware/hal/`, `firmware/loader/`, `crt0.S`, `app.ld`, `firmware/src/` (`usb.c`, `ota.c`, `recovery.c`, `lcd.c`, `gfx.c`, `libc.c`, `bootguard.h`, `uac_fir.h`), `tools/` | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca)), with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89) |
| The app `firmware/src/niwa.c`, the wiring `platform.c`, the host mode `tests/host.c`, the docs as they came (`API.md`, `RULES.md`, `MAKE-IT-YOURS.md`), all from [Dinghy](https://github.com/zvenson/dinghy) | zvenson |
| `firmware/src/fifths.h`, `tests/fifths_test.c`, `tools/upstream.py` and `tools/upstream.json`, the docs `PHILOSOPHY.md`, `UI.md`, `LESSONS.md`, `HARVEST.md`, `UPSTREAM.md`, `decisions.md`. The circle of fifths and the screen rules come from maku, which is GPL-3.0-only and derived from Felucca | okumurari2 |

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Inter Tight (maku's UI font) | SIL OFL 1.1 | not in the tree yet: `assets/fonts/` when `gfx.c` is taken from maku (docs/HARVEST.md) |
| Terminus font 8x16 (ter-u16n), rendered into `firmware/gen/felucca_font.h` | SIL OFL 1.1 | `assets/fonts/Terminus-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## The logo

`assets/logo.png` (the original, 1024 px) and `assets/logo.svg` / `logo-mark.svg` (traced from it: three flat colours, the mark
alone without the background in `logo-mark.svg`) were made for niwa by okumurari2, the image generated with Gemini. **The
licence of the logo is not stated yet** (open: GPL-3.0-only like the rest, or for niwa itself only, as Dinghy's was; a
machine-generated image may also not be protected by copyright in some countries). Until it is stated, use it only to refer to niwa.
Dinghy's logo, the designburg signet (Copyright (C) designburg, all rights reserved), is not in this tree. A firmware built on
niwa uses its own logo.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments; "M-VAVE" and "FM-1" belong to their
owners. niwa is not made or endorsed by them.
