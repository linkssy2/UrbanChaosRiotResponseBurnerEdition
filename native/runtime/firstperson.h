#pragma once
#include "model.h"
#include "firstperson_animation.h"
extern "C" {
#include "../firstperson_player.h"
#include "../firstperson_camera.h"
#include "../firstperson_projection.h"
#include "../mainplayer_state.h"
#include "../mainplayer_presentation.h"
#include "../mainplayer_events.h"
#include "../weapon_state.h"
#include "../model_bounds.h"
#include "../player_equipment_transition.h"
#include "../player_reload.h"
#include "../player_health.h"
#include "../camera_push.h"
#include "../game_random.h"
#include "../feedback_manager.h"
#include "../scene_camera_world.h"
#include "../player_shield.h"
#include "../player_shield_hit.h"
#include "../mission_flags.h"
#include "../spot_light.h"
}

class NativePlayerAudio;
class NativePlayerFootsteps;

struct NativeFirstpersonEquipment {
    UCMainplayerItemSpec spec{};
    UCWeaponState state{};
    float reload_progress=0; // Actual item+4C, retained through switches.
    UCMainplayerPresentationResource resource{};
    NativeAnimatedModel gun,arms;
    std::unique_ptr<NativeAnimatedModel> flashlight_model;
    NativeFirstpersonClips clipbank;
    std::vector<UCTextureSequenceState> appearance;
    int32_t shield_bone=-1;
    int32_t flashlight_bone=-1,muzzle_bone=-1;
};

/* Fresh Westside shield/Taser/pistol/shotgun presentation and switching. Primary reload
 * and PC firing join qualified scalar cadence/ammo, authored Fire clips and
 * CameraPush to a real caller-owned collision query. Taser uses only its genuine
 * FireRaise/Loop/Lower presentation, with no target, audio-loop or stun claim.
 * Shield uses the source hold/return selection and frontal eligibility. Bash
 * presentation emits a real caller-owned short query on the authored marker;
 * original C57C0 damage/effects remain outside this preview adapter.
 * It is not the
 * complete CFBC0/CBA20 entity/effect chain. No save profile is read or modified. */
