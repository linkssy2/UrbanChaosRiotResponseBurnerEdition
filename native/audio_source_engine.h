#ifndef UC_NATIVE_AUDIO_SOURCE_ENGINE_H
#define UC_NATIVE_AUDIO_SOURCE_ENGINE_H
#include "audio_voice_create.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2463E0 source behind the actor helper's engine pointer. Buffer members
 * are the SAME UCAudioBuffer.links[3] used by instance/voice owners. No private
 * playback graph. Pending is actual source+14; only its empty domain is admitted
 * by position/end here. Native storage replaces class words, not scalar state. */
typedef struct UCAudioSourceEngine {
 UCAudioLink membership,buffers,pending;
 uint32_t references,counter,kind;
 float minimum,maximum,position[3];
 void *group;uint32_t flags;uint8_t enabled;
} UCAudioSourceEngine;
typedef struct UCAudioSourceConfig {
 void *group;float range[2];uint32_t priority,flags,mode;uint8_t flag;
} UCAudioSourceConfig;
typedef struct UCAudioSourceEngineServices {
 void *context;
 int (*preflight)(void *,const UCAudioSourceEngine *,unsigned operation); /*pure0admit*/
 void *(*current)(void *); /* actual241880(0), not a cached lookup */
 void *(*listener)(void *,void *engine); /* interface3, NULL permitted */
 double (*scale)(void *,void *listener); /* listener+18 x87 return */
 UCAudioLink *(*registry)(void *,void *engine); /* pure actual engine+48 */
 int (*ranges)(void *,UCAudioBuffer *,float out[2]); /* buffer+4 virtual+8 property4 */
 uint32_t (*mode)(void *,const UCAudioBuffer *); /* pure spatial+10 or2 if absent */
 UCAudioBuffer *(*create)(void *,void *group,uint32_t flags,uint16_t channels,uint16_t priority);
 int (*parameter)(void *,UCAudioBuffer *,uint32_t property,const void *value);
} UCAudioSourceEngineServices;
/* Fresh unpublished storage. Whole2463E0+240850; current/listener/scale are
 * completing source callbacks, second current is reloaded before registration.
 * First current must be nonnull; listener scale is finite positive1e-20..1e20,
 * default FP mode. Caller guarantees every prospective live registry reciprocal.
 * Failure2 retains initialized fields/callback effects, never claims source NULL
 * dereference equivalence. Caller owns the source allocation and final release. */
int uc_audio_source_engine_begin(UCAudioSourceEngine *,const UCAudioSourceEngineServices *);
/* Whole246D90 including actual246C10 EMPTY-pending branch. Position copied
 * bitwise first. Spatial modes0/3/4 receive property5 using the original input
 * pointer; other modes and nonspatial buffers do not. Live bounded reciprocal
 * rings<=4096 are admitted. Callbacks preserve links/lifetimes but may mutate
 * current buffer fields; no reentry or pending insertion. No generic pending
 * sound update is silently completed. */
int uc_audio_source_engine_position(UCAudioSourceEngine *,const float[3],const UCAudioSourceEngineServices *);
/* Full245AD0 and246600. Config immutable/disjoint through callbacks. Matching
 * preserves first-list order, ordered float equality (+/-0 equal, NaN unequal),
 * source short-circuits and current spatial-mode read after ranges callback.
 * Creation requires actual group+24/type6 factory and property effects; NULL
 * factory result is a completed source result. Its fresh link must be isolated.
 * Source position is read at final property5 frontier, after earlier callbacks.
 * Returned wrapper joins source.buffers only AFTER all properties. Buffer's
 * owner34 is the actual shared group identity, not duplicated group authority. */
int uc_audio_source_engine_find(UCAudioSourceEngine *,const UCAudioSourceConfig *,const UCAudioSourceEngineServices *,UCAudioBuffer **);
int uc_audio_source_engine_create(UCAudioSourceEngine *,const UCAudioSourceConfig *,const UCAudioSourceEngineServices *,UCAudioBuffer **);
/* Actual2464D0 enable and2464F0 range publication. */
int uc_audio_source_engine_enable(UCAudioSourceEngine *);
int uc_audio_source_engine_range(UCAudioSourceEngine *,float,float);
/* Whole246510 when both owned rings are actually empty; unregister then final
 * isolated unlink. Caller must first complete real buffer/pending teardown.
 * Does not free caller storage, decrement references or invent stop success. */
int uc_audio_source_engine_end(UCAudioSourceEngine *,const UCAudioSourceEngineServices *);
/* All return0complete,1pre-effect rejection,2committed dependency failure.
 * Preflight is pure. Other hooks preserve source/input lifetimes and FP mode;
 * no callback-visible FP exception contract. Buffer flags/spatial/position may
 * change at the indicated frontiers. Allocation/resource/device playback and
 * complete spatial property ownership remain required real services. */
#ifdef __cplusplus
}
#endif
#endif
