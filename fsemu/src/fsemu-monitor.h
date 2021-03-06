#ifndef FSEMU_MONITOR_H_
#define FSEMU_MONITOR_H_

#include "fsemu-config.h"
#include "fsemu-types.h"

#define FSEMU_MONITOR_FLAG_LEFT (1 << 0)
#define FSEMU_MONITOR_FLAG_MIDDLE_LEFT (1 << 1)
#define FSEMU_MONITOR_FLAG_MIDDLE_RIGHT (1 << 2)
#define FSEMU_MONITOR_FLAG_RIGHT (1 << 3)

#define FSEMU_MONITOR_MAX_COUNT 8

typedef struct fsemu_monitor_t {
    int index;
    int flags;
    fsemu_rect_t rect;
    int refresh_rate;
    double scale;
} fsemu_monitor_t;

#ifdef __cplusplus
extern "C" {
#endif

void fsemu_monitor_init(void);
int fsemu_monitor_count(void);
bool fsemu_monitor_get_by_index(int index, fsemu_monitor_t *monitor);
bool fsemu_monitor_get_by_flag(int flag, fsemu_monitor_t *monitor);
bool fsemu_monitor_get_by_rect(fsemu_rect_t *rect, fsemu_monitor_t *monitor);

#ifdef __cplusplus
}
#endif

#endif  // FSEMU_MONITOR_H_