struct NativeFirstperson {
    NativeFirstperson()=default;
    NativeFirstperson(const NativeFirstperson &)=delete;
    NativeFirstperson &operator=(const NativeFirstperson &)=delete;
    NativeFirstperson(NativeFirstperson &&)=delete;
    NativeFirstperson &operator=(NativeFirstperson &&)=delete;
    // Borrowed session must outlive this player; null retains the silent candidate.
    NativePlayerAudio *audio=nullptr;
    // Same audio/world owners; optional static-ground footstep bridge.
    NativePlayerFootsteps *footsteps=nullptr;
    // Same borrowed session stream as the rest of the preview; never reseeded here.
    UCGameRandom *random=nullptr;
    // Shared world55E00 and camera2C0 channel; main owns both lifetimes.
    UCFeedbackManager *feedback=nullptr;UCVectorRandom *feedback_random=nullptr;
    const float *feedback_clock=nullptr;UCSceneCameraAuxiliary *feedback_auxiliary=nullptr;
    uint32_t feedback_mode=0,feedback_world_flags=0;uint8_t feedback_bypass=0;
    // True means the actual static query completed (hit or miss), not that
    // original projectile, damage, shell or particle services were completed.
    bool (*preview_shot)(void *,uint32_t,const float origin[3],const float direction[3])=nullptr;
    void *preview_shot_context=nullptr;
    bool (*preview_throw)(void*,const float eye_frame[16],float pitch,float charge)=nullptr;
    void *preview_throw_context=nullptr;
    uint32_t preview_throws=0,throw_rejections=0;
    uint8_t throw_stage=0;
    bool throw_capture_pending=false,throw_frame_ready=false,throw_emitted=false;
    float throw_charge=0,throw_pitch=0,throw_frame[16]{};
    bool fire_held=false,fire_needs_release=false,preview_firing=false,preview_shot_pending=false;
    bool shield_held=false,shield_temporary=false,shield_bashing=false,shield_bash_pending=false;
    UCPlayerShieldRecoil shield_recoil{}; // Actual244/248/24C, shared damage/input owner.
    float &shield_impact=shield_recoil.impact;
    UCPlayerImpactState impact{};float impact_rate=0,shield_feedback=0;
    UCPlayerShieldMarks shield_marks{};
    NativeGpuMesh shield_mark_mesh;ComPtr<ID3D11ShaderResourceView> shield_mark_texture;
    unsigned shield_bashes=0,shield_melee_queries=0;
    bool (*preview_melee)(void *,const float origin[3],const float end[3])=nullptr;
    void *preview_melee_context=nullptr;
    uint8_t taser_phase=0; // PC presentation:0 idle,11 raise,13 loop,12 lower; no NPC stun owner.
    uint32_t taser_cycles=0;
    float preview_clock=0;
    uint32_t preview_shots=0,preview_queries=0,preview_shot_item=0;
    UCFirstpersonPlayerMotion motion{};
    UCFirstpersonLookBase actor{};
    UCFirstpersonCamera camera{};
    UCFirstpersonCameraInput camera_input{};
    UCCameraPush camera_push{};
    UCMainplayerState inventory{};
    // Borrow the mission's actual flag bytes; load retains the matching level map.
    uint8_t *mission_flags=nullptr;
    size_t mission_flag_count=0;
    const UCLevel *mission_level=nullptr;
    float health_maximum=0;
    UCMainplayerActionState action{};
    UCMainplayerEquipControl equip_control{};
    UCPlayerEquipmentTransition switching{};
    UCPlayerEquipmentReset action_reset{};
    UCMainplayerReloadOverlay reload_overlay{};
    UCAnimationController *reload_controllers[2]{};
    bool reload_requested=false;
    UCMainplayerPresentationResource resource{};
    UCAnimationBlendMath math{};
    // Bounded owned shield/Taser/pistol/shotgun resources precede their borrowing controllers.
    std::unique_ptr<NativeFirstpersonEquipment> equipment[5];
    NativeFirstpersonEquipment *equipped=nullptr;
    NativeFirstpersonAnimations animations[2];
    UCAnimationEventQueue events[2]{};
    UCModelBounds bounds[2]{};
    UCMainplayerEvents player_events{};
    float prior_position[2]{},movement=.5f,object[16]{},camera_world[16]{},look_anchor[16]{};
    float previous_object[16]{},previous_camera[16]{},previous_anchor[16]{};
    bool presentation_history=false;
    bool flashlight_requested=false,shotgun_secondary=false,aim_held=false;
    unsigned secondary_shots=0;
    unsigned updates=0,pending_footsteps=0,reload_pulses=0,unhandled_arm_events=0,unhandled_weapon_events=0;
    void load(NativeRenderer &,const std::filesystem::path &game,
              const std::filesystem::path &assets,const UCLevel &,const UCLevelRestart &,const struct UCCheckpointSnapshot *saved=nullptr);
    // Explicit test-build grant. Does not modify normal reset or saves.
    void enable_preview_throwables(NativeRenderer&,const std::filesystem::path&game,
        const std::filesystem::path&assets,uint32_t count=10);
    bool pickup_molotov(NativeRenderer&,const std::filesystem::path&game,const std::filesystem::path&assets);
    /* PC1/2/3/4/wheel resolves requested inventory; C4CB0 requests item2/17.
     * Persistent shield0/Taser1 are a PC policy (source category2 selection is quick-use).
     * CF8A0 performs Lower -> equipment commit -> Raise -> Idle at fixed ticks for0/1/2/17
     * only: PC preview Fire can be interrupted; no aim/alternate/damage input. Pending reload resumes on the
     * actual selected item. False means
     * unsupported, unowned, unchanged or an in-flight switch. No ammo reset.
     * C8930 uses the existing passive resources adapter, not its full effects. */
    bool select_weapon(NativeRenderer &,uint32_t item);
    /* R queues a PC reload request; statistics/tutorial/permission admission
     * from original CF770 remains outside this adapter. Actual D1F40, authored
     * events, ammo transfer, interrupted progress and resume are owned. */
    bool request_reload(NativeRenderer &);
    /* PC mouse level input. For primary guns a bound query and shared RNG are required before
     * debit. Fire waits for the authored ReadyToFire/controller completion and
     * original scalar deadline; R and selection can interrupt its presentation.
     * Call before_physics/after_physics as one completed tick. Clearing input on
     * pause/focus loss never advances effects. No original trigger-owner claim. */
    bool accepts_input()const{return action.state!=13;}
    void set_fire(bool held){fire_held=held&&accepts_input();if(!fire_held)fire_needs_release=false;}
    void set_shield(bool held){shield_held=held&&accepts_input();}
    bool shield_blocks(const float incoming_direction[3]) const;
    int shield_input_response(float &forward,float &side); //BC930 then BF398, once per PC physics tick.
    int shield_surface(float point[3]); //C28F0, creates retained bullet mark and source world position.
    int shield_frame(float out[16])const;
    bool update_shield_input();
    void update_shield_bash();
    void update_preview_fire(float delta);
    void update_taser_presentation();
    void update_molotov_presentation(float delta);
    void finish_preview_fire();
    void complete_preview_shot(const float eye_frame[16]);
    /* Resolved CameraPush payload only; shot/effect entity creation remains a
     * separate owner. Added pulses advance once per completed physics tick. */
    bool add_camera_push(const UCCameraPushInput &);
    void update_reload();
    uint32_t current_animation() const {return action.animation;}
    void apply_action(uint32_t action,int32_t repeats=0,int32_t blend=0);
    // Returns the ordinary recovered movement axes for the collision controller.
    // Equipment/state speed overrides beyond this boundary remain separate.
    UCFirstpersonInputMovement before_physics(NativeRenderer &,float forward,float side,bool crouched,float delta);
    int apply_camera_feedback(); //actual1CD520 over completed camera.world
    void after_physics(const float body[3],float eye,float pitch,float yaw,bool crouched,float delta);
    void render_matrices(const float render_look[16],float aspect,float world_camera[16],float weapon_matrix[16],float object_view[16],float fraction=1) const;
    bool flashlight_compatible()const;
    void set_flashlight(bool enabled);
    // Same interpolated hand pose and world transform used by this presentation.
    // Source41E00 parameters; incompatible equipment/cutscenes emit no light.
    bool flashlight(UCNativeControlSpot &,const float rendered_camera[16],const float object_view[16],float fraction=1,float plane[4]=nullptr)const;
    void draw(NativeRenderer &,const float weapon_matrix[16],const float object_view[16],float inverse_exposure=1,float fraction=1,
        NativeModelLighting *lighting=nullptr,const NativeLightingFrame *private_frame=nullptr);
    void draw_lit(NativeRenderer &,NativeModelLighting &,const float weapon_matrix[16],const float object_view[16],float inverse_exposure=1,float fraction=1);
    /* Existing passive resource composition; initial load and CF8A0 commit only. */
    void commit_equipment();
};
