#!/bin/bash
# usage: draw_sky.sh FIRST LAST   -> /tmp/kindtest/out/skyharness-N.png and a contact sheet
ROOT=/workspaces/gluqlo
OUT=${OUT:-/tmp/kindtest/out}
mkdir -p "$OUT"
make -s -C "$ROOT" -j4 || exit 1
g++ -O1 -w $(pkg-config --cflags sdl3 sdl3-ttf fontconfig) \
	-o /tmp/draw_sky "$ROOT/dev/tools/draw_sky.cpp" \
	"$ROOT/src/"*.o $(pkg-config --libs sdl3 sdl3-ttf fontconfig) -lm || exit 1
/tmp/draw_sky "$1" "$2" "$OUT" || exit 1
cd "$OUT" || exit 1
for k in $(seq "$1" "$2"); do convert "skyharness-$k.bmp" "skyharness-$k.png"; rm -f "skyharness-$k.bmp"; done
montage $(for k in $(seq "$1" "$2"); do echo skyharness-$k.png; done) -tile 2x -geometry 640x240+2+2 "sheet_skyharness_$1_$2.png"
echo "$OUT/sheet_skyharness_$1_$2.png"
