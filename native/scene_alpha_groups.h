#ifndef UC_SCENE_ALPHA_GROUPS_H
#define UC_SCENE_ALPHA_GROUPS_H
#include "scene_texture_pass.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the actual collector +10/+12/+14/+C and globals51A008/51A014.
 * Pointer addresses remain stable; completing pass callbacks may change their
 * current values. The owner is tested for presence, never dereferenced here. */
typedef struct UCSceneAlphaGates {
    const uint8_t *disabled,*passes,*ambient,*texture;
    const void *const *owner;
    uint8_t *active;
} UCSceneAlphaGates;
typedef struct UCSceneAlphaHooks {
    void (*ambient)(void *,UCSceneTextureCursor *,UCSceneModelPass *,uint32_t blend);
    void (*light)(void *,UCSceneTextureCursor *,UCSceneModelPass *,
        const void *light_set,uint32_t shadow_request);
    void (*texture)(void *,UCSceneTextureCursor *,UCSceneModelPass *);
} UCSceneAlphaHooks;
/* Original99240 block99544..9972F over already-collected, already-ordered records.
 * Disabled returns1 without inspecting other fields/arguments or changing active.
 * Otherwise sets active1, finds alpha100 records, and groups a bit200 first
 * record through the next alpha100 record without bit200 (or end). Intervening
 * non-alpha records belong to that physical range. Each range calls ambient
 * 101/101 with blend1, light108/108 with shadow0, then texture110/110 according
 * to CURRENT gates. Calls still occur for initially-ended subcursors. Subcursor
 * records/count describe only the group; current is relative to that range.
 * One shared pass and nonnull light-set identity are forwarded by this owner.
 * Each callback may mutate its own cursor/pass and live gate values; cannot
 * mutate records, active, borrowed pointer addresses, hooks or retained set,
 * retain the transient cursor, reenter, fail, or invalidate later services.
 * Caller preadmits every possibly-selected downstream pass and all resources.
 * Required completing callbacks perform the actual pass; no draw implementation
 * or success substitute is supplied. Stable nonoverlapping borrowed owners,
 * count<=65536, no allocation. Model pointers/other record fields are opaque.
 * Returns0 before effects on invalid admission,1 complete; enabled completion
 * clears active even for an empty/no-alpha list. No sorting, opaque passes,
 * light-set construction, parent state restoration or whole-frame claim. */
int uc_scene_alpha_groups(const UCSceneModelRecord *,uint32_t count,
    UCSceneModelPass *,const void *light_set,const UCSceneAlphaGates *,
    const UCSceneAlphaHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
