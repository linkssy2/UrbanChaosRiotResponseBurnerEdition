#pragma once
extern "C" {
#include "../scene_camera_follow.h"
#include "../scene_camera_start.h"
#include "../scene_camera_events.h"
#include "../scene_camera_world.h"
#include "../scene_view_publish.h"
#include "../feedback_manager.h"
}

/* The existing world owns registration, names, stack, input and feedback.
 * The player view aliases its actual body, flags and published tag0 matrix.
 * This adapter owns only one Camera entity and its subscription storage. */
struct NativeSceneCameraOwners {
    const UCEntityFreshContext *entities=nullptr;
    UCEntityListenerList *listeners=nullptr;
    UCEntityNames *names=nullptr;
    UCSceneCameraStack *stack=nullptr;
    UCSceneCameraAuxiliary *auxiliary=nullptr;
    const uint32_t *event_ids=nullptr;
    const float *equipped_fov=nullptr;
    UCEntityEventTarget *player=nullptr;
    UCSceneCameraPlayer player_fields{};
    // Actual PC action level, shared with the firing gate; no edge consumption.
    const volatile float *fire_level=nullptr;
    const uint32_t *debug_flags=nullptr,*world_flags=nullptr;
    const uint8_t *feedback_suppressed=nullptr;
    const float *clock=nullptr;
    UCFeedbackManager *feedback=nullptr;
    UCVectorRandom *random=nullptr;
    UCSceneMatrixProvider *provider=nullptr;
    UCSceneMatrixServices matrix_services{};
};

struct NativeSceneCameraEntity {
    UCEntityEventTarget target{};
    uint32_t flags=0,flags60=0;
    int32_t parent=-1,id=-1,source=-1;
    float transform[16]{};
    const void *definition=nullptr;
    void *model=nullptr,*helper=nullptr;
    UCEntityFreshAux *aux=nullptr;
    UCEntityFreshActor actor{};
    UCEntityFreshView view{};
    UCSceneProjection projection{};
    UCBulletMotion motion{};
    UCSceneCamera camera{};
};

class NativeSceneCamera {
public:
    NativeSceneCamera();
    NativeSceneCamera(const NativeSceneCamera&)=delete;
    NativeSceneCamera&operator=(const NativeSceneCamera&)=delete;
    NativeSceneCameraEntity entity;
    UCSceneView view{};
    /* Publish entity.view to the SAME world resolver before construct. All
     * borrowed owners outlive this object; successful allocations/services are
     * pre-admitted. Source-unwritten fields retain their supplied allocation
     * provenance. No PC source-tag/activation events are fabricated here. */
    int construct(const NativeSceneCameraOwners &,const UCEntityProperties &);
    int send(uint32_t event,const void *payload,uint32_t bytes,uint8_t *handled);
    int start();
    /* Ordinary mode2/tag0 only. Actual parent stays registered and not retiring.
     * Each call follows, selects the real stack camera, constructs its view and
     * publishes to the SAME borrowed provider. Matrices and logical fire value
     * are current caller-owned state; no second camera or player is retained.
     *0 complete,1 initial rejection,2 committed failure: discard frame/teardown.
     * Generic frame scheduling, Camera retirement and GPU passes remain outside. */
    int frame(uint32_t layer,uint32_t clear,uint32_t flags);
    /* Host teardown only AFTER registry/records/list/stack/name borrows end.
     * Releases actual owned auxiliary and subscriptions; not original deletion. */
    int close_detached();
private:
    NativeSceneCameraOwners owners_{};
    UCSceneCameraEvents *events_=nullptr;
    float scratch_[16]{};
    int failure_=0;
    bool constructed_=false;
    static int activate(void *,UCSceneCamera *,int32_t);
    static int remove(void *,UCSceneCamera *);
};
