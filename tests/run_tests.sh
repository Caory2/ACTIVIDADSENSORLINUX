#!/usr/bin/env bash
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

SCRIPTS=(tests/test_happy.sh tests/test_fallback.sh tests/test_sigterm.sh tests/test_failure.sh)
PASS=0
FAIL=0

for s in "${SCRIPTS[@]}"; do
  echo "---- running $s ----"
  chmod +x "$s"
  if "$s"; then
    PASS=$((PASS+1))
  else
    FAIL=$((FAIL+1))
  fi
done

echo "Tests completed: PASS=$PASS FAIL=$FAIL"
if [ "$FAIL" -ne 0 ]; then
  exit 1
fi
