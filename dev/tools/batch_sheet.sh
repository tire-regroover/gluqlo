#!/bin/bash
# Capture a range of new events and make one contact sheet per type.
#   usage: batch_sheet.sh TAG HILL_FIRST HILL_LAST SKY_FIRST SKY_LAST
# Use 0 0 for a type to skip it. Writes $OUT/sheet_TAG_hill.jpg / sheet_TAG_sky.jpg
# and copies each frame to dev/captures/events/ as a jpg.
ROOT=/workspaces/gluqlo
OUT=${OUT:-/tmp/kindtest/out}
export OUT
tag=$1
cd "$ROOT" || exit 1
if [ "$2" != 0 ]; then
	for k in $(seq "$2" "$3"); do
		"$ROOT/dev/tools/check_event.sh" hill "$k" 12 || exit 1
		convert "$OUT/hill_event-$k.png" -quality 80 "$ROOT/dev/captures/events/hill_event-$k.jpg"
	done
	(cd "$OUT" && montage $(for k in $(seq "$2" "$3"); do echo hill_event-$k.png; done) -tile 3x -geometry 640x480+2+2 "sheet_${tag}_hill.jpg")
fi
if [ "$4" != 0 ]; then
	for k in $(seq "$4" "$5"); do
		"$ROOT/dev/tools/check_event.sh" sky "$k" 3 || exit 1
		convert "$OUT/rare_sky-$k.png" -quality 80 "$ROOT/dev/captures/events/rare_sky-$k.jpg"
	done
	(cd "$OUT" && montage $(for k in $(seq "$4" "$5"); do echo rare_sky-$k.png; done) -tile 3x -geometry 640x480+2+2 "sheet_${tag}_sky.jpg")
fi
echo done
