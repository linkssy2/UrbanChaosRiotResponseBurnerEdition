#ifndef UC_NATIVE_AUDIO_EVENT_SELECTION_H
#define UC_NATIVE_AUDIO_EVENT_SELECTION_H
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual24C160 with bank observer+A0/A4 absent. Borrow the same mutable16-byte
 * event and resolved ampw payload used by the bank; selector_index must equal
 * event+8. A direct event (FFFFFFFF) touches neither selector nor RNG. Request
 * bit10 reads only the cached high15 header bits. Otherwise flags100 select
 * nonrepeat random before200 shuffle before400 cycle, else weighted.
 * Weighted publishes selected reference into header high15, then event+2;
 * shuffle/cycle retain their actual helper mutations. Reuse the SAME audio RNG.
 * Storage is valid, stable and disjoint; rejection precedes effects. No pointer
 * wrap, observers, bank allocation, policy, voice ownership or playback claim. */
UCAudioStatus uc_audio_event_select(uint8_t event[16],uint32_t request_flags,
    uint32_t selector_index,void *payload,size_t size,UCAudioRandom *random);
#ifdef __cplusplus
}
#endif
#endif
