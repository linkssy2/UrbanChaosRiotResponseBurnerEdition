#pragma once
#include "firstperson_animation.h"
extern "C" {
#include "../character_move.h"
#include "../player_controller.h"
#include "../vector_random.h"
#include "../character_face.h"
}

// The2AEE30 locomotion subgraph, not the complete body/task scheduler. The bank
// outlives this owner and emitted event references. Each leaf retains its own
// seek cache, including while its graph weight is zero.
class NativeCharacterClipGraph {
    struct Leaf {
        UCAnimationSample sample{};
        const NativeFirstpersonClip *resource=nullptr;
        ~Leaf(){uc_animation_sample_free(&sample);}
    } leaves_[4];
    UCCharacterClipSequence sequence_{0,{0,0},1};
    UCCharacterClipLoop loop_{};
    UCCharacterClipRequest requests_[4]{};
    uint8_t roots_[4]{},emitted_[4]{};
    float frequencies_[4]{},leaf_previous_[4]{},turn_scale_=1;
    const float *brute_blend_=nullptr;
    unsigned leaf_count()const{return (sequence?2u:1u)+(brute_blend_?2u:0u);}
    double base_timing(unsigned)const;
    bool sequence=false,one_shot_=false,hold_last_=false;
    std::shared_ptr<const NativeFirstpersonClip> direct_clip_;
    std::shared_ptr<UCCharacterClipCompletion> loop_completion_;
    void load_leaf(const NativeFirstpersonClips &,const char *,int32_t,const char *,unsigned);
    void load_leaf(const NativeFirstpersonClip &,uint32_t,int32_t,unsigned);
public:
    UCCharacterClipCompletion completion{};
    UCAnimationEventQueue events{};
    float rotation_scale=0,consumed[5]{};
    NativeCharacterClipGraph(const NativeFirstpersonClips &,const char *prefix,
        int32_t root_bone,const UCCharacterLocomotionPlan &,const float *brute_blend=nullptr);
    NativeCharacterClipGraph(const NativeFirstpersonClips &,const char *prefix,int32_t root_bone,const char *action);
    NativeCharacterClipGraph(const NativeFirstpersonClips &,const char *prefix,int32_t root_bone,const char *base,const char *first);
    NativeCharacterClipGraph(std::shared_ptr<const NativeFirstpersonClip>,uint32_t bones,int32_t root_bone,
        std::shared_ptr<UCCharacterClipCompletion>);
    NativeCharacterClipGraph(std::shared_ptr<const NativeFirstpersonClip>,uint32_t bones,int32_t root_bone,bool hold_last);
    //2B4350 initializes the leaf's previous phase, independently of the loop
    // phase initialized by2B2780. Used by the authored body baseline only.
    NativeCharacterClipGraph(const NativeFirstpersonClips &,const char *prefix,int32_t root_bone,
        const char *action,float previous_phase);
    NativeCharacterClipGraph(const NativeFirstpersonClips &,const char *prefix,int32_t root_bone,
        const char *action,float root_scale,bool one_shot);
    NativeCharacterClipGraph(const NativeCharacterClipGraph &)=delete;
    NativeCharacterClipGraph &operator=(const NativeCharacterClipGraph &)=delete;
    // Reports first-clip completion to its actual task owner. That owner must
    // deliver the task callback before sample(); no navigation/events fabricated.
    bool advance(float delta);
    bool advance(float delta,const float weights[5]);
    void sample(NativeRenderer &,NativeAnimatedModel &,const UCAnimationBlendMath &);
    struct Request {
        UCAnimationBindingSample sample;
        const UCAnimationAnnotations *annotations;
        float previous,root_scale;uint8_t has_root;
    };
    void append(std::vector<Request> &);
    void loop_weights(const float input[5],float output[5])const;
    bool has_initial_clip()const{return sequence;}
    bool one_shot_active()const{return one_shot_&&(hold_last_||loop_.phase<1);}
    float blend_in()const{return sequence?leaves_[0].sample.clip.source_timing[1]:(float)base_timing(1);}
    float blend_out()const{return(float)base_timing(2);}
};

