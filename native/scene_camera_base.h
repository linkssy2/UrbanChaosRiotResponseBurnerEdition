#ifndef UC_SCENE_CAMERA_BASE_H
#define UC_SCENE_CAMERA_BASE_H
#include "entity_fresh.h"
#include "scene_view.h"
#include "bullet_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual Camera base suffix21E470: same mode1 actor, same projection at+D0,
 * same embedded motion at+150. No second entity/transform/registry is owned.
 * Native motion's actor backpointer is the borrowed fresh view, as in Bullet.
 * Projection FOV/alternateFOV/cache and source-unwritten fields retain their
 * caller allocation provenance; actual1CED10 initializes them later.
 * Required current_viewport is original graphics+58. It observes completed
 * actor construction and the partial21E1E0 projection publication, and may
 * mutate current fields while preserving these owners and finite transform.
 * Fresh constructor completing allocation/registration contracts apply.
 * Input owners are stable, disjoint, exclusive; source-contextNULL/mode1 only.
 *0 complete,1 invalid before effects,2 committed failure (never retry). No
 * class factory, authored Camera properties/listener/activation, camera stack,
 * update/render scheduling or palette initialization is implied. */
int uc_scene_camera_base_construct(const UCEntityFreshContext *,const UCEntityFreshView *,
    UCSceneProjection *,UCBulletMotion *,void *(*current_viewport)(void *),void *);
#ifdef __cplusplus
}
#endif
#endif
