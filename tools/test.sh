#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
cc -O2 -g -std=c99 -ffp-contract=off -Wall -Wextra -Werror -Wno-misleading-indentation \
  -Idsp dsp/acid.c tests/engine_test.c -lm -o build/tests/engine_test
build/tests/engine_test
python3 base/tools/build.py . --host
build/host/acid-fm1_host tests/panel.txt build/tests
