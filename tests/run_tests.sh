#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The host tests: no FM-1 and no toolchain needed (Python 3 with Pillow, a C compiler).
set -e
cd "$(dirname "$0")/.."
mkdir -p build
cc -O2 -Wall -Ifirmware/src -o build/fifths_test tests/fifths_test.c && build/fifths_test
cc -O2 -Wall -Ifirmware/src -o build/soft_test tests/soft_test.c && build/soft_test
cc -O2 -Wall -Ifirmware/src -o build/battery_test tests/battery_test.c && build/battery_test
sh tests/run_host.sh > /dev/null && echo "run_host: ok (build/host/)"
python3 tests/text_spacing_test.py build/gen/ui_fonts.h | tail -1
python3 tools/upstream.py check --no-fetch || true     # informational: needs the upstream checkouts (tools/upstream.py)
