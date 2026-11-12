#!/bin/bash
# usage: run.sh DISPLAY_NUM EFFECT KIND...
d=$1; eff=$2; shift 2
for k in "$@"; do
  DISPLAY=:$d /workspaces/gluqlo/gluqlo -w 1024 -h 768 -effect $eff:$k -seed 3 >/tmp/kindtest/$eff-$k.log 2>&1 &
  pid=$!
  sleep ${DELAY:-5}
  if kill -0 $pid 2>/dev/null; then
    DISPLAY=:$d import -window root -crop 1024x768+128+16 /tmp/kindtest/$eff-$k.png
    echo "$eff:$k ok"
  else
    echo "$eff:$k CRASHED/EXITED"
  fi
  kill $pid 2>/dev/null; wait $pid 2>/dev/null
done