// Borrowed source callback virtual4/8. Its enclosing task must remain alive
// until completion or cancellation. Callback reentry is not admitted here.
struct NativeCharacterAnimationCallback {
    void *owner=nullptr;
    void (*notify)(void *,bool cancelled)=nullptr;
};
// Concrete locomotion graph ownership behind the qualified2B8DB0 scheduler.
// This still requires the real body, movement task and navigator around it.
struct NativeCharacterBody;
struct NativeCharacterMoveTarget {float position[3]{},relative_heading=0;int32_t mode=0;uint8_t override_heading=0,near_target=0;};
class NativeCharacterAnimationSchedule {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeCharacterAnimationSchedule(const NativeFirstpersonClips &,const char *prefix,int32_t root_bone);
    ~NativeCharacterAnimationSchedule();
    NativeCharacterAnimationSchedule(const NativeCharacterAnimationSchedule &)=delete;
    NativeCharacterAnimationSchedule &operator=(const NativeCharacterAnimationSchedule &)=delete;
    //2B15D0 class1/2/10/11 baseline. All take the same default body profiles and
    // StandIdle leaf. Uses the session's shared vector RNG exactly once;
    // an isolated foreground schedule remains valid without this body layer.
    // The caller must own grounded-body/fall and explicit turn task dispatch.
    void initialize_baseline(const NativeCharacterBody &,UCVectorRandom &,uint32_t character_class);
    void initialize_class2_baseline(const NativeCharacterBody &body,UCVectorRandom &random){initialize_baseline(body,random,2);}
    void queue(const UCCharacterLocomotionPlan &,NativeCharacterAnimationCallback *completion=nullptr,
        NativeCharacterAnimationCallback *first_finished=nullptr,bool interrupt=true);
    //2AC4B0/2AC390: direct clip, no prelude/callback/time limit. Same +40
    //scheduler as locomotion, with a separately retained2B2780 cycle handle.
    std::shared_ptr<UCCharacterClipCompletion> queue_loop(std::shared_ptr<const NativeFirstpersonClip>);
    //2ADE70 direct clip. The source fourth argument selects the type8 marker
    // disables gravity while the clip emits; callback precedes its fade-out.
    void queue_clip(std::shared_ptr<const NativeFirstpersonClip>,NativeCharacterAnimationCallback *,bool hold_last=false,bool authored_motion=true);
    bool authored_motion()const;
    //2ADD60/2ADF00 with both optional flags zero: named one-shot on the
    //foreground(+40) or independent overlay(+38), with real completion.
    void queue_action(const char *name,NativeCharacterAnimationCallback *,bool overlay=false);
    //2AA5B0/2B8750: cover start/held loop/recovery, with source head-height
    //predicate supplied by the actual posed model. stop() requests recovery.
    void queue_duck(bool already_low,NativeCharacterAnimationCallback *started);
    //2AD210/2AD6D0: independent +30 speech channel. Copies authored bytes;
    // borrows the session RNG through pending graphs and their retiring fades.
    void queue_face(const uint8_t *phonemes,uint32_t ticks,UCGameRandom &);
    void stop_face();
    const std::vector<UCCharacterFacePose> &face_poses()const;
    // Class10's actual+3C gun-stance owner; this boundary admits its source
    // modes0/1/4 with no externally supplied clip; reload/fire own callbacks.
    void cop_stance(int32_t mode,bool transition=true,bool already_low=false);
    void cop_shoot(NativeCharacterAnimationCallback *completion);
    void cop_reload(NativeCharacterAnimationCallback *completion=nullptr);
    bool cop_stance_busy()const;
    int32_t cop_stance_mode()const;
    int32_t cop_weapon_mode()const;
    //Class1's2A9860 blend owner drives the actual RunBatUp/Run/Run factory.
    void brute_movement_mode(int32_t mode);
    //2AC250/2AC2E0: separate body aim and speech head-look points. Mode0
    //turns the body through authored clips; mode4 leaves body heading alone.
    void aim(const float point[3],uint32_t mode=4,NativeCharacterAnimationCallback *completion=nullptr);
    void look_at(const float point[3]);
    void stop_look();
    void stop_aim(); // Inner+1C/2AC2B0: cancel the actual aim task.
    void stop(NativeCharacterAnimationCallback *completion=nullptr);
    // Body and completion receiver remain borrowed until the graph retires.
    void queue_approach(const NativeCharacterBody &,const float target[3],const float direction[3],
        NativeCharacterAnimationCallback *completion=nullptr,float rate=2);
    // Target/body remain borrowed through fades; used by the real movement owner.
    void queue_locomotion(const NativeCharacterBody &,const NativeCharacterMoveTarget &,bool near_target);
    //2AE180/2AE380: retained outer task and its own nested scheduler. The
    //target and arrival slot remain borrowed through retiring fades.
    void queue_movement(const NativeCharacterBody &,const NativeCharacterMoveTarget &,
        NativeCharacterAnimationCallback **arrival);
    bool movement_busy()const;
    const float *heading_override()const;
    // CPU animation requests before pose sampling; borrowed until next advance.
    // update() calls this once. Do not call both for the same simulation step.
    const std::vector<NativeCharacterClipGraph::Request> &advance(float delta);
    void update(NativeRenderer &,NativeAnimatedModel &,const UCAnimationBlendMath &,float delta);
    bool empty()const;
    float rotation_scale()const;
    const UCAnimationEventQueue &events()const;
    const float *movement_helper()const;
};

// Grounded NPC motion adapter: actual sampled animation -> shared PC capsule.
// This is not the full2AB250 body (override bodies, ground probes, look/IK and
// navigation remain separate). Call AFTER schedule.update at exactly1/30s.
// A failed step leaves body state unchanged; the already sampled graph does
// not roll back. Caller stops the session on failure rather than sampling twice.
struct NativeCharacterBody {
    UCPlayer physics{};
    UCCharacterHeading heading{10,2,0,0,0};
    float world[16]{},velocity[3]{},heading_delta=0;
    NativeCharacterBody(const float position[3],float yaw);
    void step(const NativeAnimatedModel &,float rotation_scale,
        const UCCollisionAsset &,const UCSolidBlockGeometry *const *blocks,size_t count,
        const float helper[4],const float displacement[3],const float *heading_override=nullptr,bool authored_motion=false);
};

//2AE180 target/callback ownership and2AE380 movement task. The enclosing actor
// owns navigation and keeps the body alive. This owner samples the graph; the
// actor then advances body collision once. No arrival is inferred by a timer.
class NativeCharacterMovement {
    const NativeCharacterBody &body_;
    NativeCharacterMoveTarget target_{};
    NativeCharacterAnimationCallback *completion_=nullptr;
    bool busy_=false;
    void notify(bool cancelled);
public:
    NativeCharacterAnimationSchedule graph;
    NativeCharacterMovement(const NativeFirstpersonClips &,const char *prefix,int32_t root,const NativeCharacterBody &);
    ~NativeCharacterMovement();
    void request(const NativeCharacterMoveTarget &,NativeCharacterAnimationCallback *completion=nullptr);
    void stop();
    void update(NativeRenderer &,NativeAnimatedModel &,const UCAnimationBlendMath &,float delta);
    void clear_heading(){target_.override_heading=0;} // Original33020 movement owner store.
    bool arrival_pending()const{return completion_!=nullptr;}
};
