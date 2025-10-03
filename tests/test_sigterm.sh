#!/usr/bin/env bash
set -eu

BIN="$(pwd)/build/assignment-sensor"
LOG=/tmp/assignment_sensor_test_sigterm.log

rm -f "$LOG"
"$BIN" --interval 1 --logfile "$LOG" --device /dev/urandom &
PID=$!
sleep 2
kill -TERM "$PID"
sleep 1
if kill -0 "$PID" 2>/dev/null; then
  echo "sigterm: FAIL (process still running)"
  kill -KILL "$PID" || true
  exit 1
fi

COUNT=$(wc -l < "$LOG" 2>/dev/null || echo 0)
if [ "$COUNT" -ge 1 ]; then
  echo "sigterm: PASS ($COUNT lines)"
  exit 0
else
  echo "sigterm: FAIL (no lines)"
  exit 1
fi
