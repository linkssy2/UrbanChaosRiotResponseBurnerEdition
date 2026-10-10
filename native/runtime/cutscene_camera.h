#pragma once
#include "mission_world.h"
#include "scene_camera.h"
extern "C" {
#include "../cutscene_camera_construct.h"
#include "../entity_attached_create.h"
#include "../cutscene_camera_cleanup.h"
}
class NativeCutsceneCamera;
/* Actual external endpoints still required by1733C0. They operate on the same
 * current player/provider and parent model; they are never completed noops.
 * preflight is pure and returns0. Their state/storage survives every call. */
struct NativeCutsceneCameraServices {
    void *owner=nullptr;
    int (*preflight)(void *)=nullptr;
    void *(*current_provider)(void *)=nullptr;
    int (*player)(void *,void *,int32_t,int32_t *)=nullptr;
    int (*control)(void *,int32_t)=nullptr;
    int (*mode)(void *,void *,int32_t,int32_t)=nullptr;
    int (*bone_matrix)(void *,UCEntityEventTarget *,int32_t,float[16],uint8_t *)=nullptr;
    int (*remove)(void *)=nullptr;
    /* Both enable the actual camera lifecycle; release must really delete its
     * enclosing NativeCutsceneCamera. Existing scoped callers may omit both
     * and retain required external remove; gameplay deletion then rejects. */
    int (*return_control)(void *,int32_t,uint32_t)=nullptr;
    void (*release_camera)(void *,NativeCutsceneCamera *)=nullptr;
    // Independent heap owner: shutdown releases it after world detachment too.
    bool release_on_close=false;
};
/* One real dynamic LockToParentBoneCamera: existing mode1 base, projection,
 * motion, actual listener registry and same-world relative records. This owner
 * is immovable; its borrowed stack must outlive its actual deleting destruction
 * or world.close(). No authored level slot is added. Relative construction during
 * a step uses the world's current/new-parent admission. Failed construction retains ownership
 * for real detach/close, never retry. No SimpleCutscene timeline is invented.
 * entity.camera supplies ONLY an opaque identity to the existing camera list;
 * none of its ordinary-Camera fields are read. fields/projection/transform are
 * the sole authoritative LockToParentBoneCamera state.
 */
class NativeCutsceneCamera {
public:
    NativeCutsceneCamera();
    NativeCutsceneCamera(const NativeCutsceneCamera&)=delete;
    NativeCutsceneCamera&operator=(const NativeCutsceneCamera&)=delete;
    NativeSceneCameraEntity entity;
    UCCutsceneCamera fields{};
    UCSceneView view{};
    int create(NativeMissionWorld &,const int32_t *parent,
        const UCEntityProperties &,UCSceneCameraStack &,const uint32_t *bone_event,
        const uint32_t *fov_event,const NativeCutsceneCameraServices &);
    int send(uint32_t event,const void *payload,uint32_t bytes,uint8_t *handled);
    /* Only selected-camera view construction/publication. World.tick performs
     * real step/late separately; no hidden animation or timing advance here. */
    int publish(uint32_t layer,uint32_t clear,uint32_t flags,
        UCSceneMatrixProvider &,const UCSceneMatrixServices &);
    int failure()const{return failure_;}
private:
    NativeMissionWorld *world_=nullptr;
    UCSceneCameraStack *stack_=nullptr;
    NativeCutsceneCameraServices services_{};
    UCEntityFreshContext context_{};
    UCEntityProperties properties_{};
    UCCutsceneCameraListener listener_{};
    UCEntitySubscriptions *subscriptions_=nullptr;
    const uint32_t *bone_event_=nullptr,*fov_event_=nullptr;
    int failure_=0;
    bool constructed_=false;
    int construct();
    int step();
    int late();
    int close_detached();
    UCCutsceneCameraCleanup cleanup();
    int remove();
    int destroy();
};
