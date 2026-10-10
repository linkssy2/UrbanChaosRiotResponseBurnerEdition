#ifndef UC_GLASS_COLLECT_H
#define UC_GLASS_COLLECT_H
#include "glass_registry.h"
#include "scene_matrix_provider.h"
#ifdef __cplusplus
extern "C" {
#endif
/* WholeAD5A0 with actual graphics89910 and provider456D88 getters: +4=ADE80
 * returns MATERIAL(+90), +0C=214E80 returns VIEW(+50). No projection is read.
 * current aliases the actual graphics+54 provider slot. Registry transform and
 * provider stay immutable/live through completion; no concurrent/reentrant
 * mutation. Concrete getters are pure, so both source current lookups resolve
 * this same owner. The depth is positive0 minus Z of translation*material*view,
 * with original pairwise float grouping. No enabled/empty/frustum gate is added.
 * Required append completes real collector publication of (group5,flags110,
 * depth,partition0); it may affect its collector, not mutate/free these borrows
 * during the call. No sorting/storage or outer view scheduling is invented.
 * Finite arithmetic, RN/gradual SSE; unused-row FP sticky effects are excluded.
 * Returns0 completed,1 pre-effect admission failure. */
int uc_glass_collect(const UCGlassRegistry *,const UCSceneMatrixProvider *const *current,
    void (*append)(void *,const UCGlassRegistry *,uint32_t group,uint32_t flags,float depth,uint32_t partition),void *);
#ifdef __cplusplus
}
#endif
#endif
