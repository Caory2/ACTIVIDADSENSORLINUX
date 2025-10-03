#!/usr/bin/env bash
set -eu

WORKDIR=$(dirname "$0")
BIN="$(pwd)/build/assignment-sensor"
LOG=/tmp/assignment_sensor_test_happy.log

rm -f "$LOG"
"$BIN" --interval 1 --logfile "$LOG" --device /dev/urandom &
PID=$!
sleep 3
kill -TERM "$PID" || true
wait "$PID" 2>/dev/null || true

COUNT=$(wc -l < "$LOG" 2>/dev/null || echo 0)
if [ "$COUNT" -ge 2 ]; then
  echo "happy: PASS ($COUNT lines)"
  exit 0
else
  echo "happy: FAIL (lines=$COUNT)"
  tail -n 50 "$LOG" || true
  exit 1
fi
