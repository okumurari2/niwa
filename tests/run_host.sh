#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run firmware/src/niwa.c on this computer (tests/host.c): build/host/niwa.wav and the screens as PNG.
set -e
cd "$(dirname "$0")/.."
export APP=${APP:-niwa}                                          # the app: firmware/src/APP.c (a firmware built on niwa: its own name)
mkdir -p build/host build/gen
if [ -z "$PYTHON" ]; then
    if [ -x ".venv/bin/python3" ]; then
        PY=".venv/bin/python3"
    elif [ -x "../.venv/bin/python3" ]; then
        PY="../.venv/bin/python3"
    else
        PY="python3"
    fi
else
    PY="$PYTHON"
fi
gen() { out=build/gen/$1; tool=tools/$2; shift 2; [ "$out" -nt "$tool" ] || "$PY" "$tool" "$out" "$@" > /dev/null; }
gen ui_fonts.h gen_aa_font.py --preset inter-tight        # the UI fonts and palettes (docs/UI.md), made once
gen ui_palettes.h gen_ui_palettes.py
[ build/gen/app_fonts.h -nt tools/app_fonts.txt ] && [ build/gen/app_fonts.h -nt tools/gen_aa_font.py ] || "$PY" tools/gen_aa_font.py build/gen/app_fonts.h --faces-file tools/app_fonts.txt > /dev/null   # the firmware's own faces (empty without tools/app_fonts.txt)
cc -O2 -Wall -Wno-unused-function -DNIWA_HOST -DAPP_VER="\"${APP_VER:-dev}\"" -Ifirmware/src -Ibuild/gen -o build/host/$APP firmware/src/$APP.c
build/host/$APP build/host/$APP
"$PY" - <<'PY'
import os
from pathlib import Path
from PIL import Image
for p in sorted(Path("build/host").glob("%s-*.ppm" % os.environ.get("APP", "niwa"))):
    Image.open(p).resize((480, 480), Image.NEAREST).save(p.with_suffix(".png"))
    p.unlink()
    print("screen ->", p.with_suffix(".png"))
PY
