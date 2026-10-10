#ifndef UC_AUDIO_ZERO_POLICY_H
#define UC_AUDIO_ZERO_POLICY_H
#include "audio_impact_policy.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual240080 named limit0, request0/80/84/86 and arbitrary priority. Source match values
 * project actual245D90 on each same live instance, not guessed positions.
 * flags pointers alias current instance64; request_flags is the caller's actual
 * word and may be changed after fade. Required fade is248E80(instance,4000),
 * required stop is24A900(instance,0,0,2). They may release victim metadata after
 * this decision's final graph read. Callback failure is committed, no retry.
 * Event80 same-event duplicates reject before effects. Existing eventbit4 instances
 * fade; requestbit2 crossfades with2000, otherwise8000 suppresses new playback.
 * Typed stable ordered views admit4096 instances, pure getters, equal-priority
 * victims or source-exact lower-priority refusal. A higher-priority request
 * requiring23FDF0 policy-wide purge remains an explicit unconnected boundary;
 * other policies reject explicitly. Output accepted changes only on completion. */
int uc_audio_zero_policy(const UCAudioPolicyBank*,const uint8_t*event,const uint8_t*sound,
 const UCAudioPolicyInstance*,uint32_t count,unsigned has_source,const uint8_t*source_matches,
 const uint32_t*const*flags,uint32_t*request_flags,
 int(*fade)(void*,uint32_t),int(*stop)(void*,uint32_t),void*context,uint8_t*accepted);
#ifdef __cplusplus
}
#endif
#endif
