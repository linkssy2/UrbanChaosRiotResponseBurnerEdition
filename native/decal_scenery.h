#ifndef UC_DECAL_SCENERY_H
#define UC_DECAL_SCENERY_H
#include "collision_scenery_trace.h"
#include "decal_submit.h"
#include "decal_ring.h"
#include "material_resources.h"
#include "scene_ambient_query.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Stable borrowed projection of the current scenery-query index, mark manager,
 * shared cache and ambient manager. No parallel world/resource/light registry.
 * Resource archive remains open for cold material lookups. Ring may be NULL,
 * matching actual absent-manager rejection after all four successful rays.
 * Registered physics world, provider selection, renderable lifetime and GPU
 * scheduling are upstream; this owner supplies no dynamic-body collision. */
typedef struct UCDecalScenery {
    UCCollisionSceneryTrace trace;
    UCDecalRing *ring;
    UCMaterialResources *resources;
    const uint32_t *ambient;
    const UCSceneAmbientLight *lights;
    uint32_t light_count;
} UCDecalScenery;
/* Shared source223BC0 four-ray validation ->9BBB0 insertion ->22B280 material
 * lookup ->A4890 color query. Same random, cache, counters and ring owners.
 * Return0 complete/miss;1 static rejection;2 committed failure, never retry
 * unchanged. submitted retains the lower source publication-attempt meaning;
 * successful visible rendering is not implied. All lower finite/RN bounds,
 * stable disjoint ownership and completing allocation requirements apply. */
int uc_decal_scenery_submit(const UCDecalScenery *,const char *shader,
    const float point[3],const float normal[3],float size,UCVectorRandom *,uint8_t *submitted);
#ifdef __cplusplus
}
#endif
#endif
