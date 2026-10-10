#ifndef UC_NATIVE_AUDIO_LIVE_MIX_H
#define UC_NATIVE_AUDIO_LIVE_MIX_H
#include "audio_device_buffer.h"
#include "audio_group.h"
#ifdef __cplusplus
extern "C" {
#endif
enum UCAudioLiveMixOperation {UC_AUDIO_LIVE_GAIN,UC_AUDIO_LIVE_PITCH,UC_AUDIO_LIVE_FILTER,UC_AUDIO_LIVE_ALL_GAINS};
typedef struct UCAudioLiveMixServices {
 void *context;
 int (*preflight)(void *,const UCAudioInstance *,const UCAudioInstanceTrack *,unsigned); /* pure0admits */
 const UCAudioGroup *(*group)(void *,const void *identity); /* pure actual scalar projection */
 /* Borrow actual+68/+6C float slots for this SAME group. Its parent field is
  * the sole parent authority; no copied parallel group/filter hierarchy. */
 int (*filter_fields)(void *,const UCAudioGroup *,const float **cutoff,const float **resonance);
 void (*gain)(void *,void *backend,uint8_t stream,int32_t);
 void (*pitch)(void *,void *backend,uint8_t stream,int32_t);
 void (*filter)(void *,void *backend,uint8_t stream,const uint32_t coefficients[6]);
} UCAudioLiveMixServices;
/* Actual248C50/2495C0/248B80 and ordinary251A20/251A50/251D80 receivers.
 * The track's exact class QueryInterface(4FED44) is the pure current voice
 * field projection; voice/device/group identities belong to existing owners.
 * Instance.sound is NULL or actual immutable>=12bytes. Instance.buffer is
 * NULL or actualUCAudioBuffer. The concrete timing query2491A0 reads the same
 * instance track ring (<=255, valid reciprocal members) for flag0200.
 * Group scalar projection/lifetimes stay live and immutable during numerical
 * evaluation; <=64 groups, no cycles. Filter fields are actual finite floats.
 * Captured cutoff sum*uint16parameter must fit signed32 truncation; source
 * then wraps low16. The finite resonance sum multiplies literal0, so actual
 *251D80 receives resonance0. No arbitrary resonance-policy approximation.
 *
 * Pure preflight admits all completing backend effects before any delivery.
 * Callbacks may change current device.buffer/stream and future track/instance
 * mix scalars, preserving captured device and all rings/owners/metadata. The
 * source stream is reloaded AFTER buffer delivery. No fabricated output or
 * hardware filter success; coefficients are sent to required actual backend.
 * Default FP mode; callback-visible FP exception status excluded. The cutoff
 * middle branch uses explicit host sin/log2 (same bounded mathematical boundary
 * as the original oracle); this does not claim NV2A/APU DSP equivalence.
 * status0complete,1pre-effect rejection. NULL track/absent voice/device are
 * genuine quiet branches, not a successful sound/device substitute. */
int uc_audio_live_gain(UCAudioInstance *,UCAudioInstanceTrack *,const UCAudioLiveMixServices *);
int uc_audio_live_pitch(UCAudioInstance *,UCAudioInstanceTrack *,const UCAudioLiveMixServices *);
int uc_audio_live_filter(UCAudioInstance *,UCAudioInstanceTrack *,const UCAudioLiveMixServices *);
/* Whole248D10: gain only, in actual current track-list order. Completing
 * delivery preserves topology; scalar changes affect later tracks. Full pure
 * preflight admits every later call; unexpected violation returns2 after any
 * committed earlier deliveries, never rollback. */
int uc_audio_live_all_gains(UCAudioInstance *,const UCAudioLiveMixServices *);
#ifdef __cplusplus
}
#endif
#endif
