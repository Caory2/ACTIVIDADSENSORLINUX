
BINARY = build/assignment-sensor
SRC = src/main.c

.PHONY: all build clean install uninstall test

CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra

all: build

build: $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $(BINARY) $(SRC)

clean:
	rm -rf build

install: build
	install -Dm755 $(BINARY) /usr/local/bin/assignment-sensor
	install -Dm644 systemd/assignment-sensor.service /etc/systemd/system/assignment-sensor.service


uninstall:
	systemctl disable --now assignment-sensor.service || true
	rm -f /usr/local/bin/assignment-sensor
	rm -f /etc/systemd/system/assignment-sensor.service
	systemctl daemon-reload || true

test: build
	@./tests/run_tests.sh
