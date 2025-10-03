# Assignment: systemd sensor logger

This repository provides a small C program and a systemd unit that sample a simulated sensor and append timestamped values to a log file.

Contents
- `src/main.c` — sampler program (C)
- `build/assignment-sensor` — compiled binary (after `make build`)
- `systemd/assignment-sensor.service` — example unit file
- `tests/` — automated test scripts and runner
- `ai/` — prompt-log, reflection and provenance

Prerequisites
- gcc (or compatible C compiler)
- make
- systemd (for installing and enabling the unit)

Clone & build

git clone <your-repo-url>
cd ACTIVIDADSENSORLINUX
make build

Artifacts
- Binary: `build/assignment-sensor`

Install & enable

sudo make install
sudo systemctl daemon-reload
sudo systemctl enable --now assignment-sensor.service

CLI / runtime interface (detailed)

Usage:

assignment-sensor [--interval <seconds>] [--logfile <path>] [--device <path>]

Flags and defaults:
- `--interval <seconds>`: sampling interval in seconds (integer). Default: 5
- `--logfile <path>`: primary destination logfile. Default: `/tmp/assignment_sensor.log`
- `--device <path>`: device or file to read raw bytes from. Default: `/dev/urandom`

Behavior and guarantees:
- On start the program opens the configured `--device` for reading. If the device cannot be opened the program exits with a non-zero code (fatal init error).
- The program attempts to open `--logfile` for appending. If opening the primary path fails, it falls back to `/var/tmp/assignment_sensor.log`. If neither can be opened the program exits with non-zero.
- The main loop: read 8 bytes per sample, compute an ASCII hex representation prefixed with `0x`, and append a single line with an ISO-8601 UTC timestamp (RFC3339Nano) followed by ` | ` and the value. Each sample is flushed to disk (fflush + fsync attempt) to avoid partial lines on crashes.
- Signal handling: the program handles `SIGINT` and `SIGTERM`. On receiving either it stops the loop, flushes and closes the logfile and exits with code 0.

Log format
- One line per sample, example:

  2025-10-03T04:55:10.372784951Z | 0xd8482ad47017050a

- Timestamp: RFC3339 with nanoseconds (UTC)
- Value: hex representation of 8 raw bytes read from the device, prefixed by `0x`.

Exit codes
- `0`: normal termination (including when stopped via SIGTERM)
- `2`: fatal init error opening device
- `3`: fatal init error opening log and fallback
- non-zero: other runtime errors (documented on stderr)

Examples

Run interactively, sampling every 2 seconds:

./build/assignment-sensor --interval 2 --logfile /tmp/assignment_sensor.log

Run using a synthetic file as device (for deterministic tests):

./build/assignment-sensor --device /path/to/sample.bin --logfile /tmp/test.log

Systemd unit

The example unit is `systemd/assignment-sensor.service`. Basic recommended contents:

[Unit]
Description=Mock sensor logger (assignment)
After=multi-user.target

[Service]
Type=simple
ExecStart=/usr/local/bin/assignment-sensor --interval 5 --logfile /tmp/assignment_sensor.log --device /dev/urandom
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target

If you want the service to run as a non-root user, add `User=<username>` under `[Service]` and ensure the chosen `--logfile` (or fallback) is writable by that user.

Testing

There are automated tests in `tests/`. Run them with:

make test

This runs the following checks:
- happy: starts the binary, waits a few samples, sends SIGTERM and ensures lines were appended
- fallback: attempts to use an unwritable primary path to verify fallback to `/var/tmp`
- sigterm: validates clean shutdown on SIGTERM
- failure: runs with a non-existent device (`/dev/fake0`) and expects an error

Uninstall

sudo make uninstall

