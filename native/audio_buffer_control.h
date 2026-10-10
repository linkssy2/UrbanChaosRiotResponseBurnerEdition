#ifndef UC_NATIVE_AUDIO_BUFFER_CONTROL_H
#define UC_NATIVE_AUDIO_BUFFER_CONTROL_H
#include "audio_device_bus.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioBufferControl {
 void *context;
 int (*preflight)(void *,const UCAudioDeviceBuffer *,unsigned); /* pure0admits */
 /* Actual42501D captures up to8 source bin/volume pairs. Negative HRESULT is
  * a completed source query failure. Success writes count<=8 and all pairs. */
 int32_t (*bins)(void *,void *backend,UCAudioMixBin pairs[8],uint32_t *count);
 void (*volume)(void *,void *backend,uint32_t bin,int32_t volume); /*424F85*/
 void (*output)(void *,void *backend); /*425F23(_,NULL)*/
} UCAudioBufferControl;
/* Whole2523A0 ordinary buffer branch. Stores clamped signed16[-10000,-600],
 * but sends the ORIGINAL signed16 argument to the FIRST bin10 only. Backend
 * identity is reloaded after the real query. Missing backend/no bin10/query
 * failure are original quiet branches, not invented attenuation success.
 * Ordinary devices only (stream/helperNULL); source query count<=8. Completing
 * hooks may replace buffer with another admitted live backend or change gain,
 * preserving device/service/storage lifetimes. They cannot mutate captured
 * query output after return. No reentry. Preflight admits all prospective
 * current backends; unexpected oversized query returns2 after cached gain
 * publication. Other returns0complete/1pure rejection. No DSP equivalence. */
int uc_audio_buffer_attenuation(UCAudioDeviceBuffer *,int16_t,const UCAudioBufferControl *);
/* Whole245640(_,NULL,0)->251890(NULL): detach/reappend SAME wrapper to its
 * actual provider, then clear real backend output. No stop/filter/reset is
 * added. Voice deviceNULL is quiet. Valid reciprocal provider/device rings
 * required; callbacks may change other owners but preserve these rings and
 * device/provider lifetimes. No source/destination retain or finalfree here. */
int uc_audio_buffer_clear(UCAudioBuffer *,const UCAudioBufferControl *);
#ifdef __cplusplus
}
#endif
#endif
