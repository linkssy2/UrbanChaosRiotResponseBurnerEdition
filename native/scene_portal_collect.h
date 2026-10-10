#ifndef UC_SCENE_PORTAL_COLLECT_H
#define UC_SCENE_PORTAL_COLLECT_H
#include "scene_geometry_collect.h"
#include "scene_matrix_provider.h"
#include "scene_portal_visibility.h"
#include "scene_portal_move.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalOwner {
    UCScenePortalGraph *graph;
    UCScenePortalStart *start;
    UCScenePortalPlanes *planes;
    UCScenePortalVisibility *visible;
    uint32_t active; /* original +4, distinct from graph storage lifetime */
    uint8_t freeze,failed;
    uint8_t *movement_ambiguity; /* SAME engine51B420, borrowed across views. */
} UCScenePortalOwner;
typedef struct UCScenePortalCollectHooks {
    UCSceneGeometryHooks geometry;
    uint8_t (*occlusion)(void *,const UCSceneGeometryConfig *);
    UCSceneMatrixServices matrices;
} UCScenePortalCollectHooks;
/* Nonnull-occluder AB9E0 -> actual matrix provider -> AB7F0 -> successive AB2E0/
 * AB1E0 -> A9310 -> AB8B0. geometry.occluder is this exact borrowed owner;
 * root_count aliases geometry+14 and may be smaller than sector count (the
 * exterior sectors are admitted without a room query). Background stays NULL.
 * camera returns the actual UCSceneMatrixProvider from graphics+50, captured
 * before the first append. Config is reloaded at the original frontiers.
 * Matrix uploads/context and sector/model callbacks are completing services.
 * Model callback must use the actual BA400 owner with the same live matrices,
 * mode, collector and manager. No fabricated collection/membership success.
 * Stable graph/provider/owner, capacity-admitted completing services and no
 * reentry. Other geometry counts/storage/enabled may change within the existing
 * sector contract. Root count stays within graph room count. A running portal
 * update uses the same start-list and shared movement ambiguity byte for
 * first and subsequent camera positions. Inactive/frozen gates preserve them.
 * Return0 complete,1 initial rejection without effects,2 failure after effects;
 * status2 poisons this owner until its caller destroys/reinitializes it. Partial
 * collection/state is retained, and no retry or successful frame is implied.
 * No camera/view construction, background, sorting, palette or GPU scheduling. */
int uc_scene_portal_collect(UCSceneGeometry *,const int32_t *root_count,
    uint8_t *group_mode,void *collector,void *manager,
    const UCScenePortalCollectHooks *,void *owner,uint32_t budget);
#ifdef __cplusplus
}
#endif
#endif
