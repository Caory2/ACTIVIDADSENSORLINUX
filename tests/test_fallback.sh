#!/usr/bin/env bash
set -eu

BIN="$(pwd)/build/assignment-sensor"
LOG_UNWRITABLE="/root/assignment_sensor_test_fallback.log"
FALLBACK="/var/tmp/assignment_sensor.log"

rm -f "$FALLBACK"
set +e
"$BIN" --interval 1 --logfile "$LOG_UNWRITABLE" --device /dev/urandom &
PID=$!
sleep 2
kill -TERM "$PID" || true
wait "$PID" 2>/dev/null || true
set -e

if [ -f "$FALLBACK" ]; then
  echo "fallback: PASS (logged to $FALLBACK)"
  wc -l "$FALLBACK" || true
  exit 0
else
  echo "fallback: FAIL ($FALLBACK not created)"
  exit 1
fi
