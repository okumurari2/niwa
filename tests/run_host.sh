#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run firmware/src/niwa.c on this computer (tests/host.c): build/host/niwa.wav and the screens as PNG.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host build/gen
gen() { out=build/gen/$1; tool=tools/$2; shift 2; [ "$out" -nt "$tool" ] || python3 "$tool" "$out" "$@" > /dev/null; }
gen ui_fonts.h gen_aa_font.py --preset inter-tight        # the UI fonts and palettes (docs/UI.md), made once
gen ui_palettes.h gen_ui_palettes.py
cc -O2 -Wall -Wno-unused-function -DNIWA_HOST -DAPP_VER="\"${APP_VER:-dev}\"" -Ifirmware/src -Ibuild/gen -o build/host/niwa firmware/src/niwa.c
build/host/niwa build/host/niwa
python3 - <<'PY'
from pathlib import Path
from PIL import Image
for p in sorted(Path("build/host").glob("niwa-*.ppm")):
    Image.open(p).resize((480, 480), Image.NEAREST).save(p.with_suffix(".png"))
    p.unlink()
    print("screen ->", p.with_suffix(".png"))
PY
