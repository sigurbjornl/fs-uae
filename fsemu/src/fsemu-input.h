#ifndef FSEMU_INPUT_H_
#define FSEMU_INPUT_H_

#include "fsemu-action.h"
#include "fsemu-common.h"
#include "fsemu-inputdevice.h"
#include "fsemu-inputport.h"
#include "fsemu-key.h"
// #include "fsemu-scancodes.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSEMU_INPUT_MAX_PORTS 8

// FSEMU_INPUT_MAX_DEVICES cannot be more than 128, since an int8_t is
// sometimes used to hold device_index.
#define FSEMU_INPUT_MAX_DEVICES 16

typedef struct {
    int key;
    int mod;
    int action;
} fsemu_input_configure_keyboard_t;

typedef struct {
    const char *name;
    uint16_t value;
    int flags;
} fsemu_input_action_t;

void fsemu_input_add_action(fsemu_input_action_t *action);

void fsemu_input_add_actions(fsemu_input_action_t actions[]);

fsemu_error_t fsemu_input_add_device(fsemu_inputdevice_t *device);

void fsemu_input_add_port(fsemu_inputport_t *port);

// FIXME: Possibly temporary function
void fsemu_input_autofill_devices(void);

void fsemu_input_configure_keyboard(
    fsemu_input_configure_keyboard_t mapping[]);

void fsemu_input_handle_controller(int device_index, int slot, int16_t state);
void fsemu_input_handle_keyboard(fsemu_key_t scancode, bool pressed);
void fsemu_input_handle_mouse(int device_index, int slot, int16_t state);

void fsemu_input_init(void);

bool fsemu_input_next_action(uint16_t *action, int16_t *state);

fsemu_action_and_state_t fsemu_input_pack_action_state(uint16_t action,
                                                       int16_t state);

// Internal function used by fsemu_sdlinput.
void fsemu_input_process_action(uint16_t action, int16_t state);

void fsemu_input_reconfigure(void);

void fsemu_input_remove_device(fsemu_inputdevice_t *device);
void fsemu_input_remove_device_by_index(int device_index);

void fsemu_input_unpack_action_state(fsemu_action_and_state_t action_and_state,
                                     uint16_t *action,
                                     int16_t *state);

void fsemu_input_work(int timeout);

extern int fsemu_input_log_level;

#define fsemu_input_log(format, ...)                         \
    if (fsemu_input_log_level > 0) {                         \
        fsemu_log("[FSEMU] [INPUT] " format, ##__VA_ARGS__); \
    }

#ifdef __cplusplus
}
#endif

#endif  // FSEMU_INPUT_H_
