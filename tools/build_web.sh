#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
command -v emcc >/dev/null || { echo "Activate Emscripten first." >&2; exit 1; }
emcc -O2 -std=c99 -ffp-contract=off dsp/acid.c dsp/web_bridge.c \
  --no-entry -sSTANDALONE_WASM -sFILESYSTEM=0 -sINITIAL_MEMORY=2097152 \
  -sSTACK_SIZE=131072 -o web/engine.wasm
python3 tools/manifest.py
python3 base/tools/build.py . --web
mkdir -p web/hardware
cp build/emu/index.html build/emu/app.wasm build/emu/worklet.js web/hardware/
test ! -d build/emu/fonts || cp -R build/emu/fonts web/hardware/
python3 tools/prepare_hardware_web.py
touch web/.nojekyll
echo "Browser builds: web/index.html and web/hardware/index.html"
