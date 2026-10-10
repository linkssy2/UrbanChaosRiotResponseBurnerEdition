#ifndef UC_NATIVE_AUDIO_SHUFFLE_EVENT_H
#define UC_NATIVE_AUDIO_SHUFFLE_EVENT_H
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual footstep opcode1 flags00A4, repeat1, selector uint16 index with upper
 * word0. Validated projection reuses the qualified opcode1 parser/executor;
 * all external hooks still receive the ORIGINAL command, including its flags.
 * The original cached voice, pitch->volume->create->start ordering is retained.
 * create_voice is a required completing owner: it must run actual24D000 mode4
 * selection (uc_audio_selector_shuffle) against persistent bank data, on the
 * SAME audio RNG after pitch/volume, then resolve the selected reference.
 * This adapter does not implement resources, policy programs, spatial mixing,
 * scheduling, or playback. Existing resident playback admission is unchanged.
 * All ordinary audio_event service/lifetime/alias contracts apply. Rejection
 * precedes effects; a failing completing hook is a contract violation, not a
 * retryable command. No guest code or alternate random stream is used. */
UCAudioStatus uc_audio_shuffle_command_read(const void *,size_t,UCAudioCommand *);
UCAudioStatus uc_audio_shuffle_event_execute(UCAudioTrackState *,const UCAudioCommand *,
    UCAudioRandom *,const UCAudioEventServices *);
#ifdef __cplusplus
}
#endif
#endif
