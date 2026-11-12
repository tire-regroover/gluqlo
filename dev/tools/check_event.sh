#!/bin/bash
# Build gluqlo, run one event kind on a virtual display and save a screenshot.
#   usage: check_event.sh sky|hill KIND [SECONDS_BEFORE_SCREENSHOT]
# Sky events are short (4-7 s), so they are captured after 2 s by default; hill
# events last half a minute or more, so they are captured after 10 s.
ROOT=/workspaces/gluqlo
OUT=${OUT:-/tmp/kindtest/out}
mkdir -p "$OUT"
type=$1
kind=$2
case $type in
	sky) effect=rare_sky; delay=${3:-2};;
	hill) effect=hill_event; delay=${3:-10};;
	*) echo "usage: $0 sky|hill KIND [SECONDS]"; exit 2;;
esac
if make -s -C "$ROOT" 2>&1 | grep -E "error|warning"; then
	echo "BUILD-PROBLEM"
	exit 1
fi
display=${DISP:-90}
if ! pgrep -f "Xvfb :$display" >/dev/null; then
	Xvfb ":$display" -screen 0 1280x800x24 >/dev/null 2>&1 &
	sleep 1
fi
DISPLAY=":$display" "$ROOT/gluqlo" -w 1024 -h 768 -effect "$effect:$kind" -seed 3 >"$OUT/$effect-$kind.log" 2>&1 &
pid=$!
sleep "$delay"
if kill -0 $pid 2>/dev/null; then
	DISPLAY=":$display" import -window root -crop 1024x768+128+16 "$OUT/$effect-$kind.png"
	echo "$effect:$kind ok"
else
	echo "$effect:$kind EXITED"
	cat "$OUT/$effect-$kind.log"
fi
kill $pid 2>/dev/null
wait $pid 2>/dev/null
