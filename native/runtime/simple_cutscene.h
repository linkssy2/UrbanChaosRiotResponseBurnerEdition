#pragma once
#include "cutscene_camera.h"
#include "model.h"
#include "player_audio.h"
extern "C" {
#include "../simple_cutscene.h"
#include "../animation_controller.h"
#include "../fade.h"
#include "../environment_history.h"
}
class NativeSimpleCutscene;

/* Remaining live-player/world endpoints. These must perform the real operation;
 * no successful default is supplied. camera uses the SAME current provider and
 * registry. history is the SAME game-manager+930 effect (55EB0->44630).
 * Handoff sends the original event0x22 through that registry to provider player0;
 * restart includes95CB0 and the real4C150 snapshot, not just a position change.
 * Its direct goto_restart field is zero, but normal OnEnd command16 operates
 * RestartPoint288 and indirectly reaches4C150. That receiver remains required. */
struct NativeSimpleCutsceneServices {
    NativeCutsceneCameraServices camera{};
    int (*provider_ready)(void *,void *,uint8_t *)=nullptr;
    int (*skip_input)(void *,double *)=nullptr;
    int (*restart)(void *,int32_t)=nullptr;
    UCEnvironmentHistoryEffect *history=nullptr;
};

/* Actual authored SimpleCutscene, selected through the same mission factory.
 * The initial implementation admits the real empty-bank/zero-animation-event
 * opening profile. It owns one model, animation controller, local trigger and
 * independently registered dynamic camera; it borrows SAME audio session, fade manager and mission graph.
 * Native model/GPU ownership is a PC adapter, not original graphics registration.
 * No guessed timeline, player handoff, restart or arbitrary sound fallback.
 * Caller binds entity.view before construct and retains it until world detach.
 * Camera callbacks/ownership survive parent release; the world binding supplies
 * final allocation release after owned cleanup. */
class NativeSimpleCutscene {
    NativeRenderer *renderer_=nullptr;
    NativeMissionWorld *world_=nullptr;
    NativePlayerAudio *audio_=nullptr;
    NativeSimpleCutsceneServices services_{};
    UCFade *fade_=nullptr;
    const UCFadeServices *fade_services_=nullptr;
    UCSceneCameraStack *stack_=nullptr;
    const uint32_t *bone_event_=nullptr,*fov_event_=nullptr;
    UCAnimationBlendMath math_{};
    NativeAnimatedModel model_;
    UCAnimationTemplate skeleton_{};
    UCAnimationAnnotations annotations_{};
    UCAnimationController animation_{};
    struct Camera;
    Camera *camera_=nullptr; // Observer; dynamic world binding owns camera lifetime.
    NativePlayerAudio::EmptySource *sound_=nullptr;
    std::vector<uint8_t> bank_;
    std::vector<int32_t> finish_,wait_,prefade_;
    const char *clip_=nullptr;
    uint32_t end_type_=0;
    uint32_t model_flags_=0; //Only B5D60 bit40; other primary+60 bits are not mirrored.
    float fov_=0;
    bool ready_=false,animation_started_=false,trigger_bound_=false;
    int failure_=0;
    int advance(float);
    int handoff();
    int finish_flags();
    int start_camera();
    UCSimpleCutsceneServices hooks();
public:
    NativeMissionEntity entity;
    UCSimpleCutscene fields{};
    NativeSimpleCutscene()=default;
    ~NativeSimpleCutscene();
    NativeSimpleCutscene(const NativeSimpleCutscene&)=delete;
    NativeSimpleCutscene&operator=(const NativeSimpleCutscene&)=delete;
    int construct(NativeRenderer &,NativeMissionWorld &,uint32_t source_index,
        const UCArchive *,const uint8_t archive_hash[32],const std::filesystem::path &catalog,
        const UCAnimationBlendMath &,NativePlayerAudio &,UCSceneCameraStack &,
        const uint32_t *bone_event,const uint32_t *fov_event,UCFade &,
        const UCFadeServices &,const NativeSimpleCutsceneServices &);
    int update();
    int late();
    //114F90 ordering for this owned PC model/empty-source profile. Retires into
    //the world's pending queue; the binding must supply the final destructor.
    int remove();
    int close_detached();
    // PC inspection submission; not the original scene model registration/pass.
    void draw(NativeRenderer &,const float view_projection[16],float inverse_exposure=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    NativeCutsceneCamera *camera()const;
    const NativeAnimatedModel &model()const{return model_;}
    const UCAnimationController &animation()const{return animation_;}
    uint32_t model_flags()const{return model_flags_;}
    int failure()const{return failure_;}
};
