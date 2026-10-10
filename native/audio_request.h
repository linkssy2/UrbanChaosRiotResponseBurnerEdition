#ifndef UC_NATIVE_AUDIO_REQUEST_H
#define UC_NATIVE_AUDIO_REQUEST_H
#include "audio_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioRequestSelection {
    const uint8_t *sound; /* Borrowed original28-byte sound metadata. */
    const void *group; /* Actual shared group identity, possibly null. */
} UCAudioRequestSelection;
typedef struct UCAudioRequestServices {
    void *context;
    const uint8_t *(*sound)(void *,uint16_t index);
    const void *(*group)(void *,uint16_t index);
} UCAudioRequestServices;
/* 24C650 +24C160 direct-event branch: event is an immutable16-byte record,
 * orNULL for missing indexFFFFFFFF. The caller owns correct name/index lookup.
 * No event selector, bank observer, or sound policy program (+0E!=FFFF).
 * Sound/group accessors are pure, return actual stable bank-owned metadata and
 * cached shared-group references, and admit every requested index. A null sound
 * is invalid; a null group is a real result for a later controller fallback.
 * Existing nonnull selection.group is preserved without a group lookup.
 * Missing event or soundFFFF preserves selection and returns found0.
 * Failure preserves selection/found. Other sound flags remain authored data;
 * this only prepares a request, not admission of playback/spatial/arbitration.
 * Input flags are unchanged in this source subset, so no flags slot is owned.
 * All inputs/outputs/accessor storage are stable and disjoint as appropriate.
 */
UCAudioStatus uc_audio_request_prepare(const uint8_t *event,
    const UCAudioRequestServices *,UCAudioRequestSelection *,uint8_t *found);
#ifdef __cplusplus
}
#endif
#endif
