#pragma once
#include "mission_world.h"
#include "firstperson.h"
extern "C" {
#include "../scene_camera_stack.h"
#include "../player_damage.h"
#include "../player_blast.h"
#include "../firstperson_aim.h"
}

struct NativePlayerImpactServices {
 void*owner=nullptr;
 int(*statistic)(void*,unsigned offset)=nullptr;
 int(*shield_sound)(void*,const char*)=nullptr;
 int(*shield_spark)(void*,const float point[3],const float rotation[3])=nullptr;
};
struct NativePlayerBlastServices {
 void*owner=nullptr;
 float*shared_attenuation=nullptr; //Actual shared4E7D38; fresh session value1.
 UCExplosionObstruction obstruction=nullptr;
 int(*secondary_pain)(void*)=nullptr;
 int(*sound)(void*,const char*)=nullptr;
 int(*parameter)(void*,const char*event,const char*name,float)=nullptr;
 int(*bus_filter)(void*,const char*,float cutoff,float attack,float hold,float release)=nullptr;
 int(*group_pitch)(void*,const char*,float)=nullptr;
 int(*enabled)(void*)=nullptr; //Gamevirtual2C &&20A130(current player view).
};
// Registry connection for the existing PC player, not a replacement D3910
// constructor. Movement/rendering still run once in main's physics step.
// Body and pitch are borrowed directly; no second player pose is retained.
class NativeMissionPlayer {
    NativeMissionWorld *world_=nullptr;
    NativeFirstperson *player_=nullptr;
    UCFirstpersonAimHit target_{-1,0,300.f,0,{}}; //D3910 source388..3A0;44C5B0=300.
    float previous_position_[3]{}; //D0080 snapshot, owned across subsequent NPC queries.
    UCPlayerDamageState damage_state_{};
    UCPlayerDamageServices damage_services_{};
    NativePlayerImpactServices impact_services_{};
    NativePlayerBlastServices blast_services_{};
    UCPlayerConcussion concussion_{};
    float concussion_feedback_=0; //Actual actor664 contribution to history5F890.
    const uint32_t *damage_world_flags_=nullptr;
    uint32_t selected_level74_=0;
    UCLevelBytes disable_orders_{}; //Authored D47E7 list; mission level outlives this owner.
    int (*damage_elapsed_gate_)(void*)=nullptr;
    int (*damage_pain_step_)(void*)=nullptr;
    int step();
    int blast(const UCExplosionBlast*,const uint32_t*sender);
    UCEntityEventSubscription rows_[6]{};
    UCEntityEventListener listener_{};
public:
    NativeMissionEntity entity;
    NativeMissionPlayer()=default;
    NativeMissionPlayer(const NativeMissionPlayer&)=delete;
    NativeMissionPlayer&operator=(const NativeMissionPlayer&)=delete;
    int construct(NativeMissionWorld &,uint32_t source,NativeFirstperson &);
    // PC resource/input transition for a quiet opening. Does not claim full
    // C2670/CC240 effect parity; active actions remain rejected before mutation.
    // Completes original C8E90 environmental kinds12/13/15 using the SAME live
    // health/equipment/camera owners. Other damage kinds remain explicit errors.
    // Services and world flags must outlive this player; setter may precede construct.
    int set_damage_services(const UCPlayerDamageServices&,const uint32_t *world_flags,
        uint32_t selected_level74,int (*elapsed_gate)(void*),int (*pain_step)(void*));
    int set_impact_services(const NativePlayerImpactServices&);
    int set_blast_services(const NativePlayerBlastServices&);
    //After the same player's C0C80 camera response: C4980, then691B0.
    int after_camera_response();
    const UCPlayerConcussion&concussion_state()const{return concussion_;}
    const float&concussion_feedback()const{return concussion_feedback_;}
    int damage(float,const UCPlayerFireDamage&);
    const UCPlayerDamageState&damage_state()const{return damage_state_;}
    int take_control();
    int return_control();
    int close_detached();
    // C36B0 target-query subset. 1 found,0 original-unhandled,-1 unsupported.
    // Source3A8/world10 remain outside this target-query adapter; action170
    // and actor202 are the live action/switching owners. Target getters expose the
    // last actual published ray, retained across every pellet and queued injury.
    // Only C1020's initial hit stores are owned; no autoaim classification.
    int publish_target(const UCFirstpersonAimCollision &);
    int target_property(const char *name,int32_t &out)const;
    int target_scalar_property(const char *name,double &out,uint32_t session_mode,uint32_t health_tier)const;
    int snapshot_motion(); //Immediately before PC player translation integration.
    int target_vector_property(const char *name,float out[3])const;
    int target_matrix_property(const char *name,float out[16])const;
    NativeFirstperson *player()const{return player_;}
};

// Register the existing PC camera, without creating a second camera simulation.
// The stack token is identity only; transform aliases Firstperson.camera_world.
// World and any cutscene cameras must close before this owner's destruction.
class NativePlayerCamera {
    NativeMissionWorld *world_=nullptr;
public:
    NativeMissionEntity entity;
    UCSceneCamera token{};
    UCSceneCameraStack stack{};
    UCSceneViewport viewport{};
    NativePlayerCamera()=default;
    NativePlayerCamera(const NativePlayerCamera&)=delete;
    NativePlayerCamera&operator=(const NativePlayerCamera&)=delete;
    ~NativePlayerCamera(){uc_scene_camera_stack_close(&stack);}
    int construct(NativeMissionWorld &,NativeMissionPlayer &,unsigned width,unsigned height);
    int resize(unsigned width,unsigned height){return !width||!height?1:
        uc_scene_viewport_begin(&viewport,nullptr,0,0,(float)height,(float)width,float(width)/height);}
    int close_detached();
    bool selected()const;
};
