#ifndef UC_SCENE_GEOMETRY_COLLECT_H
#define UC_SCENE_GEOMETRY_COLLECT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneGeometrySector {
    void *identity;
    void **models;
    int32_t count;
} UCSceneGeometrySector;
typedef struct UCSceneGeometry {
    void *identity;
    UCSceneGeometrySector *sectors;
    int32_t count;
    void *background,*occluder;
    uint8_t enabled;
} UCSceneGeometry;
typedef struct UCSceneGeometryConfig { void *filter; } UCSceneGeometryConfig;
typedef struct UCSceneGeometryHooks {
    void *(*camera)(void *owner);
    const UCSceneGeometryConfig *(*config)(void *owner,void *collector);
    uint8_t (*filter_flags)(void *owner,void *filter);
    void (*append)(void *owner,void *collector,void *target,
                   uint32_t partition,uint32_t group,uint32_t flags,float depth);
    void (*model)(void *owner,void *target,void *collector,void *manager);
} UCSceneGeometryHooks;
/* Complete AB9E0/AB8B0 within the explicit absent background(+3C) and
 * absent occluder(+34) domain. These are borrowed CURRENT geometry+10/+18,
 * sector+8/+10 and global51B74C fields, not flattened record copies. At most
 * 4096 sectors and4096 models per sector; signed nonpositive counts are empty.
 * All storage/identities are accessible and live through the completing call.
 * Pure validation precedes effects. Camera is called BEFORE the geometry
 * record; config is reloaded a second time iff the first filter is nonnull.
 * Config getters return nonnull live objects, and the second filter must then
 * remain nonnull. Filter rejection preserves group
 * mode; otherwise mode=1 surrounds sector/model callbacks and mode=0 follows.
 * Geometry emits (partition0,group7,flags51,depth-1); each sector emits all0
 * before any model. Counts/storage/enabled are reloaded at source frontiers.
 * Hooks may change future fields/storage while preserving this bounded valid
 * domain, but must preserve current callback targets until it returns, optional
 * owners NULL and accessible remaining storage. No reentry of the same owner.
 * Model hook must invoke its actual +40 collector (BA400 for admitted static
 * models), with the SAME live mode and unchanged collector/manager arguments.
 * No constructor/tree insertion, camera/provider, optional occlusion/background,
 * collector allocation/sort, membership, palette or frame scheduling is implied.
 * Return0 completed,1 invalid initial state/arguments before effects. Completing
 * hooks cannot fail; their late domain violations are not retryable operations. */
int uc_scene_geometry_collect(UCSceneGeometry *,uint8_t *group_mode,void *collector,
    void *manager,const UCSceneGeometryHooks *,void *owner);
int uc_scene_sector_collect(UCSceneGeometrySector *,void *collector,void *manager,
    const UCSceneGeometryHooks *,void *owner);
#ifdef __cplusplus
}
#endif
#endif
