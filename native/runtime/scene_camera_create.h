#pragma once
#include "scene_camera.h"
extern "C" {
#include "../entity_attached_create.h"
}

/* Known retail Camera row4F8CB8 ->1CF2D0 ->1CED10 only. The caller supplies
 * the actual active-descriptor slot and the SAME descriptor used by entities'
 * definition pointer. Its class-name string must equal "Camera" case-sensitively
 * and outlive the entity. This specializes a preexisting registered class; it
 * does not reconstruct generic factory registration or unknown classes.
 * allocate must create a real fresh NativeSceneCamera and publish its view to
 * owners.entities' resolver before returning. Source-unwritten fields retain
 * that allocation's documented provenance. Borrowed owner/descriptor/active/
 * output slots are distinct and stable, with successful capacity pre-admitted.
 * All services complete without
 * exceptions/reentrancy; the active slot/descriptor/name remain authoritative.
 * The concrete Camera constructor runs here, never through a success stub.
 *0 complete,1 invalid before effects,2 committed failure. After allocation,
 * object receives the actual owner even on failure; keep its partial borrowers
 * alive and tear them down before close_detached/delete. ID is published only
 * on success. Active descriptor is cleared NULL after allocation/construction,
 * never restored. NULL allocation is native committed failure, not a successful
 * original207D20 NULL-object path. No main or generic Camera deletion owner. */
int native_scene_camera_create(const NativeSceneCameraOwners &owners,
    UCEntityProperties **active,void *allocation_owner,
    NativeSceneCamera *(*allocate)(void *),const int32_t *parent,
    UCEntityProperties &properties,const char *class_name,
    NativeSceneCamera **object,int32_t *id);
