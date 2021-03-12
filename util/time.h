#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <util/atomic.h>
#include "hardware/AFSK.h"
#include "hardware/LED.h"
#include "hardware/sdcard/diskio.h"

#define UNIX_EPOCH_OFFSET 946684800

#define DIV_ROUND(dividend, divisor)  (((dividend) + (divisor) / 2) / (divisor))

typedef int32_t ticks_t;
typedef int32_t mtime_t;
volatile ticks_t _clock;

volatile uint32_t _rtc_seconds;
volatile uint16_t _rtc_seconds_accu;

static inline ticks_t timer_clock(void) {
    ticks_t result;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result = _clock;
    }

    return result;
}

inline ticks_t ms_to_ticks(mtime_t ms) {
    return ms * DIV_ROUND(CLOCK_TICKS_PER_SEC, 1000);
}

inline mtime_t ticks_to_ms(ticks_t ticks) {
    return DIV_ROUND(ticks, DIV_ROUND(CLOCK_TICKS_PER_SEC, 1000));
}

static inline mtime_t milliseconds(void) {
    return ticks_to_ms(timer_clock());
}

static inline uint32_t rtc_seconds(void) {
    uint32_t result;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result = _rtc_seconds;
    }

    return result;
}

static inline mtime_t rtc_milliseconds(void) {
    return ticks_to_ms(timer_clock() % CLOCK_TICKS_PER_SEC);
}

static inline uint32_t rtc_unix_timestamp(void) {
    uint32_t result;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result = _rtc_seconds;
    }

    return result+UNIX_EPOCH_OFFSET;
}

static inline void rtc_set_seconds(uint32_t seconds) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        _rtc_seconds = seconds;
    }
}

inline void cpu_relax(void) {
    // Do nothing!
}

static inline void delay_ms(unsigned long ms) {
    ticks_t start = timer_clock();
    unsigned long n_ticks = ms_to_ticks(ms);
    while (timer_clock() - start < n_ticks) {
        cpu_relax();
    }
}
#endif