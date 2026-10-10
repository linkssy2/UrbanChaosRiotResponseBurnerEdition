#ifndef UC_NATIVE_AUDIO_IMPACT_REQUEST_H
#define UC_NATIVE_AUDIO_IMPACT_REQUEST_H
#include "audio_default_policy.h"
#include "audio_event_selection.h"
#include "audio_request.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioSelectorPayload {void *data;size_t bytes;} UCAudioSelectorPayload;
/* Borrowed spans over the one actual bank owner, not copied events/selectors or
 * another voice graph. All banks used by current voices share event identities. */
typedef struct UCAudioImpactBank {
    UCAudioPolicyBank policy;
    uint8_t *events;uint32_t event_count;
    const uint8_t *sounds;uint32_t sound_count;
    UCAudioSelectorPayload *selectors;uint32_t selector_count;
    const void *const *groups;uint32_t group_count;
} UCAudioImpactBank;
typedef struct UCAudioImpactVoices {
    const UCAudioPolicyInstance *instances;uint32_t count;
    const uint32_t *const *flags;
    int (*stop)(void *,uint32_t index);void *context;
} UCAudioImpactVoices;
/* Full24C650 admitted bank observerNULL, cached shared groups, concrete event
 * selection plus event80/default named policies. event_indexFFFFFFFF is a real
 * missing event. SoundFFFF preserves prior selection; policy rejection DOES
 * publish the selected sound first and preserves prior group. Exact source
 * reload after a completing stop permits changing selection.sound/group/flags;
 * replacement sound must remain a valid record in this same live bank.
 * Stop must complete real24A900(0,0,2), or return failure; no silent playback.
 * Bank storage/RNG/output stable and appropriately disjoint. Stop can change the
 * current voice graph after policy's last read, but not bank storage/lifetimes.
 * 0 source-completed, found is24C650 AL (not a started voice);1 rejected before
 * selection effects;2 failure after selection succeeded: retain event/RNG and
 * any sound/stop publications, found unchanged. Valid bank/voice admission must
 * guarantee selected rows, policy and groups; there is no rollback on corruption.
 * Limits:65535 events/sounds/selectors,65536 cached groups,4096 current voices.
 * Uncached group creation, observers and other arbitration remain unsupported. */
int uc_audio_impact_request(const UCAudioImpactBank *,uint32_t event_index,
    uint32_t *flags,UCAudioRandom *,const UCAudioImpactVoices *,
    UCAudioRequestSelection *,uint8_t *found);
#ifdef __cplusplus
}
#endif
#endif
