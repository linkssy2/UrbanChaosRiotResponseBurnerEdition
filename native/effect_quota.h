#ifndef UC_EFFECT_QUOTA_H
#define UC_EFFECT_QUOTA_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEffectQuotaEntry {float point[3],radius_squared,remaining;} UCEffectQuotaEntry;
typedef struct UCEffectQuota {UCEffectQuotaEntry entries[64];uint32_t count;} UCEffectQuota;
/* Actual493C0 represented fields: only count is initialized. Caller owns this
 * one persistent manager; no duplicated quota per impact and no stale clearing. */
int uc_effect_quota_init(UCEffectQuota *);
/* Full49250/492C0/49340. Finite RN/gradual-underflow, count<=64, disjoint
 * parameters/results/owner and valid live entries. Full add completes with
 * added=0; distance exactly equal to radius_squared does NOT block. Tick uses
 * source swap-last/retest order and retains all stale tail words.
 * All functions return0 complete,1 pre-effect rejection; outputs unchanged on1.
 * No callback/heap/per-impact allocation. This is the quota state, not world
 * construction or its frame scheduling. delta may be negative if finite. */
int uc_effect_quota_add(UCEffectQuota *,const float point[3],float radius,float duration,uint8_t *added);
int uc_effect_quota_test(const UCEffectQuota *,const float point[3],uint8_t *blocked);
int uc_effect_quota_tick(UCEffectQuota *,float delta);
#ifdef __cplusplus
}
#endif
#endif
