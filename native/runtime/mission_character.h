#pragma once
#include "mission_world.h"
#include "character_animation.h"
#include "character_navigation.h"
class NativeAudioBank;
class NativeMissionCharacter;
class NativeRagdollWorld;
struct UCRagdollAsset;
class NativeCharacterFactions;
class NativeCharacterCoverPoints;
class NativeCharacterDialogueAudio;
class NativeCharacterDialogue;
class NativeDialogueText;
struct NativeDialogueLine;
struct NativeNpcThrowServices;
struct UCNpcGunControllerServices;
struct UCNpcMeleeController;
extern "C" {
#include "../character_appearance.h"
#include "../vector_random.h"
#include "../character_damage.h"
#include "../character_health.h"
#include "../model_bounds.h"
}

// Opening CharacterBurnerBlazer and authored Character/Blazer root owner.
// It borrows the same collision world,
// navigation manager and random streams as the rest of the mission. Unsupported
// combat/interrupt requests fail explicitly; this is not the general NPC AI.
struct NativeMissionCharacterServices {
    void *owner=nullptr;
    int (*ensure_molotov_template)(void *,NativeMissionWorld &,int32_t parent)=nullptr;
    int (*ensure_brute_template)(void *,NativeMissionWorld &,int32_t parent)=nullptr;
    int32_t (*player_id)(void *)=nullptr;
    NativeCharacterWorld *collision=nullptr;
    NativeRagdollWorld *ragdoll_world=nullptr;
    const UCRagdollAsset *ragdoll_asset=nullptr;
    NativeCharacterNavigationManager *navigation=nullptr;
    NativeCharacterDoors *doors=nullptr;
    NativeCharacterCoverPoints *cover_points=nullptr;
    UCArchiveLookup *texture_lookup=nullptr;
    int (*refresh_collision)(void *)=nullptr;
    int (*tags)(void *,int32_t,const char *name,bool add)=nullptr;
    int (*faction)(void *,int32_t,unsigned faction,bool add)=nullptr;
    int (*hazard)(void *,const float position[3],unsigned kind,uint8_t *hit)=nullptr;
    int (*animation_events)(void *,int32_t,const UCAnimationEventQueue &,
        const NativeAnimatedModel &,const float world[16],const NativeAudioBank &)=nullptr;
    int (*difficulty)(void *,double *normalized)=nullptr;
    int (*throw_difficulty)(void *,float minimum,float maximum,double *result)=nullptr;
    int (*throw_curve)(void *,float low,float middle,float high,double *result)=nullptr;
    int (*brute_difficulty)(void *,float low,float high,double *result)=nullptr;
    int (*brute_range)(void *,float low,float high,float begin,float end,double *result)=nullptr;
    int (*brute_curve)(void *,float low,float middle,float high,double *result)=nullptr;
    const NativeNpcThrowServices *npc_throw=nullptr;
    const UCNpcGunControllerServices *npc_gun=nullptr;
    int (*character_sound)(void *,NativeMissionCharacter &,const NativeAudioBank &,const char *,bool stop)=nullptr;
    int (*emote_distance)(void *,NativeMissionCharacter &,double *)=nullptr;
    int (*world_flags)(void *,uint32_t *)=nullptr;
    int (*mission_ordinal)(void *,int32_t *)=nullptr;
    int (*dialogue_zone)(void *,NativeMissionCharacter &,int32_t *)=nullptr; //34270 live named zone queries.
    int (*bullet_by)(void *,int32_t actor,bool ignore_friendly,uint32_t *)=nullptr;
    int (*cop_gun)(void *,NativeMissionWorld &,NativeMissionCharacter &,int32_t *gun_id)=nullptr;
    int (*medic_weapon)(void *,NativeMissionWorld &,NativeMissionCharacter &,int32_t *weapon_id)=nullptr;
    int (*medic_pillbox)(void *,NativeMissionWorld &,NativeMissionCharacter &,const float world[16],int32_t *weapon_id)=nullptr;
    int (*brute_weapon)(void *,NativeMissionWorld &,NativeMissionCharacter &,int32_t *weapon_id)=nullptr;
    int (*melee_weapon_type)(void *,int32_t,int32_t *)=nullptr;
    int (*melee_weapon_flags)(void *,int32_t,uint32_t)=nullptr;
    int (*melee_eligible)(void *,NativeMissionCharacter &,const UCNpcMeleeController &,int32_t,uint8_t *)=nullptr;
    int (*melee_strike)(void *,NativeMissionCharacter &,UCNpcMeleeController &)=nullptr;
    int (*medic_heals)(void *,int32_t named_character,int32_t *)=nullptr;
    int (*medic_set_heals)(void *,int32_t named_character,int32_t)=nullptr;
    int (*heartbeat_ready)(void *)=nullptr;
    int (*heartbeat_show)(void *,int32_t,const char *)=nullptr;
    int (*heartbeat_join)(void *,int32_t,const char *,const char *,const char *)=nullptr;
    int (*heartbeat_remove)(void *,int32_t)=nullptr;
    int (*medic_tutorial)(void *,uint32_t command)=nullptr; //1682B0 profile128/129.
    int (*loop_sound)(void *,NativeMissionCharacter &,const void *bank,size_t,const char *event)=nullptr;
    int (*release_helper)(void *,void *)=nullptr;
    NativeCharacterFactions *factions=nullptr;
    int (*target_property)(void *,int32_t,const char *,int32_t *)=nullptr;
    int (*target_scalar_property)(void *,int32_t,const char *,double *)=nullptr;
    int (*target_vector_property)(void *,int32_t,const char *,float[3])=nullptr;
    int (*target_matrix_property)(void *,int32_t,const char *,float[16])=nullptr;
    int (*visible)(void *,NativeMissionCharacter &,int32_t)=nullptr;
    int (*visible_query)(void *,NativeMissionCharacter &,int32_t,float cosine,bool head,const float offset[3],bool characters)=nullptr;
    int (*hazard_segment)(void *,const float from[3],const float to[3])=nullptr;
    NativeCharacterDialogueAudio *dialogue_audio=nullptr;
    NativeDialogueText *dialogue_text=nullptr;
    int (*subtitle)(void *,const NativeDialogueLine &,int32_t,int32_t *)=nullptr;
    int (*controller_registration)(void *,int32_t,bool add)=nullptr;
    int (*maintenance_registration)(void *,int32_t)=nullptr;
    int (*health_policy)(void *,NativeMissionCharacter &,UCCharacterHealth &,uint32_t,float,UCCharacterHealthServices &)=nullptr;
    int (*character_died)(void *,NativeMissionCharacter &,int32_t killer,bool gore)=nullptr;
    int (*held_burn)(void *,NativeMissionCharacter &,bool completed)=nullptr;
    int (*blast_obstruction)(void *,const float from[3],const float to[3],const void *target,int32_t collision_group)=nullptr;
};

