#ifndef UC_AUDIO_SOURCE_BUSY_H
#define UC_AUDIO_SOURCE_BUSY_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Full2160A0/2167C0 predicate over actual245E00 enumeration order: pending
 * then buffer-owned instances. Caller supplies same retained instance fields,
 * including not-yet-playing and fading instances. Does not query PCM status.
 * Null helper/engine => no views. Views remain stable, at most4096. */
typedef struct UCAudioBusyInstance {uint32_t flags;int32_t phase;} UCAudioBusyInstance;
int uc_audio_source_busy(const UCAudioBusyInstance*,size_t,uint8_t*);
#ifdef __cplusplus
}
#endif
#endif
