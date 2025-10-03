#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>

static volatile sig_atomic_t stop_requested = 0;

void handle_sigint(int signo) {
    (void)signo;
    stop_requested = 1;
}

// Write formatted message to file with flush
int safe_write_log(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    if (r < 0) return -1;
    if (fflush(f) != 0) return -1;
    // try sync if possible
    int fd = fileno(f);
    if (fd >= 0) fsync(fd);
    return 0;
}

// produce ISO8601 UTC string with nanoseconds
void iso_now(char *buf, size_t buflen) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    gmtime_r(&ts.tv_sec, &tm);
    // format: YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ
    snprintf(buf, buflen, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             (long)ts.tv_nsec);
}

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [--interval <seconds>] [--logfile <path>] [--device <path>]\n", prog);
}

int ensure_dir_for_path(const char *path) {
    char *dup = strdup(path);
    if (!dup) return -1;
    char *dir = dirname(dup);
    if (!dir) { free(dup); return -1; }
    int r = mkdir(dir, 0755);
    if (r < 0 && errno != EEXIST) { free(dup); return -1; }
    free(dup);
    return 0;
}

int open_log_file(const char *path, FILE **out) {
    // Ensure directory exists
    // For simplicity, try creating parent directory; if fails, continue since /tmp exists
    // Use fopen with append
    FILE *f = fopen(path, "a");
    if (!f) return -1;
    *out = f;
    return 0;
}

int main(int argc, char **argv) {
    int interval = 5;
    const char *logfile = "/tmp/assignment_sensor.log";
    const char *device = "/dev/urandom";

    // Simple arg parse
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval = atoi(argv[++i]);
            if (interval <= 0) interval = 5;
        } else if (strcmp(argv[i], "--logfile") == 0 && i + 1 < argc) {
            logfile = argv[++i];
        } else if (strcmp(argv[i], "--device") == 0 && i + 1 < argc) {
            device = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    // Open device
    int devfd = open(device, O_RDONLY);
    if (devfd < 0) {
        fprintf(stderr, "fatal: cannot open device %s: %s\n", device, strerror(errno));
        return 2;
    }

    // Open log file, fallback to /var/tmp
    FILE *logf = NULL;
    if (open_log_file(logfile, &logf) != 0) {
        const char *fallback = "/var/tmp/assignment_sensor.log";
        if (open_log_file(fallback, &logf) != 0) {
            fprintf(stderr, "fatal: cannot open log %s or fallback %s\n", logfile, fallback);
            close(devfd);
            return 3;
        } else {
            logfile = fallback;
        }
    }

    // Setup signal handling
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    unsigned char buf[8];
    char tsbuf[64];

    while (!stop_requested) {
        ssize_t r = read(devfd, buf, sizeof(buf));
        if (r < 0) {
            // Non-fatal: log warning, sleep then continue
            fprintf(stderr, "warning: read device error: %s\n", strerror(errno));
            sleep(interval);
            continue;
        }
        if (r != (ssize_t)sizeof(buf)) {
            fprintf(stderr, "warning: short read from device: %zd bytes\n", r);
            sleep(interval);
            continue;
        }

        iso_now(tsbuf, sizeof(tsbuf));
        // build hex string
        char hexval[2 * sizeof(buf) + 3];
        char *p = hexval;
        p += sprintf(p, "0x");
        for (size_t i = 0; i < sizeof(buf); i++) {
            sprintf(p, "%02x", buf[i]);
            p += 2;
        }

        if (safe_write_log(logf, "%s | %s\n", tsbuf, hexval) != 0) {
            fprintf(stderr, "warning: write log error: %s\n", strerror(errno));
        }

        // Sleep in small increments so we can respond to signals faster
        int slept = 0;
        while (slept < interval && !stop_requested) {
            sleep(1);
            slept++;
        }
    }

    // cleanup
    if (logf) {
        fflush(logf);
        int fd = fileno(logf);
        if (fd >= 0) fsync(fd);
        fclose(logf);
    }
    close(devfd);
    return 0;
}
