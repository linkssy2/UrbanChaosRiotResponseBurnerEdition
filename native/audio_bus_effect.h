#ifndef UC_AUDIO_BUS_EFFECT_H
#define UC_AUDIO_BUS_EFFECT_H
#include "audio_group.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioBusEnvelope {
 float target,current,start,attack,hold,release,inverse_attack,inverse_release;
} UCAudioBusEnvelope;
/* Complete20F750 payload and20E320 single retained map member. The real
 * sound-manager owns its case-sensitive name map, creation flag and clock.
 * step publishes current even when expired; remove only AFTER publication.
 * Source holds the last sampled current in the hold and expired intervals. */
int uc_audio_bus_envelope_begin(UCAudioBusEnvelope*,int inserted,float clock,
 float cutoff,float attack,float hold,float release);
int uc_audio_bus_envelope_step(UCAudioBusEnvelope*,float clock,int *remove);
/* Whole247A80 mode2 ->2440F0: ratio0..4, RN4096log2, low signed16 store.
 * This changes pitch, not volume. The owner must update all real live voices. */
int uc_audio_bus_pitch(UCAudioGroup*,float ratio);
#ifdef __cplusplus
}
#endif
#endif
