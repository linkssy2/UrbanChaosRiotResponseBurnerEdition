#ifndef UC_AUDIO_FADE_H
#define UC_AUDIO_FADE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Retained actual instance64/68/60; all track commands share this owner. */
typedef struct UCAudioFadeState {uint32_t flags;int32_t phase;int16_t gain;} UCAudioFadeState;
/* Whole248E80. xfde is the actual current event's 12-byte row, NULL if absent.
 * enqueue must copy the44-byte command for each ordered actual track, with the
 * actual engine clock. Zero tracks still changes phase/flags. No voice stop. */
int uc_audio_fade_begin(UCAudioFadeState*,uint32_t flags,const uint8_t*xfde,
 uint32_t tracks,uint64_t now,int(*enqueue)(void*,uint32_t,uint64_t,const uint8_t[44]),void*);
/* Whole241130 numeric/branch policy plus240010 rescheduling. Admit only the
 * actual248E80 opcode0 command. Engine dispatch supplies now, not prior due time.
 * On interior gain publishes reschedule then mix-all in that order. At the gain
 * boundary clears1000 without mix. The caller retires this command if repeat=0;
 * actual instance lifecycle then owns eventual voice stop. */
int uc_audio_fade_tick(UCAudioFadeState*,const uint8_t command[44],uint64_t now,
 int(*reschedule)(void*,uint64_t),int(*mix_all)(void*),void*,uint8_t*repeat);
/* Source24A6F0 ordinary-track fade completion gate. Call in instance update,
 * in live track order, after scheduled commands. stop is exact24AA60(0), hence
 * 24A900(0,0,0), and must publish phase7/cancel actual commands/stop voices.
 * Ordinary status/retirement follows through the existing real owner. */
int uc_audio_fade_transition(UCAudioFadeState*,uint16_t track_flags,int(*stop)(void*),void*);
#ifdef __cplusplus
}
#endif
#endif
