#include "common.h"
#include "config.h"

#include <errno.h>
#include <time.h>

long monotonic_ms(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

long elapsed_sim_time(long start_wall_ms) {
    long elapsed_ms = monotonic_ms() - start_wall_ms;

    if (elapsed_ms < 0) {
        elapsed_ms = 0;
    }

    return elapsed_ms / DEFAULT_TIME_UNIT_MS;
}

void sleep_ms(long ms) {
    struct timespec req;

    if (ms <= 0) {
        return;
    }

    req.tv_sec = ms / 1000L;
    req.tv_nsec = (ms % 1000L) * 1000000L;

    while (nanosleep(&req, &req) != 0 && errno == EINTR) {
    }
}

void sleep_sim_units(int units) {
    if (units <= 0) {
        return;
    }

    sleep_ms((long)units * DEFAULT_TIME_UNIT_MS);
}

void sleep_until_sim_time(long start_wall_ms, int target_time) {
    long target_ms;
    long now;

    if (target_time <= 0) {
        return;
    }

    target_ms = start_wall_ms + (long)target_time * DEFAULT_TIME_UNIT_MS;
    while ((now = monotonic_ms()) < target_ms) {
        sleep_ms(target_ms - now);
    }
}
