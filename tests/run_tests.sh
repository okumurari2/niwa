#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The host tests: no FM-1 and no toolchain needed (Python 3 with Pillow, a C compiler).
set -e
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -Wall -Ifirmware/src -o build/fifths_test tests/fifths_test.c && build/fifths_test
cc -O2 -Wall -Ifirmware/src -o build/soft_test tests/soft_test.c && build/soft_test
cc -O2 -Wall -DTRACKS_LOGIC_ONLY -Ifirmware/src -o build/tracks_test tests/tracks_test.c && build/tracks_test
cc -O2 -Wall -Ifirmware/src -o build/battery_test tests/battery_test.c && build/battery_test
sh tests/run_host.sh > /dev/null && echo "run_host: ok (build/host/)"
python3 tests/text_spacing_test.py build/gen/ui_fonts.h | tail -1
# the browser build (web/emu, docs/EMULATOR.md): clang for wasm32 with wasm-ld, and node
if command -v node >/dev/null 2>&1 && command -v wasm-ld >/dev/null 2>&1 && clang --target=wasm32 -c -x c /dev/null -o /dev/null 2>/dev/null; then
    sh web/emu/build.sh
    node web/emu/emu_test.mjs build/emu/niwa.wasm
else
    echo "== skip the browser build (needs node, wasm-ld and clang with the wasm32 target)"
fi
python3 tools/upstream.py check --no-fetch || true     # informational: needs the upstream checkouts (tools/upstream.py)
