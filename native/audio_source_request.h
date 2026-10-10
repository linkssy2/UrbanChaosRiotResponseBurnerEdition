#ifndef UC_NATIVE_AUDIO_SOURCE_REQUEST_H
#define UC_NATIVE_AUDIO_SOURCE_REQUEST_H
#include "audio_source_engine.h"
#include "audio_instance_prepare.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioSourceRequestServices {
 void *context;
 int (*preflight)(void *,const UCAudioSourceEngine *,const UCAudioInstanceBank *,uint32_t); /*pure0admit*/
 int (*select)(void *,const UCAudioInstanceBank *,uint32_t,uint32_t *,UCAudioRequestSelection *,uint8_t *); /*24C650*/
 const UCAudioSourceEngineServices *source;
 const float *(*listener)(void *); /*actual241880→interface3→+28 current position*/
 UCAudioInstance *(*instance)(void *,void *group); /*245B80 real allocation+group retain; NULL allowed*/
 int (*bind)(void *,UCAudioInstance *,UCAudioBuffer *); /*2489F0 completing same graph*/
 int (*prepare)(void *,UCAudioInstance *,const UCAudioInstanceBank *,uint32_t,uint32_t *,uint8_t *); /*249280*/
 int (*publish)(void *,const UCAudioInstanceBank *,UCAudioInstance *,uint32_t,uint8_t *); /*24C800*/
 int (*failed)(void *,UCAudioInstance *); /*24A500 real stop/release after failed prepare/start*/
} UCAudioSourceRequestServices;
/* Whole246BB0→2468E0, ordinary AMPC, nonpersistent sounds (sound+20 bit20
 * clear when selected buffer is spatial). Reuses the one concrete source
 * buffer ring and typed actual instance, not another voice graph. Bank resource
 * identity/interface/type projections are admitted pure and same identity104.
 * Optional out is setNULL before actual lookup; successful returned instance
 * is retained with real248730 then published, as original (outNULL no retain).
 * Missing bank/index/group/config/instance, selection rejection or out-of-range
 * one-shot distance is a completed quiet result, not backend success.
 * Spatial config uses the actual amp3 stride40; strict distance is computed
 * (dz*dz+dx*dx)+dy*dy AFTER current listener callback. Sound+9 permits distant
 * start. No persistent pending entry is invented or silently omitted.
 *
 * Preflight admits every possible selected sound/spatial record and completing
 * service before publication. Metadata is immutable thereafter; select may
 * mutate its own event selection/state/RNG exactly as its qualified contract.
 * Source factory/property hooks retain their contract; listener may change
 * source.position (read afterwards). Other hooks can change their actual
 * instance/reference state and request flags, but retain captured source,
 * buffer, bank, sound, listener and service lifetimes; failed may destroy its
 * instance because no later access follows. No reentry or shadow clocks/RNG.
 * Callback-visible FP exception flags excluded; default RN/DAZ/FTZ required.
 *
 * Services must complete actual functions, using existing qualified owners
 * where available. This controller does not fabricate factories, preparation,
 * playback, spatial mixing or cleanup success. 0source-completed,1pre-effect
 * rejection,2committed unexpected dependency/domain failure; no rollback of
 * source buffers, refs, flags or already-cleared output on committed failure. */
int uc_audio_source_request(UCAudioSourceEngine *,const UCAudioInstanceBank *,uint32_t,
 const UCAudioSourceRequestServices *,UCAudioInstance **optional_out);
#ifdef __cplusplus
}
#endif
#endif
