#ifndef UC_NATIVE_AUDIO_DEVICE_PLAYBACK_H
#define UC_NATIVE_AUDIO_DEVICE_PLAYBACK_H
#include "audio_device_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioPlaybackServices {
 void *context;
 int (*preflight)(void *,const UCAudioDeviceBuffer *,unsigned operation,uint32_t repeat); /*pure0admit*/
 void (*play)(void *,void *backend,uint32_t repeat); /*424FA1(backend,0,0,repeat)*/
 void (*stop)(void *,void *backend); /*424FC5*/
 void (*status)(void *,void *backend,uint32_t *out); /*425001*/
} UCAudioPlaybackServices;
/* Actual245430→251710,245450→252B60,251770 and2519C0 on the SAME ordinary device wrapper.
 * stream/helper must remainNULL. Play ORs the low repeat bit into flags even
 * with no backend or unready format/data; it starts only with flags03000000.
 * It does not clear an old repeat bit. There is no synthetic successful start.
 * Backend calls must complete actual effects; may change current scalar fields
 * while preserving captured wrappers/lifetimes and absent stream/helper fields.
 * preflight is pure; operations0/1/2 mean play/stop/status. It admits the actual
 * backend operation, including loop support if requested. Source status bits
 * are backend output, not inferred by this leaf. Status output is distinct
 * storage; NULL follows the original quiet path. No refcount/free/arbitration.
 * Returns0 source-completed,1 rejected before effects. Completion with no
 * backend is a real source quiet branch, never evidence of played audio. */
int uc_audio_voice_start(UCAudioBuffer *,const UCAudioPlaybackServices *);
int uc_audio_voice_stop(UCAudioBuffer *,uint32_t flags,const UCAudioPlaybackServices *);
int uc_audio_device_stop(UCAudioDeviceBuffer *,const UCAudioPlaybackServices *);
int uc_audio_device_status(UCAudioDeviceBuffer *,uint32_t *,const UCAudioPlaybackServices *);
#ifdef __cplusplus
}
#endif
#endif
