#ifndef UC_GLASS_CONTAINER_H
#define UC_GLASS_CONTAINER_H
#include "glass_pane.h"
#include "glass_ground.h"
#include "glass_registry.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassContainer {
    UCGlassRegistry *registry;
    UCGlassStore *air;
    UCGlassPane *pane;
    UCGlassGroundStore *ground;
    UCVectorRandom *random;
} UCGlassContainer;
typedef struct UCGlassContainerInput {
    UCGlassRegistry *registry;
    UCGlassPool *air_pool;
    UCGlassGroundPool *ground_pool;
    UCVectorRandom *vectors;
    UCGameRandom *random;
    UCGlassPaneUV *uv;
    const float *world;
    const float (*quad)[2];
    float floor;
    const char *pane_shader,*shard_shader;
} UCGlassContainerInput;
/* Actual51830, with existing concrete ground/air/pane constructors and SAME
 * registry. Floor is rounded floor+2; ground and air share it. Pane owns air,
 * which owns ground: the outer's air/ground fields are borrowed aliases.
 * Three current-resource reads precede ground,air,pane registration in order.
 * Getter represents22B210->2E0E00, may change external provider state, and must
 * return the real live cache owner. All inputs/storage remain stable; no reentry
 * or callback mutation of this graph. Native source identity is each actual
 * owner pointer, replacing the original adjusted interface address bijectively.
 * Registry capacity/uniqueness and resource admission must cover all three.
 * Output publishes before the first getter, so its actual fields are observable.
 * Return0 complete;1 rejected before effects;2 allocation/committed failure.
 * On2, nonnull *out owns the real partial registration and MUST be destroyed;
 * never retry construction. Cache/UV effects are not rolled back. On1 output
 * is unchanged. Shared pools/RNG/UV and registry remain borrowed throughout.
 * This does not establish pane material provenance or global group attachment. */
int uc_glass_container_create(const UCGlassContainerInput *,UCMaterialResources *(*current)(void *),void *,UCGlassContainer **out);
/* Actual51A50: any nonzero pane trigger returns2, otherwise0. */
uint32_t uc_glass_container_state(const UCGlassContainer *);
/* Actual51A70: +/-zero is quiet; otherwise air updates BEFORE ground, sharing
 * the SAME vector stream. Finite delta[0,1] admitted, so original NaN/negative
 * caller paths are excluded.0complete,1before effects,2partial update failure. */
int uc_glass_container_update(UCGlassContainer *,float delta);
/* Actual51AD0/51AB0: remove ground,air,pane wrappers, then pane's owned cleanup,
 * then free outer. Exclusive valid graph; all other borrows must be detached.
 *0complete,1invalid before effects,2unexpected partial failure: never retry. */
int uc_glass_container_destroy(UCGlassContainer *);
#ifdef __cplusplus
}
#endif
#endif