struct NativeCharacterNavigationState {int32_t status=0,blocker=0,behaviour=-1,phase=0,node=0;float goal[3]{},waypoint[3]{};bool movement_busy=false;};

class NativeMissionCharacter {
    struct Impl;std::unique_ptr<Impl> p_;
    void presentation_world(float,float out[16])const;
public:
    NativeMissionEntity entity;
    NativeMissionCharacter();~NativeMissionCharacter();
    NativeMissionCharacter(const NativeMissionCharacter&)=delete;
    NativeMissionCharacter&operator=(const NativeMissionCharacter&)=delete;
    int construct(NativeRenderer &,NativeMissionWorld &,uint32_t source,
        const UCArchive *,const uint8_t hash[32],const std::filesystem::path &catalog,
        const UCAnimationBlendMath &,UCGameRandom &,UCVectorRandom &,
        const NativeMissionCharacterServices &);
    int update();int late();int remove();int close_detached();
    int controller_update(bool expensive);uint32_t controller_flags()const;
    bool has_controller()const;bool tasks_empty()const;void request_retirement();
    void draw(NativeRenderer &,const float view_projection[16],float exposure,float fraction,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void collect_shadows(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,class NativeModelShadowCasters &,float fraction=1);
    bool ready()const;uint32_t status()const;
    bool navigation_state(NativeCharacterNavigationState &)const;
    bool order_bounds(UCModelBounds &)const; //Original posed B8FE0 model-local bounds.
    void append_collision(NativeCharacterWorld &);
    int32_t bone_index(const char *)const;
    bool posed_bone_local(int32_t,float matrix[16])const;
    bool posed_bone(const char *,float matrix[16])const;
    bool target_position(float position[3])const;
    bool head_position(float position[3])const;
    int property(const char *,int32_t *)const;
    int scalar_property(const char *,double *)const;
    int bullet_relation(int32_t shooter,uint8_t *handled,uint8_t *result)const;
    int bullet(const UCCharacterBullet &,const UCCharacterBulletImpact &,float damage,const UCCharacterBulletServices &);
    int injure(uint32_t,float,const float point[3],int32_t kind,int32_t owner,uint8_t check_shield,uint8_t *accepted);
    int blast(const UCExplosionBlast &,int32_t sender);
    int set_health(float value,int32_t kind=-1);
    int held_molotov_burn(); //Held11 packet domain; authored animation, PC flame/corpse presentation.
    int damage_target(int32_t);bool damage_friendly(int32_t)const;int32_t root_id()const;
    int health_body(UCCharacterHealthBody,uint8_t *)const;
    NativeCharacterDialogue *dialogue()const;
    const char *audio_bank_path()const;
    const char *heartbeat_resource()const;
    bool dialogue_ready(bool friendly_death_cutscene)const;
    bool dialogue_playing()const;
    bool dialogue_request(const char *,int32_t speaker,bool look,bool force);
    bool dialogue_owner(int32_t &)const;
    void dialogue_cancel();
    // Concrete authored behavior entry; used only after its own real factory
    // registration, never as a substitute for a missing registry target.
    int enter_behaviour(uint32_t source);
};

// Allocates real322D0-derived MoveRun, RandomSkinAndEquipment and DeleteMyself,
// authored362D0 Blazer roots and20BDC0 CharacterSpawnPoint owners.
// owners. They sleep in world update records while the controller owns their
// foreground calls; their exit reactivates the real deferred removal path.
int native_construct_character_behaviour(NativeMissionWorld &,uint32_t source,const UCArchive *archive=nullptr);
