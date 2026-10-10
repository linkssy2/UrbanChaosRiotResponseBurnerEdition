#ifndef UC_NATIVE_AUDIO_SELECTOR_WEIGHTED_H
#define UC_NATIVE_AUDIO_SELECTOR_WEIGHTED_H
#include "audio_selector.h"
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full24BE60 mode0 with no observer. Immutable packed ampw table,1..32767
 * twelve-byte rows. For count>1 draw exactly once from the shared audio LCG,
 * mask to15bits, select the FIRST inclusive uint16 interval at row+8/+10.
 * No match selects row0; overlapping, reversed and out-of-range intervals are
 * preserved rather than repaired. Count1 uses row0 without consuming RNG.
 * Table/outputs/random are valid, stable and disjoint. Rejection preserves all. */
UCAudioStatus uc_audio_selector_weighted(const void *,size_t,UCAudioRandom *,UCAudioSelection *);
#ifdef __cplusplus
}
#endif
#endif
