#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# The firmware in the browser: firmware/src/APP.c, as tests/run_host.sh builds it, compiled to WebAssembly with clang (wasm32, wasm-ld:
# no Emscripten), plus the page. docs/EMULATOR.md.
#   web/emu/build.sh   ->  build/emu/{index.html, app.js, worklet.js, APP.wasm, fonts/, logo, favicon.svg}
#   APP          the app (default niwa): firmware/src/APP.c, which needs APP_STEPPED (platform.c)
#   EMU_PREVIEW  1 (default): the page says "(preview)" after the version; 0 drops it
# The page's own part is web/emu/app.js (the firmware's: replace it); the rest of web/emu is niwa's.
set -e
cd "$(dirname "$0")/../.."
export APP=${APP:-niwa}
command -v clang >/dev/null 2>&1 || { echo "emu: no clang"; exit 1; }
mkdir -p build/emu/fonts build/gen
gen() { out=build/gen/$1; tool=tools/$2; shift 2; [ "$out" -nt "$tool" ] || python3 "$tool" "$out" "$@" > /dev/null; }
gen ui_fonts.h gen_aa_font.py --preset inter-tight
gen ui_palettes.h gen_ui_palettes.py
VERSION=${APP_VER:-dev}
# -fno-builtin: libc.c's memset must stay a loop (as the firmware's flags, tools/build.py). The stack is 256 KiB, the memory 2 MiB.
clang --target=wasm32 -O2 -ffreestanding -fno-builtin -nostdlib -Wall -Wno-unused-function \
    -DNIWA_HOST -DNIWA_WEB "-DAPP_VER=\"$VERSION\"" -Ifirmware/src -Ibuild/gen \
    -Wl,--no-entry -Wl,-z,stack-size=262144 -Wl,--initial-memory=2097152 \
    -o "build/emu/$APP.wasm" "firmware/src/$APP.c"
[ "${EMU_PREVIEW:-1}" = 0 ] && PREVIEW='' || PREVIEW=' (preview)'
sed -e "s/@VERSION@/$VERSION/g" -e "s/@PREVIEW@/$PREVIEW/g" -e "s/@BUILD@/$(git rev-parse --short HEAD 2>/dev/null | sed 's/^/ · build /')/g" web/emu/index.html > build/emu/index.html
cp web/emu/worklet.js build/emu/
cp web/emu/app.js build/emu/
cp web/emu/fonts/DotGothic16-subset.woff web/emu/fonts/OFL.txt build/emu/fonts/
[ -f assets/logo.svg ] && cp assets/logo.svg build/emu/logo.svg
[ -f assets/logo-mark.svg ] && cp assets/logo-mark.svg build/emu/favicon.svg
echo "emu: build/emu ($(wc -c < "build/emu/$APP.wasm" | tr -d ' ') B wasm, $VERSION)"
