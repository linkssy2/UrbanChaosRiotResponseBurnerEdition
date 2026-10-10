#ifndef UC_SHOT_LIGHTFLASH_H
#define UC_SHOT_LIGHTFLASH_H
#include "light_flash.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCShotLightFlashRequest {
    UCLightFlashParameters parameters;
    float position[3],rotation[3];
    uint32_t parent_id,mode;
} UCShotLightFlashRequest;
typedef struct UCShotLightFlashPlacement {
    float transform[16];
    uint32_t flags,source_index;
} UCShotLightFlashPlacement;
typedef struct UCShotLightFlashServices {
    void *context;
    int (*preflight)(void *,uint32_t item,const uint32_t *entity_id);
    /* Complete synchronous LightFlash creation (20A230/207CD0/1FBC50),
     * including actual generic entity registration/base and coroutine lifetime.
     * Request is borrowed only during this call. Resolve the CURRENT parent at
     * the base20A850 point, then use uc_shot_lightflash_place and the existing
     * uc_light_flash_initialize. Do not substitute a muzzle/camera transform.
     * Null entity factory is not admitted; null point-light creation is.
     * The existing LightFlash remove_entity callback must not free its owner. */
    void (*create)(void *,const UCShotLightFlashRequest *);
} UCShotLightFlashServices;
/* CAB20's final ordinary2/17 recipe, reached after all prior shot effects even
 * when pellet count is zero. Earlier CAB20 work is NOT implemented here.
 * entity_id borrows actual Mainplayer+54, read after pure preflight. Completing
 * create may mutate game state/consume the existing shared RNG; this leaf has
 * no RNG or allocator. Temporary property/string storage is typed stack data.
 * Successful allocation/property semantics only, no original exceptions.
 * Return0 success,1 invalid/unadmitted before effects. No aliasing of borrowed
 * inputs/request/services; no reentry or callback changes to FP environment. */
int uc_shot_lightflash_emit(uint32_t item,const uint32_t *entity_id,
    const UCShotLightFlashServices *);
/* Full20A850 mode2 specialized to this recipe's +0 Euler angles and offset.
 * Parent is the live entity matrix resolved by207D70 at that source frontier,
 * not an earlier snapshot. Finite components <=1e6, RN and DAZ/FTZ off.
 * Exact three rotation compositions are retained, including zero products.
 * Output unchanged on invalid input; disjoint parent/output storage. */
int uc_shot_lightflash_place(const float parent[16],UCShotLightFlashPlacement *);
/* Same whole20A850 mode2 with the actual descriptor position and +0 Euler.
 * NPC D9670 uses source globals rather than CAB20's fixed player offset. */
int uc_shot_lightflash_place_offset(const float parent[16],const float offset[3],UCShotLightFlashPlacement *);
#ifdef __cplusplus
}
#endif
#endif
