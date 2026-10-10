#ifndef UC_NATIVE_AUDIO_SELECTOR_SHUFFLE_H
#define UC_NATIVE_AUDIO_SELECTOR_SHUFFLE_H
#include "audio_selector.h"
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full24BE60 mode4, observer+A4 absent. Persistent ampw payload, count1..32767.
 * For count>1 at least one row's uint32 marker(+4) must equalFFFFFFFF on entry;
 * the original otherwise retries forever. Other marker values mean used.
 * Uses the SAME audio RNG as pitch/volume, including every retry. After the
 * final available row is chosen, reset other rows but retain its used marker.
 * Singleton returns row0 without changing markers or RNG. Header/other fields
 * are preserved. All storage is distinct/stable; failures preserve everything.
 * Selection does not imply GUID residency, playback or wider bank ownership. */
UCAudioStatus uc_audio_selector_shuffle(void *,size_t,UCAudioRandom *,UCAudioSelection *);
#ifdef __cplusplus
}
#endif
#endif
