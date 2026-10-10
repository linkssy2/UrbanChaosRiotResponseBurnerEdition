#ifndef UC_NATIVE_AUDIO_DEFAULT_POLICY_H
#define UC_NATIVE_AUDIO_DEFAULT_POLICY_H
#include "audio_impact_policy.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual240080 Default event-byte0/priority255, named limit1..3, no source
 * scoping. Ordered current instance projections alias the one actual voice
 * graph; flags[i] points at that instance's actual current flags64. All getters
 * and metadata are stable/pure through the decision. Unmatched flags mayNULL.
 * Count>limit SAME-event matches enters the original third pass, selecting the
 * FIRST policy-name match of ANY event. This bounded profile admits victim
 * priority255 and event bit4 clear; it calls actual24A900(victim,0,0,2) unless
 * current victim4000 or request8000 suppresses it. stop is a REQUIRED completing
 * endpoint, never a playback-success stand-in. It may destroy/change graph
 * state since there are no further graph reads afterward; no reentry.
 * 0 complete (accepted1);1 pre-effect unsupported/invalid;2 committed stop
 * service failure (accepted preserved). Validity and lifetime of all borrowed
 * views, event identities, flags and names is the caller's admission contract.
 * Same4096-instance/16MiB string bounds as the event80 owner. No voice allocation,
 * backend stopping, RNG, or group state is invented by this policy controller. */
int uc_audio_default_policy(const UCAudioPolicyBank *,const uint8_t *event,
    const uint8_t *sound,const UCAudioPolicyInstance *,uint32_t count,
    const uint32_t *const *flags,uint32_t request_flags,
    int (*stop)(void *,uint32_t index),void *context,uint8_t *accepted);
#ifdef __cplusplus
}
#endif
#endif
