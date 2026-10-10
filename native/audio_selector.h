#ifndef UC_NATIVE_AUDIO_SELECTOR_H
#define UC_NATIVE_AUDIO_SELECTOR_H
#include "audio_asset.h"
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioSelection {uint32_t index;uint16_t reference,subsound;} UCAudioSelection;
/* Actual24BE60 mode5, with bank selector-observer+ A4 absent. One mutable ampw
 * payload: packed header then1..32767 twelve-byte rows. The high15 bits store
 * the persistent NEXT index. Return the current row, advance modulo count,
 * preserve header low17 bits; a singleton does not change the header. No RNG.
 * Non-singleton next index must be<count. Other modes, callbacks and resource
 * resolution are not invented here. Rows remain unchanged. Input and output
 * must be disjoint, input readable/writable and stable for the call. Failure
 * precedes writes and preserves output. */
UCAudioStatus uc_audio_selector_cycle(void *payload,size_t size,UCAudioSelection *);
/*24BE60 mode3: random selection excluding the previous index; shared AMPC
 * RNG. Singleton consumes no RNG. No observer/provider callbacks. This is
 * event flag100; GenericExplosions' Explode uses weighted selection instead. */
UCAudioStatus uc_audio_selector_random(void *payload,size_t size,UCAudioRandom *,UCAudioSelection *);
#ifdef __cplusplus
}
#endif
#endif
