Manual test notes

1) Happy path
- Build and install per README.
- Start service: sudo systemctl enable --now assignment-sensor.service
- After a few intervals: tail /tmp/assignment_sensor.log

2) Fallback path
- Make /tmp non-writable and start; verify /var/tmp/assignment_sensor.log is used.

3) SIGTERM handling
- sudo systemctl stop assignment-sensor.service
- Ensure no partial lines and process exits.

4) Failure path
- Start binary with --device /dev/fake0 and confirm it exits with non-zero.
