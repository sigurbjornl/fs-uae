#ifndef FSEMU_AUDIO_BUFFER_H_
#define FSEMU_AUDIO_BUFFER_H_

#include "fsemu-common.h"
#include "fsemu-config.h"
#include "fsemu-types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fsemu_audio_buffer_init(void);
void fsemu_audio_buffer_clear(void);

int fsemu_audio_buffer_fill(void);
int fsemu_audio_buffer_fill_ms(void);
int64_t fsemu_audio_buffer_fill_us(void);

void fsemu_audio_buffer_update(const void *data, int size);

void fsemu_audio_buffer_frame_done(void);

void fsemu_audio_buffer_write_silence(int size);
void fsemu_audio_buffer_write_silence_ms(int ms);

#ifdef FSEMU_SAMPLERATE
double fsemu_audio_buffer_adjustment(void);
void fsemu_audio_buffer_set_adjustment(double adjustment);
#endif

int fsemu_audio_buffer_calculate_target(void);

double fsemu_audio_buffer_calculate_adjustment(void);

/*
extern int fsemu_audio_buffer.size;
extern volatile uint8_t *fsemu_audio_buffer;
extern volatile uint8_t *fsemu_audio_buffer.end;
extern volatile uint8_t *volatile fsemu_audio_buffer.read;
extern volatile uint8_t *volatile fsemu_audio_buffer.write;
*/

typedef struct {
    volatile uint8_t *data;
    int size;
    uint8_t volatile *end;
    uint8_t volatile *volatile read;
    uint8_t volatile *volatile write;
    int volatile underrun;
    int volatile add_silence;
} fsemu_audio_buffer_t;

extern fsemu_audio_buffer_t fsemu_audio_buffer;

#ifdef __cplusplus
}
#endif

#ifdef FSEMU_INTERNAL

#endif

#endif  // FSEMU_AUDIO_BUFFER_H_