Notes and design rationale
- Device choice: `/dev/urandom` is used as a safe, available, non-blocking source for pseudo-random bytes. It avoids needing any special hardware or privileges. A deterministic synthetic counter or test file can be used with `--device` during tests.
- Logging: the program uses explicit flushes and attempts to call `fsync()` on the logfile descriptor to avoid partial lines if the process is killed. This meets the requirement for line-buffered writes or equivalent.

AI evidence
- See `ai/prompt-log.md`, `ai/reflection.md` and `ai/provenance.json` for the audit trail of assistance.

If you want, I can also add:
- `make package` to create a tarball with binary + unit file
- journald integration (log to stdout/stderr and rely on systemd journald)
- log rotation helper

Detailed technical notes (how `assignment-sensor` works)

Contract (inputs / outputs / error modes)
- Inputs:
  - CLI flags: `--interval` (int seconds), `--logfile` (path), `--device` (path)
  - Device: any file or character device readable by the process that yields bytes (e.g., `/dev/urandom`, a FIFO or a regular file).
- Outputs:
  - Appends lines to a logfile. Each line: `RFC3339Nano_TIMESTAMP | 0xHEXVALUE` (8 bytes in hex).
  - Returns 0 on clean shutdown; non-zero on fatal init errors.
- Error modes / exit behavior:
  - If the device cannot be opened at startup, the program prints an error to stderr and exits with code 2.
  - If neither the configured logfile nor the fallback (`/var/tmp/assignment_sensor.log`) can be opened, program exits with code 3.
  - Runtime read/write errors are logged to stderr but do not necessarily terminate the program.

Edge cases and how they are handled
- Short reads: if reading the device returns fewer than 8 bytes, the program logs a warning to stderr and retries after the interval.
- Device blocking: `/dev/random` can block if entropy is low; use `/dev/urandom` for non-blocking behavior (recommended).
- Logfile permissions: if the process lacks write permission for the primary logfile it attempts the documented fallback. If running as non-root, ensure the chosen path is writable.
- Partial writes: the program calls `fflush()` and attempts `fsync()` after each line to minimize the risk of partial lines on crash/kill.

Permissions and systemd notes
- Default example unit writes to `/tmp/assignment_sensor.log`. If you want the service to run as a dedicated user, update the unit with `User=sensor` and create the directory with appropriate ownership:

```bash
sudo useradd --system --no-create-home --shell /usr/sbin/nologin sensor
sudo install -d -o sensor -g sensor -m 0755 /var/log/assignment_sensor
# then update ExecStart to use --logfile /var/log/assignment_sensor/assignment_sensor.log
```

Debugging and troubleshooting
- Check process status:

```bash
systemctl status assignment-sensor.service
journalctl -u assignment-sensor.service --since "5 minutes ago"
```

- Run the binary directly for verbose stderr messages (useful during development):

```bash
./build/assignment-sensor --interval 1 --logfile /tmp/test.log --device /dev/urandom
```

- If logfile is not appearing, check permissions and fallback:

```bash
ls -l /tmp/assignment_sensor.log /var/tmp/assignment_sensor.log
```

Development notes (code pointers)
- Main loop and signal handling: `src/main.c` sets up `SIGINT`/`SIGTERM` handlers that set a flag; the loop checks this flag and exits cleanly.
- Time formatting: ISO8601 with nanoseconds produced by `clock_gettime()` + `gmtime_r()` and formatted as RFC3339Nano.
- IO: reads 8 bytes per sample using `read()` and writes lines via `fprintf()` + `fflush()` + `fsync()`.

Try it: quick checks

Build and run one-off sampler for demo:

```bash
make build
./build/assignment-sensor --interval 1 --logfile /tmp/demo_assignment_sensor.log
tail -n 20 /tmp/demo_assignment_sensor.log
```

Run automated tests:

```bash
make test
```

Questions / next additions
- Want the program to log to stdout/stderr instead and let systemd/journald collect logs? (simpler for systemd deployments)
- Want log rotation support (e.g., integrate with `logrotate` or rotate by size)?

