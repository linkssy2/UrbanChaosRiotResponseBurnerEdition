#ifndef UC_NATIVE_AUDIO_IMPACT_POLICY_H
#define UC_NATIVE_AUDIO_IMPACT_POLICY_H
#include "audio_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed, immutable projection of actual current bank/instance getters.
 * This does not own/copy a live voice graph. strings is the original ampz
 * payload (bank+8C+8), type is the current bank's type identifier. */
typedef struct UCAudioPolicyBank {
    const uint8_t *strings;
    size_t string_bytes;
    uint32_t type;
} UCAudioPolicyBank;
typedef struct UCAudioPolicyInstance {
    uint32_t resource_type; /* instance queried resource +20 */
    const UCAudioPolicyBank *bank; /* current instance+3C */
    const uint8_t *sound; /* current instance+50, original28-byte record */
    const uint8_t *event; /* actual24B600 result; POINTER identity matters */
} UCAudioPolicyInstance;
/* Full240080's nonzero named-concurrency/event80 domain, used by authored
 * HardImpacts/SoftImpacts/FleshImpacts. sound+0E high2 bits must1..3 and the
 * low14 bits must resolve a terminated policy name. sound+15 bit4 is excluded
 * (source-scoped matching); event byte0 bit80 is required. Original first
 * pass rejects the(limit+1)th matching policy-name AND event identity; if it
 * completes, the second pass cannot reach stealing/stop/flag effects.
 * Original pure getters are projected into ordered live instance views.
 * All views, pointer identities, records and names remain stable throughout
 * this call; no concurrent engine update or callback-visible query effects.
 * Instances0..4096; bank string spans1..16MiB. Type104 is actual retail0x68.
 * Output changes only on completion. No allocation, RNG, flags, playback or
 * missing-voice suppression. Other arbitration domains reject explicitly. */
UCAudioStatus uc_audio_impact_policy(const UCAudioPolicyBank *,const uint8_t *event,
    const uint8_t *sound,const UCAudioPolicyInstance *,uint32_t count,uint8_t *accepted);
#ifdef __cplusplus
}
#endif
#endif
