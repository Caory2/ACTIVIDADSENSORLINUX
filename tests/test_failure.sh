#!/usr/bin/env bash
set -eu

BIN="$(pwd)/build/assignment-sensor"

set +e
"$BIN" --interval 1 --logfile /tmp/assignment_sensor_test_failure.log --device /dev/fake0 &
PID=$!
sleep 1
kill -0 "$PID" 2>/dev/null
EXIT=0
wait "$PID" 2>/dev/null || EXIT=$?
set -e

if [ "$EXIT" -ne 0 ]; then
  echo "failure: PASS (exit $EXIT)"
  exit 0
else
  echo "failure: FAIL (process exited 0 or still running)"
  kill -KILL "$PID" 2>/dev/null || true
  exit 1
fi
