#ifndef UC_NATIVE_AUDIO_VOICE_RETIREMENT_H
#define UC_NATIVE_AUDIO_VOICE_RETIREMENT_H
#include "audio_device_playback.h"
#include "audio_buffer_factory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioVoiceRetirement {
 void *context;
 UCAudioLink *pending; /* SAME engine+30 pending buffer ring */
 uint32_t *available[3]; /* SAME engine+78/+7C/+80 device budgets */
 int (*preflight)(void *,const UCAudioBuffer *); /*pure0admit*/
 void (*stop)(void *,void *); /*424FC5*/
 void (*status)(void *,void *,uint32_t *); /*425001*/
 void (*output_null)(void *,void *); /*425F23(NULL)*/
 void (*stop_extended)(void *,void *); /*424FDD(0,0,0)*/
 void (*release_backend)(void *,void *); /*423B6D; real owned backend destruction*/
 void (*release_device)(void *,UCAudioDeviceBuffer *); /*actual native allocator*/
 void (*release_voice)(void *,UCAudioBuffer *); /*actual native allocator*/
} UCAudioVoiceRetirement;
/* Full246230 and ordinary deleting246150/247030 +23FA30(type8),2516C0,
 * 252E00/252F30; same graph and budgets. This fresh resident-voice domain has
 * owner34/owner38/spatialNULL, no voice children/instances, no device children,
 * no stream/helper. Device.spatial is only borrowed, never freed here.
 * Nonempty pending ring is allowed. All live links must be reciprocal.
 * Backend callbacks complete their effects, preserve every owner scalar/ring
 * and lifetime until explicit release, and may mutate external state. Final
 * release callbacks may immediately free their argument; no later reads occur.
 * Device allocator service replaces the original pool reuse with the admitted
 * native allocator; unlinking actual device membership precedes that service.
 * Preflight admits all effects and status convergence within4096 polls. An
 * exhausted poll budget returns2 after effects, preserving the unfreed owner;
 * no retry/rollback or successful destruction is implied.
 * remaining is separate live storage, written on0 only. Returns0 complete,
 * 1 pure rejection,2 committed bounded-domain failure. A retained count1 may
 * be queued in pending exactly as source; this is NOT its future drain owner.
 * No group/source-buffer destruction or arbitrary spatial allocation assumed. */
int uc_audio_voice_release(UCAudioBuffer *,const UCAudioVoiceRetirement *,uint32_t *remaining);
/* Same246230/246150/device destructor for a QUIESCENT source buffer. flags8
 * clear, owner38/spatialNULL, empty children/instances/aux and detached source
 * link3. owner34 is NULL or group.identity; its borrowed references>=2 so the
 * actual2471A0 decrement cannot delete the externally retained group. Buffer
 * references>=1; a retained result is live and is NOT falsely disposed. Device
 * and backend contracts above apply, including real bit10 no-poll destruction.
 * This does not stop active instances. All ownership/links are checked before
 * effects; group storage and backend callbacks preserve each other's fields.
 * Final release_voice may immediately free; remaining must not alias owners. */
int uc_audio_source_buffer_release(UCAudioBuffer *,const UCAudioBufferGroup *,
                                  const UCAudioVoiceRetirement *,uint32_t *remaining);
#ifdef __cplusplus
}
#endif
#endif
