#ifndef UC_HYBRID_CASTERS_GUEST_H
#define UC_HYBRID_CASTERS_GUEST_H
#include "hybrid_lighting_guest.h"
#include "../native/runtime/control_shadow_caster.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Source-admitted actor/light pairs, PC opaque surface silhouette replacement.
 * Does not use the camera receiver iterator, query visibility or stencil volume
 * packets. Original simulation, pose and caster registration stay in CONTROL. */
int uc_hybrid_casters_collect(const UcHybridLightingScene *,const UcHybridLightingRecord *,unsigned count);
int uc_hybrid_casters_host_submit(const UcHybridLightingScene *,const NativeShadowCaster *,unsigned count);
void uc_hybrid_casters_read_counts(uint64_t *,unsigned count);
#ifdef __cplusplus
}
#endif
#endif
