#ifndef UC_MATERIAL_EMPTY_ORDINARY_H
#define UC_MATERIAL_EMPTY_ORDINARY_H
#include "material_alpha_prepass.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialEmptyContext {
    uint8_t override_enabled; /* original current context+4 */
    uint32_t lod_bits;        /* original current context+8, raw MOVSS bits */
} UCMaterialEmptyContext;
typedef struct UCMaterialEmptyState {
    const uint8_t *alpha_active; /* 51A014 */
    const uint32_t *bias_bits;   /* 4565E0 */
    uint32_t *dirty,*lod_bits;   /* 3F3D98 / 3F3DB8 */
} UCMaterialEmptyState;
typedef struct UCMaterialEmptyHooks {
    void (*transform)(void *); /* complete graphics+50 then result+30 */
    const UCMaterialEmptyContext *(*context)(void *); /* each call completes
        CURRENT graphics+A4 then returned owner+1C; nonnull live result */
} UCMaterialEmptyHooks;
/* Exact ordinary8FD80 for a recipe from uc_material_alpha_prepass_prepare:
 * flags21, setup0/pass count0, debug globals0. Same material's alpha+20 entry
 * remains a real draw. No texture lookup/bind or draw occurs in THIS entry.
 * Required transform runs first. Then capture0 if current alpha byte nonzero,
 * otherwise current global bias bits. First context's override byte controls
 * whether a SECOND context call supplies its current raw LOD bits. The second
 * override byte is ignored. Finally reload dirty|1, then publish LOD bits.
 * No float arithmetic/canonicalization; every32-bit LOD payload is admitted.
 * All borrowed fields/identities and recipe/hook storage stay disjoint/stable;
 * dirty and LOD cannot alias bias or each other. Completing non-reentrant hooks
 * may mutate state values and context owner/results without invalidating their
 * storage. Results need remain live only through the immediate field read.
 * Return0 before effects for admission failure;1 completed. A null context
 * result violates the completing service contract and returns0 after effects;
 * that case cannot be retried as if nothing happened. No allocation/GPU, parent
 * pass/state restoration, source loader or entire material-family claim. */
int uc_material_empty_ordinary(const UCMaterialAlphaPrepass *,UCMaterialEmptyState,
    const UCMaterialEmptyHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
