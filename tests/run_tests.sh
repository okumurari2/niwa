#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The host tests: no FM-1 and no toolchain needed (Python 3 with Pillow, a C compiler).
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
export PYTHON="$PY"
mkdir -p build
cc -O2 -Wall -Ifirmware/src -o build/fifths_test tests/fifths_test.c && build/fifths_test
cc -O2 -Wall -Ifirmware/src -o build/soft_test tests/soft_test.c && build/soft_test
cc -O2 -Wall -Ifirmware/src -o build/battery_test tests/battery_test.c && build/battery_test
sh tests/run_host.sh > /dev/null && echo "run_host: ok (build/host/)"
"$PY" tests/text_spacing_test.py build/gen/ui_fonts.h | tail -1
"$PY" tools/gen_aa_font.py build/app_fonts_sample.h --faces-file tests/app_fonts_sample.txt | tail -1   # the firmware's own faces (tools/app_fonts.txt)
grep -q "AF_XL;\|AF_XL =" build/app_fonts_sample.h && echo "app_fonts: ok" || { echo "app_fonts: FAILED"; exit 1; }
# the browser build (web/emu, docs/EMULATOR.md): clang for wasm32 with wasm-ld, and node
if command -v node >/dev/null 2>&1 && command -v wasm-ld >/dev/null 2>&1 && clang --target=wasm32 -c -x c /dev/null -o /dev/null 2>/dev/null; then
    sh web/emu/build.sh
    node web/emu/emu_test.mjs build/emu/niwa.wasm
else
    echo "== skip the browser build (needs node, wasm-ld and clang with the wasm32 target)"
fi
"$PY" tools/upstream.py check --no-fetch || true     # informational: needs the upstream checkouts (tools/upstream.py)
