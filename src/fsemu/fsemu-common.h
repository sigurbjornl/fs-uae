#ifndef FSEMU_COMMON_H_
#define FSEMU_COMMON_H_

#ifdef FSEMU_INTERNAL

#include "fsemu-config.h"

#include <stdbool.h>

// FIXME: Temporarily disabled translations
#define _(x) x

#define fsemu_return_if_already_initialized() \
    static bool initialized;                  \
    if (initialized) {                        \
        return;                               \
    }                                         \
    initialized = true;

#define fsemu_init_once(var) \
    if (*(var)) {            \
        return;              \
    }                        \
    *(var) = true;

#endif

#endif  // FSEMU_COMMON_H_