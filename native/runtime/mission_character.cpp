#include "mission_character.h"
#include "model_shadow_casters.h"
#include "audio.h"
#include "character_targets.h"
#include "character_dialogue.h"
#include "character_side_step.h"
#include "cop_attack_cover.h"
#include "character_script_tasks.h"
#include "character_behaviour_script.h"
#include "character_threat.h"
#include "character_attack_position.h"
#include "character_collision.h"
#include "npc_throw_handler.h"
#include "ragdoll_world.h"
#include "order_client.h"
#include "order_manager.h"
extern "C" {
#include "../ragdoll_pose.h"
#include "../point_light_receiver.h"
#include "../blazer_attack.h"
#include "../blazer_combat.h"
#include "../blazer_throw.h"
#include "../brute_chase.h"
#include "../brute_combat.h"
#include "../character_attention.h"
#include "../character_face_target.h"
#include "../character_threat_math.h"
#include "../cop_overlay.h"
#include "../cop_overlay_math.h"
#include "../npc_gun_controller.h"
#include "../cop_attack.h"
#include "../character_interaction.h"
#include "../character_player_range.h"
#include "../character_look.h"
#include "../character_ground.h"
#include "../npc_melee_controller.h"
#include "../medic_root_policy.h"
#include "../medic_update.h"
#include "../medic_heal.h"
#include "../medic_heal_player.h"
#include "../medic_heal_dialogue.h"
#include "../medic_team.h"
#include "../character_root_update.h"
#include "../character_heartbeat.h"
#include "../character_look_driver.h"
#include "../character_follow.h"
#include "../character_step_aside.h"
}
#include <DirectXMath.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <list>
#include <stdexcept>

namespace {
void require(bool b,const char *message){if(!b)throw std::runtime_error(message);}
uint32_t word(const UCLevelBytes &p,size_t n){require(n<p.size/4,"NPC property extent");uint32_t x;std::memcpy(&x,p.data+n*4,4);return x;}
float real(const UCLevelBytes &p,size_t n){uint32_t x=word(p,n);float f;std::memcpy(&f,&x,4);return f;}
bool empty_scripts(const UCLevelBytes&p){return p.size==8&&!word(p,0)&&!word(p,1);}
struct ActiveDescriptor {
    NativeMissionWorld &world;const UCEntityAuthored *saved;
    explicit ActiveDescriptor(NativeMissionWorld &w):world(w),saved(*w.active_descriptor()){}
    ~ActiveDescriptor(){*world.active_descriptor()=saved;}
};
int close_fields(NativeMissionEntity &e){
    if(e.script.close_detached())return 1;
    if(e.aux){if(uc_entity_fresh_aux_destroy(e.aux))return 1;e.aux=nullptr;}
    return 0;
}
int retire_fields(NativeMissionWorld &w,NativeMissionEntity &e,const NativeMissionCharacterServices *services=nullptr){
    if(e.actor.handleBC||e.actor.audioC0)return 1;
    if(e.aux&&uc_entity_fresh_aux_clear(e.aux))return 2;
    e.flags|=0x8100;uint8_t queued=0;
    //20A620 releases216000's actual helper before registry retirement.
    if(e.helper){if(!services||!services->release_helper||services->release_helper(services->owner,e.helper))return 2;e.helper=nullptr;}
    if(uc_entity_retire(&w.retirement(),e.id,&queued)||!queued)return 2;
    if(e.flags&0x1000){if(e.source<0||uint32_t(e.source)>=w.source_count())return 2;auto*s=w.slot(e.source).flags;*s=(*s&~15u)|16u;}
    if(e.identity.listeners[0]){if(uc_entity_listener_list_remove_first(w.listeners(),e.id))return 2;for(auto&listener:e.identity.listeners)listener=nullptr;}
    e.flags|=0x80000;return 0;
}
void request_remove(NativeMissionWorld &w,NativeMissionEntity &e){
    //20A140(1)->207F00. Dynamic roots and authored foregrounds use the same
    //actual update collection. This publishes flags before insertion.
    e.flags|=1;if(e.flags&8){auto&r=*w.context().registration;
        *r.creation={0,1,e.parent};require(!r.insert_record(r.owner,&e.view.target,e.id,1),"NPC dormant record reinsertion");e.flags&=~8u;}
}
struct CharacterState;
//36280 callback has two owners: the requesting behavior and the graph which
//must publish completion/cancellation before the allocation may disappear.
struct CharacterCompletion {
    NativeCharacterAnimationCallback callback;int status=0;bool released=false;
    CharacterCompletion():callback{this,[](void*p,bool cancelled){auto*c=static_cast<CharacterCompletion*>(p);c->status=cancelled?2:1;if(c->released)delete c;}}{}
    void release(){released=true;if(status)delete this;}
};
struct Behaviour {
    enum Kind {Move,Appearance,Delete,Named,NoSecondary,Condition,Speech,CopMove,Loop,Overlay,None,Follow,PlayerRange,FlagCondition,Attack,Interaction,BlazerAttack,MedicCheck,Heartbeat,DisableHeal,Invulnerable,Scripted,GenericExit,Face,PlayAnim,BruteAttack,Restrained,MedicSpeech,Walk,JoinTeam,FollowPlayer,StepAside,HealPlayer,HealDialogue,LineOfSight} kind;
    NativeMissionEntity entity;NativeMissionWorld *world=nullptr;CharacterState *actor=nullptr;
    UCCharacterMove move{};UCCharacterAppearance appearance{};std::vector<int32_t> nodes;bool active=false;
    int32_t named=-1,condition=-1;bool narrow_speech=true;float speech_elapsed=0;Behaviour *child=nullptr,*condition_child=nullptr,*parent=nullptr,*replacement=nullptr,*pending_heal=nullptr;
    UCCharacterLoop loop{};std::shared_ptr<NativeFirstpersonClip> loop_clip;
    std::shared_ptr<UCCharacterClipCompletion> loop_completion;std::vector<uint8_t> loop_bank;
    UCEntitySubscriptions *subscriptions=nullptr;
    UCCopOverlay overlay{};bool compliance_allowed=false,can_heal=true;void *weapon_controller=nullptr;CharacterCompletion *overlay_task=nullptr;
    UCCharacterFollow follow{};NativeCharacterSideStep side_step;int32_t follow_source=-1;float health_snapshot=0;bool friendly_bulletby=false;
    UCCharacterPlayerFollow player_follow{};UCCharacterStepAside step_aside{};UCMedicHealPlayer heal_player{};UCMedicHealDialogue heal_dialogue{0,-1,0,0,0};std::shared_ptr<NativeFirstpersonClip> heal_clips[2];bool child_priority=false;float target_head[3]{};
    float player_range=0;int32_t range_next=-1;
    UCCharacterInteraction interaction{};NativeCharacterBehaviourScript script_runner;CharacterCompletion *turn_completion=nullptr;
    int32_t script_type=0,script_index=-1;uint8_t generic_event=0,animation_end=0;bool hold_last=false,ragdoll_when_tazered=false;
    UCCopAttack attack{};std::unique_ptr<NativeCopAttackCover> cover;int32_t cover_source=-1;bool can_relax=true;
    UCMedicHealCheck heal_check{};UCCharacterHeartbeat heartbeat{};uint8_t invulnerable[4]{};
    UCMedicTeamJoin team_join{};std::string team_text[3];
    NativeCharacterThreat threat;UCBlazerAttackIdle blazer_idle{};std::unique_ptr<NativeCharacterAttackPosition> positioning;
    CharacterCompletion *blazer_tasks[6]{};
    int32_t blazer_stage=-1;float blazer_elapsed=0,blazer_cooldown=0,blazer_melee_elapsed=0,blazer_duck_quiet=0;uint8_t blazer_first=1,blazer_throw_low=1;
    uint8_t blazer_melee=0,blazer_throw=0,blazer_hit=0,blazer_melee_enabled=0,blazer_threw=0;unsigned throw_slot=1;float attack_position[3]{};bool position_ready=false,position_changed=false;
    UCBruteCombat brute{};UCBruteSide brute_side{};int32_t brute_stage=-1;
    uint8_t brute_charge=0,brute_nav_paused=0,brute_throw=0,brute_restock=0;
    explicit Behaviour(Kind k):kind(k){}
    ~Behaviour(){if(turn_completion)turn_completion->release();if(overlay_task)overlay_task->release();for(auto*t:blazer_tasks)if(t)t->release();for(auto t:brute.tasks)if(t)reinterpret_cast<CharacterCompletion*>(t)->release();uc_entity_subscriptions_close(subscriptions);}
    int enter(CharacterState &);void base_exit();void exit();int foreground(bool expensive);UCCharacterLoopServices loop_services();UCCharacterFollowServices follow_services();UCCopAttackServices attack_services();UCCharacterInteractionServices interaction_services();
    void update_condition();
    NativeCharacterSideStep::Services side_services();UCCharacterPlayerFollowServices player_follow_services();UCCharacterStepAsideServices step_aside_services();
    void blazer_transition(int32_t);int blazer_update(bool);NativeCharacterThreatServices threat_services();
    void brute_transition(int32_t);void brute_acquire(bool,bool);void brute_refresh();int brute_update(bool);
    UCBruteChaseServices brute_chase_services();UCBruteCombatServices brute_combat_services();
    UCBlazerCombatServices combat_services();UCBlazerThrowServices throw_services();UCBlazerThrow throw_state();
    int prepare_throw();int request_throw(uint8_t,uint8_t);float reset_throw_cooldown();
    UCCopOverlayCombatServices overlay_services();
    UCMedicHealServices heal_services();UCMedicHealPlayerServices heal_player_services();UCMedicHealDialogueServices heal_dialogue_services();UCCharacterHeartbeatServices heartbeat_services();
    static Behaviour *make_overlay(NativeMissionWorld &,int32_t parent);
    static Behaviour *make_medic_check(NativeMissionWorld &,int32_t parent);
    static Behaviour *make_restrained(CharacterState &,uint8_t on_floor);
    static Behaviour *make_player_follow(CharacterState &);static Behaviour *make_step_aside(Behaviour &);static Behaviour *make_heal_player(Behaviour &);static Behaviour *make_heal_dialogue(Behaviour &);
    static int step(void *p){return static_cast<Behaviour*>(p)->entity.script.update();}
    static int late(void *){return 0;} //218C40: no model/helper in these authored descriptors.
    static int remove(void *p){auto&b=*static_cast<Behaviour*>(p);if(b.active)b.exit();return retire_fields(*b.world,b.entity);}
    static int close(void *p){auto*b=static_cast<Behaviour*>(p);
        if(b->entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(b->world->listeners(),b->entity.id))return 1;for(auto&listener:b->entity.identity.listeners)listener=nullptr;}
        int r=close_fields(b->entity);if(!r)delete b;return r;}
};
struct Root {
    NativeMissionEntity entity;NativeMissionWorld *world=nullptr;CharacterState *actor=nullptr;
    int32_t skin=-1,order_id=-1;float elapsed=100,damage_elapsed=FLT_MAX;bool active=false,damage_enabled=false;
    uint32_t reactions=0,protection=0;float damage=0,timed=0;UCCharacterRootUpdate root_update{-1,-1,0,0,FLT_MAX,nullptr};
    UCCharacterInjury injury{};UCCharacterHealth health_state{};UCCharacterBlastNotice blast_notice{};
    bool medic=false,plain_medic=false,move_to_heal=true,cop=false,brute=false,has_gun=false,compliance=false,enemy=false;int32_t secondary_source=-1;Behaviour *secondary=nullptr,*medic_speech=nullptr;
    float skill=0;std::string emotes;const char *death_cues[4]{};bool named_cues=false;
    std::unique_ptr<UCNpcGunController> gun;
    std::unique_ptr<UCNpcMeleeController> melee;UCMedicRootPolicy medic_policy{};UCMedicUpdate medic_state{};
    UCEntitySubscriptions *subscriptions=nullptr;std::vector<Behaviour*> medic_children;
    std::vector<int32_t> quit_flags;int32_t quit_behaviour=-1;
    ~Root(){uc_entity_subscriptions_close(subscriptions);}
    UCNpcMeleeServices melee_services();UCMedicRootServices medic_services();UCMedicUpdateServices medic_update_services(bool);
    int heals();void common(bool publish=true);int update_health(uint8_t,uint8_t);
    int join_team(const UCMedicTeamJoin&);
    std::unique_ptr<NativeNpcThrowHandler> thrown;
    Root(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
    void exit();void update(bool expensive);
    static int step(void *){return 0;} //20A5F0 empty base, activated only for removal.
    static int late(void *){return 0;} //20A610 empty base.
    static int remove(void *p){auto&r=*static_cast<Root*>(p);if(r.active)r.exit();return retire_fields(*r.world,r.entity);}
    static int close(void *p){auto*r=static_cast<Root*>(p);if(r->entity.identity.listeners[0]){
        if(uc_entity_listener_list_remove_first(r->world->listeners(),r->entity.id))return 1;for(auto&l:r->entity.identity.listeners)l=nullptr;}
        if(r->thrown)r->thrown->close();r->thrown.reset();delete r;return 0;}
};
struct SpawnPoint {
    NativeMissionEntity entity;NativeMissionWorld *world=nullptr;
    SpawnPoint(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
    static int step(void*){return 0;} //20A5F0/11030: source empty base callbacks.
    static int remove(void*p){auto&s=*static_cast<SpawnPoint*>(p);return retire_fields(*s.world,s.entity);}
    static int close(void*p){delete static_cast<SpawnPoint*>(p);return 0;}
};
struct CharacterState {
    NativeMissionCharacter *owner=nullptr;NativeMissionWorld *world=nullptr;NativeRenderer *renderer=nullptr;
    const UCArchive *archive=nullptr;NativeMissionCharacterServices services{};UCGameRandom *random=nullptr;UCVectorRandom *vectors=nullptr;
    UCAnimationBlendMath math{};NativeFirstpersonClips bank;std::unique_ptr<NativeAnimatedModel> model;
    std::string model_name;std::unique_ptr<NativeCharacterCollision> collision;
    NativeAudioBank character_audio;std::string character_audio_path;UCModelEquipment equipment{};std::unique_ptr<NativeCharacterDialogue> dialogue;
    std::unique_ptr<NativeCharacterBody> body;std::unique_ptr<NativeCharacterMovement> movement;
    std::unique_ptr<NativeCharacterNavigator> navigator;UCCharacterLoopController controller{};
    int32_t reaction_suppression=0;uintptr_t actor_support=0;Root *root=nullptr;Behaviour *foreground=nullptr;int32_t starting=-1,root_source=-1,spawn=-1;uint32_t actor_status=0,faction=0;
    bool allow_deactivation=false,snap_to_ground=false,player_pov_death=false,generic_speech=false;unsigned character_class=2,animation_locks=0;int32_t named_character=0;
    NativeCharacterTargetState targets;float threat_speech_time=0;uint32_t bullet_by144=0,bullet_by148=0;std::vector<int32_t> attackers;bool dialogue_look=false,dialogue_face=false;
    UCCharacterLook look{};
    UCCharacterBlastResponse hit_response{};int32_t restraint_cause=0,death_kind=0;float corpse_time=0,corpse_world[16]{},corpse_ground=0,hit_time=0;
    std::unique_ptr<NativeRagdoll> ragdoll;
    std::unique_ptr<float[][16]> corpse_local,corpse_pose;
    std::vector<int32_t> corpse_body;std::vector<uint32_t> corpse_order;
    UCRagdollForce corpse_forces[2]{};bool death_forces=false,death_gore_disabled=false;
    bool burning=false;uint8_t burn_done=0;NativeCharacterAnimationCallback burn_completion{};
    void die();void corpse_step();void ragdoll_begin();void ragdoll_step(float);int restrain(uint8_t,uint8_t);
    std::unique_ptr<UCCharacterAttention> attention;bool attention_active=false,recent_attention=false;float recent_attention_time=0;
    struct DebugState {std::string label;float duration;bool persistent;};std::list<DebugState> debug_states;
    //E3005 owns a separate circular list at130, not generic entity84 aux.
    std::unique_ptr<std::list<uint32_t>> contacts;
    std::unique_ptr<UCEntitySubscriptions,decltype(&uc_entity_subscriptions_close)> subscriptions{nullptr,uc_entity_subscriptions_close};
    NativeCharacterScriptTasks controller_tasks; //Actual controller+10 Run/Wait ownership.
    int32_t head_bone=-1;uint8_t trigger_flags[5]{};UCAnimationEventQueue pending_events{};
    struct Trigger {const char *name;uint8_t *flag;};std::vector<Trigger> triggers;
    float previous[16]{};bool started=false,closed=false,faction_joined=false,controller_registered=false;uint32_t controller_flags=0;int failure=0;
    NativeMissionEntity &entity(){return owner->entity;}
    void load_audio(NativeAudioBank &out,const char *name){size_t i=find_resource(archive,name);BytesOwner b;
        require(i!=SIZE_MAX&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&b.value)==UC_AR_OK,"NPC audio bank resource");out.load(b.value.data,b.value.size);if(&out==&character_audio)character_audio_path=name;}
    void bind_trigger(const char*name,uint8_t*flag){require(name&&flag,"NPC animation trigger storage");*flag=0;triggers.push_back({name,flag});}
    void unbind_trigger(uint8_t*flag){auto i=std::find_if(triggers.begin(),triggers.end(),[flag](const Trigger&t){return t.flag==flag;});require(i!=triggers.end(),"NPC animation trigger removal");triggers.erase(i);}
    const float *live_world(int32_t id)const{auto*t=uc_entity_event_lookup(&world->registry(),id);if(!t)return nullptr;
        const auto&c=world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"NPC live world query");return v->transform;}
    int point(int32_t id,const char*name,float out[3])const{const float*m=live_world(id);if(!m)return 0;
        int found=services.target_vector_property(services.owner,id,name,out);require(found>=0,"NPC vector property query");if(!found)std::copy_n(m+12,3,out);return 1;}
    UCCharacterAttentionServices attention_services(){UCCharacterAttentionServices h{};h.owner=this;h.delta=*world->script_services().delta;
        h.world=[](void*p,int32_t id,float*out){auto&s=*static_cast<CharacterState*>(p);const float*m=s.live_world(id);if(!m)return 0;std::copy_n(m,16,out);return 1;};
        h.head=[](void*p,int32_t id,float*out){return static_cast<CharacterState*>(p)->point(id,"head pos",out);};return h;}
    void attention_begin(){if(attention)return;auto h=attention_services();attention=std::make_unique<UCCharacterAttention>();
        require(!uc_character_attention_init(attention.get(),entity().id,&h),"NPC attention constructor393B0");}
    void attention_step(){const bool previous_active=attention_active;attention_active=false;if(!attention)return;
        auto h=attention_services();require(!uc_character_attention_step(attention.get(),&h),"NPC attention39620");
        int nearby=uc_character_attention_near(attention.get(),&h);require(nearby>=0,"NPC attention range39490");
        if(nearby&&attention->elapsed<1){movement->graph.look_at(attention->point);attention_active=true;}
        else {const int32_t*ids=nullptr;uint32_t count=0;require(!uc_entity_names_view(world->names(),"EXTREMELY_INTERESTING_LOOK",&ids,&count),"NPC attention tag view");
            for(uint32_t i=0;i<count&&!attention_active;i++)if(live_world(ids[i])){int selected=uc_character_attention_select(attention.get(),ids[i],550,&h);require(selected>=0,"NPC attention39690");attention_active=selected!=0;}}
        if(!attention_active&&previous_active)movement->graph.stop_look();
    }
    void head_position(float out[3])const{
        require(head_bone>=0&&model->prepared.ready&&model->prepared.world_valid[head_bone],"NPC posed head position");
        const float *p=model->prepared.world[head_bone]+12,*m=owner->entity.transform;
        //E1CA3 uses the actual model-local posed head, then actor world. Keep
        //the source pairwise float32 ordering instead of a guessed eye height.
        for(unsigned i=0;i<3;i++)out[i]=(m[i]*p[0]+m[4+i]*p[1])+(m[8+i]*p[2]+m[12+i]);
    }
    void idle_look(){
        require(services.target_vector_property,"NPC look vector properties");
        UCCharacterLookServices h{};h.owner=this;h.names=world->names();h.player=services.player_id(services.owner);
        h.actor=[](void*p,int32_t id,UCCharacterLookActor*out,uint8_t*found){auto&s=*static_cast<CharacterState*>(p);
            auto*t=uc_entity_event_lookup(&s.world->registry(),id);*found=t!=nullptr;if(!t)return 0;
            auto&c=s.world->context();auto*v=c.resolve(c.owner,t);if(!v||!v->transform)return 1;std::memcpy(out->world,v->transform,64);
            int r=s.services.target_vector_property(s.services.owner,id,"head pos",out->head);if(r<0)return 1;out->has_head=r!=0;return 0;};
        require(root,"NPC look root properties");int32_t holding=0;if(root->order_id!=-1){int r=services.target_property(services.owner,h.player,"player holding order button",&holding);require(r>=0,"NPC order button property");if(!r)holding=0;}
        float point[3];require(!uc_character_look_aim_point(&look,random,&h,entity().id,world->frame(),*world->script_services().delta,root->order_id,holding!=0,point),"NPC idle look owner");
        movement->graph.aim(point,4);
    }
    NativeCharacterTargetServices target_services(){NativeCharacterTargetServices h{};h.owner=this;
        h.resolve=[](void*p,int32_t id,float out[3]){auto&s=*static_cast<CharacterState*>(p);auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return 0;
            auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v,"NPC target live view");std::memcpy(out,v->transform+12,12);return 1;};
        h.property=[](void*p,int32_t id,const char*name,int32_t*out){auto&s=*static_cast<CharacterState*>(p);require(s.services.target_property,"NPC target property owner");return s.services.target_property(s.services.owner,id,name,out);};
        h.visible=[](void*p,int32_t id){auto&s=*static_cast<CharacterState*>(p);require(s.services.visible,"NPC target visibility owner");return s.services.visible(s.services.owner,*s.owner,id);};
        h.player=[](void*p){auto&s=*static_cast<CharacterState*>(p);return s.services.player_id(s.services.owner);};
        h.dialogue=[](void*p,uint32_t code,unsigned priority)->int{auto&s=*static_cast<CharacterState*>(p);require(s.dialogue!=nullptr,"NPC numeric dialogue owner");s.dialogue->say((int32_t)code,(int32_t)priority);return 0;};return h;
    }
    NativeCharacterTargetActor target_actor()const{
        NativeCharacterTargetActor a;a.id=owner->entity.id;a.faction=faction_joined?(int32_t)faction:-1;
        std::memcpy(a.position,owner->entity.transform+12,12);std::memcpy(a.forward,owner->entity.transform+8,12);return a;
    }
    int32_t current_target(){require(services.factions,"NPC faction query owner");return native_character_current_target(targets,target_actor(),*services.factions,target_services());}
    bool acquire(bool prefer,bool visible){require(services.factions,"NPC faction query owner");auto h=target_services();auto a=target_actor();
        return native_character_acquire_target(targets,a,*services.factions,h,prefer,visible);
    }
    Behaviour *enter_child(int32_t source){
        require(source>=0&&uint32_t(source)<world->source_count(),"NPC child source");ActiveDescriptor restore(*world);int32_t id=-1;
        if(world->ensure((uint32_t)source,&id))throw std::runtime_error("NPC child factory source "+std::to_string(source)+" "+world->failure_class()+" phase "+world->failure_phase());auto*b=world->binding((uint32_t)source);
        require(b&&b->step==Behaviour::step&&*b->view->id==id,"NPC child registry type");auto*result=static_cast<Behaviour*>(b->owner);
        require(!result->enter(*this),"NPC child entry");return result;
    }
    static int ray(void*p,const float*a,const float*b,uint32_t region,uint32_t shape,uint32_t type,uint8_t*hit,int32_t*entity){
        auto&s=*static_cast<CharacterState*>(p);require(!s.services.refresh_collision(s.services.owner),"NPC collision query refresh");
        return NativeCharacterWorld::ray(s.services.collision,a,b,region,shape,type,hit,entity);}
    static uint8_t scripted(void*p,int32_t target){auto&s=*static_cast<CharacterState*>(p);require(s.services.doors,"NPC door property owner");return s.services.doors->check(target,s.entity().id,s.entity().transform+12);}
    static void scripted_ray(void*p,const float*a,const float*b){auto&s=*static_cast<CharacterState*>(p);require(s.services.doors,"NPC door cast owner");s.services.doors->ahead(s.entity().id,a,b);}
    void transition(int32_t source){
        if(foreground)foreground->exit();
        if(source==-1)return;
        require(source>=0&&uint32_t(source)<world->source_count(),"NPC next behavior source");
        ActiveDescriptor restore(*world);int32_t id=-1;int result=world->ensure(source,&id);
        if(result)throw std::runtime_error("NPC behavior factory source "+std::to_string(source)+" "+world->metadata(source).definition->class_name+" code "+std::to_string(result));
        auto*b=world->binding(source);require(b&&b->step==Behaviour::step&&*b->view->id==id,"NPC behavior registry type");
        foreground=static_cast<Behaviour*>(b->owner);require(!foreground->enter(*this),"NPC behavior entry");
    }
    void interrupt(Behaviour *next){
        require(next&&!next->active,"NPC temporary foreground ownership");
        //32D10/324D0 retain the controller foreground, install its E0 child,
        //and invoke its actual A0/A4 suspension pair. These admitted source
        //classes have no-op pairs except FollowPlayer's idle timer reset.
        if(foreground){auto*suspended=foreground;
            require(suspended->kind==Behaviour::Loop||suspended->kind==Behaviour::FollowPlayer||suspended->kind==Behaviour::Move||suspended->kind==Behaviour::Walk||
                suspended->kind==Behaviour::Scripted||suspended->kind==Behaviour::Follow||suspended->kind==Behaviour::Face||suspended->kind==Behaviour::PlayAnim||
                suspended->kind==Behaviour::None||suspended->kind==Behaviour::HealDialogue,"NPC foreground suspension class is unqualified");
            if(suspended->replacement&&suspended->replacement->active)suspended->replacement->exit();
            if(suspended->kind==Behaviour::FollowPlayer)suspended->player_follow.idle_time=0;
            if(!(suspended->entity.flags&1)){suspended->replacement=next;next->parent=suspended;}
        }else foreground=next;
        require(!next->enter(*this),"NPC temporary foreground entry32470");
    }
    UCCharacterMoveServices move_services(){UCCharacterMoveServices h{};h.owner=this;
        h.reset=[](void*p,UCCharacterLoopController*c){auto&s=*static_cast<CharacterState*>(p);require(c==&s.controller,"NPC reset controller");s.navigator->stop();};
        h.resolve=[](void*p,int32_t packed)->const float*{auto&s=*static_cast<CharacterState*>(p);const UCLevelNode*n=nullptr;
            auto r=uc_level_node_find(s.world->script_services().logic,uint32_t(packed),&n);require(r==UC_LEVEL_OK||r==UC_LEVEL_NOT_FOUND,"NPC authored node lookup");return n?n->position:nullptr;};
        h.target=[](void*p,UCCharacterLoopController*c,const float*t){auto&s=*static_cast<CharacterState*>(p);require(c==&s.controller,"NPC target controller");s.navigator->start(t,s.services.navigation->query(s.entity().id));};
        h.run=[](void*p,UCCharacterLoopController*c){auto&s=*static_cast<CharacterState*>(p);require(c==&s.controller,"NPC run controller");return s.run();};
        h.transition=[](void*p,UCCharacterMove*,int32_t source){static_cast<CharacterState*>(p)->transition(source);};
        h.actor_world=[](void*p,void*a)->const float*{auto&s=*static_cast<CharacterState*>(p);require(a==&s,"NPC actor identity");return s.entity().transform;};return h;
    }
    int run_flags(const uint8_t flags[5]){UCCharacterRunServices h{};h.owner=this;
        h.goal=[](void*p,float*out){auto&s=*static_cast<CharacterState*>(p);std::memcpy(out,s.navigator->goal(s.entity().transform+12),12);};
        h.actor_position=[](void*p)->const float*{return static_cast<CharacterState*>(p)->entity().transform+12;};
        h.player_id=[](void*p){auto&s=*static_cast<CharacterState*>(p);require(s.services.player_id,"NPC player query");return s.services.player_id(s.services.owner);};
        h.position=[](void*p,int32_t id)->const float*{auto&s=*static_cast<CharacterState*>(p);auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return nullptr;auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v,"NPC live target view");return v->transform+12;};
        h.stop=[](void*p){static_cast<CharacterState*>(p)->movement->stop();};
        h.status=[](void*p){return static_cast<CharacterState*>(p)->navigator->status();};
        h.emote_if_blocked=[](void*p)->int{auto&s=*static_cast<CharacterState*>(p);
            //33196 queries ROOT1C, default false. Plain Medic170C40 falls to
            //36590; only teamMedic16946F/Cop1BDD7F publish order8C!=-1.
            return s.root&&(s.root->cop||(s.root->medic&&!s.root->plain_medic))&&s.root->order_id!=-1;};
        h.blocker=[](void*p){return static_cast<CharacterState*>(p)->navigator->blocker();};
        h.emote=[](void*p,int32_t code,int32_t priority){auto&s=*static_cast<CharacterState*>(p);require(s.dialogue!=nullptr,"NPC blocked dialogue owner");s.dialogue->say(code,priority);};
        h.waypoint=[](void*p,float*out){auto&s=*static_cast<CharacterState*>(p);std::memcpy(out,s.navigator->waypoint(s.entity().transform+12),12);};
        h.request=[](void*p,const float*t,int32_t mode){auto&s=*static_cast<CharacterState*>(p);NativeCharacterMoveTarget target;std::memcpy(target.position,t,12);target.mode=mode;s.movement->request(target);};
        h.clear_heading=[](void*p){static_cast<CharacterState*>(p)->movement->clear_heading();};
        int32_t result;require(!uc_character_run(&h,flags,&result),"NPC controller run");return result;
    }
    int run(bool cop_move=false){const uint8_t flags[5]={1,uint8_t(!cop_move),0,uint8_t(!cop_move),1};return run_flags(flags);}
    void target_cue(bool shield){UCBlazerTargetCue value{threat_speech_time,targets.current};UCBlazerTargetCueServices h{};h.owner=this;
        h.weapon_ready=[](void*p)->uint8_t{auto&s=*static_cast<CharacterState*>(p);require(s.root&&s.root->thrown,"Blazer source weapon1C");return 1;}; //44D57C+1C=2C3360.
        h.valid=[](void*p,int32_t id)->uint8_t{auto&s=*static_cast<CharacterState*>(p);return native_character_target_valid(s.targets,s.target_actor(),*s.services.factions,s.target_services(),id);};
        h.player=[](void*p){auto&s=*static_cast<CharacterState*>(p);return s.services.player_id(s.services.owner);};
        h.say=[](void*p,int32_t code,int32_t priority)->uint8_t{return static_cast<CharacterState*>(p)->dialogue->say(code,priority);};
        h.pair=[](void*p,int32_t a,int32_t b,float chance,int32_t priority)->uint8_t{return static_cast<CharacterState*>(p)->dialogue->say_pair(a,b,chance,priority);};
        h.health=[](void*p)->double{return static_cast<CharacterState*>(p)->entity().actor.health9C;};
        h.maximum=[](void*p)->double{return static_cast<CharacterState*>(p)->entity().actor.maximumA0;};
        h.random=[](void*p,float lo,float hi){auto&s=*static_cast<CharacterState*>(p);int32_t value;require(!uc_game_random_next(s.random,&value),"NPC target cue random");return(double(value)*(double(hi)-lo))*0x1p-31+lo;};
        require(!uc_blazer_target_cue(&value,&h,shield),"NPC controller threat cue341A0");targets.current=value.target;threat_speech_time=value.remaining;
    }
    void create_body();void enter_skin(int32_t);void start();void release();
};
Behaviour *Behaviour::make_overlay(NativeMissionWorld&w,int32_t parent_id){
    require(uc_entity_event_lookup(&w.registry(),parent_id)!=nullptr,"Cop overlay dynamic parent");auto*b=new Behaviour(Overlay);b->world=&w;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("Cop overlay binding");}
    auto context=w.context();const void*definition="CopOverlayShootBehaviour";context.definition=&definition;*context.registration->creation={0,1,parent_id};
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"Cop overlay original parent placement");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"Cop overlay dormant record");
    require(!uc_entity_subscriptions_create(b,&b->subscriptions),"Cop overlay embedded listener");uc_cop_overlay_init(&b->overlay,1);return b;
}
Behaviour *Behaviour::make_medic_check(NativeMissionWorld&w,int32_t parent_id){
    auto*b=new Behaviour(MedicCheck);b->world=&w;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("Medic secondary binding");}
    auto context=w.context();const void*definition="CheckHealPlayer";context.definition=&definition;*context.registration->creation={0,1,parent_id};
    //1699DE builds an empty descriptor;366E0->20A230 sets mode1 through
    //1F8E30 before207CD0 constructs this child of the base-only Medic root.
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"Medic secondary parent descriptor");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"Medic secondary dormant record");return b;
}
Behaviour *Behaviour::make_restrained(CharacterState&s,uint8_t floor){
    auto&w=*s.world;auto*b=new Behaviour(Restrained);b->world=&w;b->hold_last=floor!=0;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("RestrainedBehaviour binding");}
    auto context=w.context();const void*definition="RestrainedBehaviour";context.definition=&definition;*context.registration->creation={0,1,s.entity().id};
    //DF3D0 snapshots actor10 into its descriptor;20A260 chooses mode5.
    require(!uc_entity_fresh_construct_matrix(&context,&b->entity.view,s.entity().transform),"RestrainedBehaviour source matrix placement");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"RestrainedBehaviour dormant record");return b;
}
Behaviour *Behaviour::make_player_follow(CharacterState&s){
    auto&w=*s.world;auto*b=new Behaviour(FollowPlayer);b->world=&w;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("FollowPlayerBehaviour binding");}
    require(s.root,"FollowPlayerBehaviour source root parent");auto context=w.context();const void*definition="FollowPlayerBehaviour";context.definition=&definition;*context.registration->creation={0,1,s.root->entity.id};
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"FollowPlayerBehaviour source parent");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"FollowPlayerBehaviour dormant record");
    //16A3C4 supplies Side step1,Wait at start0,Check side step1.194400
    //initializes the remaining suffix and registers the force-step event76.
    b->player_follow.side_step=b->player_follow.check_side_step=1;
    require(!uc_entity_subscriptions_create(b,&b->subscriptions),"FollowPlayerBehaviour listener owner");int inserted=0;
    require(!uc_entity_listener_register(&w.registry(),b->entity.id,b->entity.id,uc_entity_subscriptions_listener(b->subscriptions),w.listeners(),&inserted)&&inserted,"FollowPlayerBehaviour listener registration");
    const UCEntityEventSubscription force{76,[](void*p,const void*,uint32_t*){static_cast<Behaviour*>(p)->player_follow.forced=1;return 0;}};
    require(!uc_entity_subscriptions_append(b->subscriptions,&force),"FollowPlayerBehaviour force subscription");return b;
}
Behaviour *Behaviour::make_step_aside(Behaviour&parent){
    auto&s=*parent.actor;auto&w=*s.world;auto*b=new Behaviour(StepAside);b->world=&w;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("StepAsideBehaviour binding");}
    auto context=w.context();const void*definition="StepAsideBehaviour";context.definition=&definition;*context.registration->creation={0,1,parent.entity.id};
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"StepAsideBehaviour source parent");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"StepAsideBehaviour dormant record");
    b->step_aside.distance=120;b->step_aside.target=s.services.player_id(s.services.owner);return b;
}
Behaviour *Behaviour::make_heal_player(Behaviour&parent){
    auto&s=*parent.actor;auto&w=*s.world;auto*b=new Behaviour(HealPlayer);b->world=&w;b->heal_player.pillbox=-1;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("HealPlayer binding");}
    auto context=w.context();const void*definition="HealPlayer";context.definition=&definition;*context.registration->creation={0,1,parent.entity.id};
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"HealPlayer source parent");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"HealPlayer dormant record");return b;
}
Behaviour *Behaviour::make_heal_dialogue(Behaviour&parent){
    auto&s=*parent.actor;auto&w=*s.world;auto*b=new Behaviour(HealDialogue);b->world=&w;
    if(w.bind_dynamic({&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,nullptr,Behaviour::remove,Behaviour::close})){delete b;throw std::runtime_error("MedicHealPlayer binding");}
    auto context=w.context();const void*definition="MedicHealPlayer";context.definition=&definition;*context.registration->creation={0,1,parent.entity.id};
    require(!uc_entity_fresh_construct_parent(&context,&b->entity.view),"MedicHealPlayer source parent");
    require(!w.retirement().remove_update_record(w.retirement().owner,b->entity.id),"MedicHealPlayer dormant record");return b;
}
//184580 subclass of the already source-qualified40F20 real registry client.
struct HealClient {
    NativeMissionWorld *world=nullptr;NativeOrderClient *client=nullptr;UCMedicHealOrder order{};
    static int create(Behaviour&b,int32_t*out){auto*s=new HealClient;s->world=b.world;
        NativeOrderClientCallbacks h{};h.owner=s;
        h.accept=[](void*,int32_t,const float*,uint8_t*out){*out=1;return 0;}; //15DFC0.
        h.attached=[](void*p){auto&s=*static_cast<HealClient*>(p);const NativeOrderAction actions[]={
            {5,"heal player yes",2,"Hud\\HUD_Icon_Heal_Yes.shader",1},
            {6,"heal player no",3,"Hud\\HUD_Icon_Heal_No.shader",1}};
            for(const auto&a:actions)if(int r=s.client->advertise(a))return r;return 0;};
        h.choose=[](void*p,int32_t command){auto&s=*static_cast<HealClient*>(p);return uc_medic_heal_order_choose(&s.order,command,&s,[](void*p,int32_t n){return static_cast<HealClient*>(p)->client->complete(n);});};
        h.completed=[](void*p){static_cast<HealClient*>(p)->order.command=0;return 0;}; //112020.
        h.manager_position=[](void*p,int32_t id,float*out,uint8_t*found){auto&s=*static_cast<HealClient*>(p);auto*m=NativeOrderManager::find(*s.world,id);*found=0;return m?m->vector("Position",out,found):0;};
        h.integer=[](void*p,const char*n,int32_t*out,uint8_t*found){return uc_medic_heal_order_integer(&static_cast<HealClient*>(p)->order,n,out,found);};
        h.vector=[](void*p,const char*n,float*out,uint8_t*found){auto&s=*static_cast<HealClient*>(p);*found=0;if(std::strcmp(n,"Position")&&std::strcmp(n,"Player Look Point"))return 0;
            auto*m=NativeOrderManager::find(*s.world,s.client->manager());return m?m->vector(n,out,found):0;};
        h.closed=[](void*p){delete static_cast<HealClient*>(p);};
        int result=NativeOrderClient::create(*b.world,b.entity.id,"HealOrderClient",h,&s->client);if(result){if(!s->client)delete s;return result;}*out=s->client->entity.id;return 0;
    }
};
UCMedicHealDialogueServices Behaviour::heal_dialogue_services(){UCMedicHealDialogueServices h{};h.owner=this;
    h.completion=[](void*p,int32_t*out){auto*c=static_cast<Behaviour*>(p)->turn_completion;*out=c?c->status:-1;return 0;};
    h.boolean=[](void*,const char*n,uint8_t fallback,uint8_t*out){if(std::strcmp(n,"MedicAutoHeal is healing")&&std::strcmp(n,"MedicAutoHeal is transitioning"))return 1;*out=fallback;return 0;}; //No admitted MedicAutoHeal foreground.
    h.loop=[](void*)->int{throw std::runtime_error("MedicAutoHeal dialogue animation owner is unconnected");};
    h.lock=[](void*p,uint8_t value){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;uint32_t sender=uint32_t(b.entity.id);uint8_t handled=0;return uc_entity_event_send(&b.world->registry(),s.services.player_id(s.services.owner),149,&value,1,&sender,&handled);};
    h.head=[](void*p,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.target_vector_property&&s.services.target_vector_property(s.services.owner,s.services.player_id(s.services.owner),"head pos",out)>=0?0:1;};
    h.look=[](void*p,const float*t,uint8_t head){auto&g=static_cast<Behaviour*>(p)->actor->movement->graph;if(head)g.look_at(t);else g.aim(t,2);return 0;};
    h.say=[](void*p,int32_t n,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(n,priority);return 0;};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();return 0;};
    h.stop_look=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_look();return 0;};
    h.create_client=[](void*p,int32_t*out){return HealClient::create(*static_cast<Behaviour*>(p),out);};
    h.remove_client=[](void*p,int32_t id){auto&b=*static_cast<Behaviour*>(p);auto*c=NativeOrderClient::find(*b.world,id);return c?c->remove():0;};
    h.command=[](void*p,int32_t id,int32_t*out){auto&b=*static_cast<Behaviour*>(p);auto*c=NativeOrderClient::find(*b.world,id);*out=0;uint8_t found=0;return c?c->integer("current order",out,&found):0;};
    h.nearby=[](void*p,float range,float angle,uint8_t*out){auto&b=*static_cast<Behaviour*>(p);auto services=b.heal_services();int result=services.nearby(p,range,angle);if(result<0)return 1;*out=result!=0;return 0;};
    h.heal_child=[](void*p){auto&b=*static_cast<Behaviour*>(p);if(b.child&&b.child->active)b.child->exit();b.child=make_heal_player(b);b.child->parent=&b;b.child_priority=false;return b.child->enter(*b.actor);};
    h.child=[](void*p,int32_t*out){auto&b=*static_cast<Behaviour*>(p);*out=b.child?b.child->entity.id:-1;return 0;};
    h.leave=[](void*p){static_cast<Behaviour*>(p)->exit();return 0;};
    h.base_exit=[](void*p){static_cast<Behaviour*>(p)->base_exit();return 0;};
    h.base=[](void*p,float){auto&b=*static_cast<Behaviour*>(p);std::copy_n(b.actor->entity().transform,16,b.entity.transform);return 0;};return h;
}
UCMedicHealPlayerServices Behaviour::heal_player_services(){UCMedicHealPlayerServices h{};h.owner=this;
    h.player=[](void*p,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;const float*m=s.live_world(s.services.player_id(s.services.owner));if(!m)return 1;std::copy_n(m+12,3,out);return 0;};
    h.actor=[](void*p,float*out){std::copy_n(static_cast<Behaviour*>(p)->actor->entity().transform+12,3,out);return 0;};
    h.boolean=[](void*p,uint8_t root,const char*n,uint8_t fallback,uint8_t*out){auto&s=*static_cast<Behaviour*>(p)->actor;
        if(root){if(std::strcmp(n,"move to heal player")||!s.root||!s.root->medic)return 1;*out=s.root->move_to_heal;}
        else{if(std::strcmp(n,"MedicAutoHeal is transitioning")&&std::strcmp(n,"MedicAutoHeal is healing"))return 1;
            //No admitted foreground is MedicAutoHeal; source property lookup
            //therefore takes its explicit fallback. That owner needs its own state.
            *out=fallback;}return 0;};
    h.completion=[](void*p,int32_t*out){auto*c=static_cast<Behaviour*>(p)->turn_completion;*out=c?c->status:3;return 0;};
    h.say=[](void*p,int32_t n,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(n,priority);return 0;};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();return 0;};
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();return 0;};
    h.clip=[](void*p,uint8_t recovery){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;
        auto&asset=b.heal_clips[recovery?1:0];if(!asset){const char*name=recovery?"Assets_CharacterAnimation/CharacterSpecific/CCA_Paramedic/CCA_Paramedic_Anim_HealRecover.kfa":"Assets_CharacterAnimation/CharacterSpecific/CCA_Paramedic/CCA_Paramedic_Anim_HealPlayer.kfa";
            size_t index=find_resource(s.archive,name);BytesOwner bytes;if(index==SIZE_MAX||uc_archive_read(s.archive,index,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)return 1;
            asset=std::make_shared<NativeFirstpersonClip>();asset->bytes.assign(bytes.value.data,bytes.value.data+bytes.value.size);UCAnimationClip clip{};
            std::unique_ptr<UCAnimationClip,decltype(&uc_animation_clip_free)> cleanup(&clip,uc_animation_clip_free);
            if(uc_animation_clip_load(asset->bytes.data(),asset->bytes.size(),&clip)!=UC_ANIMATION_OK||uc_animation_annotations_load(&clip,&asset->annotations)!=UC_ANIMATION_OK)return 1;}
        if(b.turn_completion)b.turn_completion->release();b.turn_completion=new CharacterCompletion;
        s.movement->graph.queue_clip(asset,&b.turn_completion->callback,false,false);return 0;};
    h.nav_target=[](void*p,const float*out){auto&s=*static_cast<Behaviour*>(p)->actor;s.navigator->retarget(out,s.services.navigation->query(s.entity().id));return 0;};
    h.nav_stop=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();return 0;};
    h.run=[](void*p,int32_t*out){const uint8_t flags[5]={1,1,0,1,0};*out=static_cast<Behaviour*>(p)->actor->run_flags(flags);return 0;};
    h.cancel_speech=[](void*p){static_cast<Behaviour*>(p)->actor->dialogue->cancel();return 0;};
    h.aim=[](void*p,const float*out,uint32_t mode){auto&b=*static_cast<Behaviour*>(p);if(b.turn_completion)b.turn_completion->release();b.turn_completion=new CharacterCompletion;b.actor->movement->graph.aim(out,mode,&b.turn_completion->callback);return 0;};
    h.event=[](void*p,uint32_t event){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;uint32_t payload=0,sender=uint32_t(b.entity.id);uint8_t handled=0;return uc_entity_event_send(&b.world->registry(),s.services.player_id(s.services.owner),event,&payload,4,&sender,&handled);};
    h.random_pair=[](void*p,int32_t*out){return uc_game_random_range(static_cast<Behaviour*>(p)->actor->random,0,2,out);};
    h.heals=[](void*p,int32_t*out){auto&s=*static_cast<Behaviour*>(p)->actor;if(!s.root)return 1;*out=s.root->heals();return 0;};
    h.bone_world=[](void*p,float*bone,float*world){auto&s=*static_cast<Behaviour*>(p)->actor;if(!s.owner->posed_bone_local(s.owner->bone_index("Bip01 L Hand"),bone))return 1;std::copy_n(s.entity().transform,16,world);return 0;};
    h.pillbox=[](void*p,const float*m,int32_t*out){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.medic_pillbox?s.services.medic_pillbox(s.services.owner,*s.world,*s.owner,m,out):1;};
    h.delivered=[](void*p){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;if(!s.root)return 1;const int32_t ids[]={s.services.player_id(s.services.owner),s.root->entity.id,s.foreground?s.foreground->entity.id:-1};
        float payload=100;uint32_t sender=uint32_t(b.entity.id);for(int32_t id:ids){uint8_t handled=0;if(uc_entity_event_send(&b.world->registry(),id,147,&payload,4,&sender,&handled))return 1;}return 0;};
    h.remove=[](void*p,int32_t id){if(id==-1)return 0;auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.melee_weapon_flags?s.services.melee_weapon_flags(s.services.owner,id,1):1;};
    h.leave=[](void*p){static_cast<Behaviour*>(p)->exit();return 0;};
    h.base=[](void*p,float){auto&b=*static_cast<Behaviour*>(p);std::copy_n(b.actor->entity().transform,16,b.entity.transform);return 0;};return h;
}
int CharacterState::restrain(uint8_t floor,uint8_t player){
    if(!started||!root||!movement||!services.ragdoll_world||!services.ragdoll_asset)return 1;
    struct Context {CharacterState&s;Behaviour*created=nullptr;} context{*this};
    UCCharacterRestraintServices h{};h.owner=&context;
    h.create=[](void*p,uint8_t floor,int32_t*out){auto&c=*static_cast<Context*>(p);c.created=Behaviour::make_restrained(c.s,floor);*out=c.created->entity.id;return 0;};
    h.activate=[](void*p,int32_t id){auto&c=*static_cast<Context*>(p);require(c.created&&c.created->entity.id==id,"RestrainedBehaviour controller identity");
        if(c.s.foreground)c.s.foreground->exit();c.s.foreground=c.created;return c.created->enter(c.s);};
    return uc_character_restrain(&actor_status,&restraint_cause,floor,player,&h);
}
UCMedicHealServices Behaviour::heal_services(){UCMedicHealServices h{};h.owner=this;
    h.delta=[](void*p)->double{return *static_cast<Behaviour*>(p)->world->script_services().delta;};
    h.exists=[](void*p,int32_t id){auto&b=*static_cast<Behaviour*>(p);return uc_entity_event_lookup(&b.world->registry(),id)?1:0;};
    h.heals=[](void*p,int32_t*out){*out=static_cast<Behaviour*>(p)->actor->root->heals();return 0;};
    h.nearby=[](void*p,float range,float angle){auto&s=*static_cast<Behaviour*>(p)->actor;const float*player=s.live_world(s.services.player_id(s.services.owner));if(!player)return 0;
        uint8_t answer=0;require(!uc_character_player_range(s.body?s.body->world+12:s.entity().transform+12,player+12,range,angle,&s,
            [](void*p,float*m,uint8_t*found){auto&s=*static_cast<CharacterState*>(p);require(s.services.target_matrix_property,"Medic player Look point owner");int r=s.services.target_matrix_property(s.services.owner,s.services.player_id(s.services.owner),"Look point",m);if(r<0)return 1;*found=r!=0;return 0;},&answer),"Medic player range33B60");return int(answer);};
    h.visible=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;const float offset[3]{};require(s.services.visible_query,"Medic visibility owner");return s.services.visible_query(s.services.owner,*s.owner,s.services.player_id(s.services.owner),-1,true,offset,false);};
    h.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.live_world(s.services.player_id(s.services.owner))?1:0;};
    h.health=[](void*p)->double{auto&s=*static_cast<Behaviour*>(p)->actor;double value=0;require(s.services.target_scalar_property&&s.services.target_scalar_property(s.services.owner,s.services.player_id(s.services.owner),"health",&value)==1,"Medic actual player health218120");return value;};
    h.percent_health=[](void*p)->double{auto&s=*static_cast<Behaviour*>(p)->actor;double value=100;require(s.services.target_scalar_property,"Medic percent health property owner");int found=s.services.target_scalar_property(s.services.owner,s.services.player_id(s.services.owner),"percent health remaining",&value);require(found>=0,"Medic player percent health");return found?value:100.;};
    h.property=[](void*p,unsigned scope,const char*name,uint8_t fallback){auto&s=*static_cast<Behaviour*>(p)->actor;
        if(scope==0){require(!std::strcmp(name,"allow heal"),"Medic root Boolean property");return int(s.root->medic_state.can_heal);}
        if(scope==2){require(!std::strcmp(name,"can heal"),"Medic foreground Boolean property");return s.foreground&&s.foreground->kind==Loop?int(s.foreground->can_heal):int(fallback);}
        require(scope==1,"Medic Boolean property scope");int32_t value=0;int found=s.services.target_property(s.services.owner,s.services.player_id(s.services.owner),name,&value);require(found>=0,"Medic player Boolean property");return found?int(value!=0):int(fallback);};
    h.dialogue_busy=[](void*p){return static_cast<Behaviour*>(p)->actor->dialogue->playing()?1:0;};
    h.spawn=[](void*p,const char*n,int32_t*out){auto&b=*static_cast<Behaviour*>(p);if(std::strcmp(n,"MedicHealPlayer")||b.pending_heal)return 1;b.pending_heal=make_heal_dialogue(b);*out=b.pending_heal->entity.id;return 0;};
    h.queue=[](void*p,int32_t id){auto&b=*static_cast<Behaviour*>(p);if(!b.pending_heal||b.pending_heal->entity.id!=id)return 1;auto*next=b.pending_heal;b.pending_heal=nullptr;b.actor->interrupt(next);return 0;};
    h.emote=[](void*p,uint32_t code,uint32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say((int32_t)code,(int32_t)priority);return 0;};
    h.base=[](void*p,float){auto&b=*static_cast<Behaviour*>(p);std::copy_n(b.actor->entity().transform,16,b.entity.transform);return 0;};return h;
}
UCCharacterLoopServices Behaviour::loop_services(){UCCharacterLoopServices h{};h.owner=this;
    h.sound=[](void*p,void*a,void*bank,int start){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;
        require(a==&s&&bank==b.loop_bank.data()&&s.services.loop_sound,"NPC loop sound owner");
        require(!s.services.loop_sound(s.services.owner,*s.owner,bank,b.loop_bank.size(),start?"Start":"Stop"),"NPC loop sound request");};
    h.attach_condition=[](void*p,UCCharacterLoop*,int32_t source){auto&b=*static_cast<Behaviour*>(p);b.child=b.actor->enter_child(source);b.child->parent=&b;};
    h.collision=[](void*p,void*a,int release){auto&s=*static_cast<Behaviour*>(p)->actor;require(a==&s,"NPC animation override actor");
        if(release){if(s.animation_locks)--s.animation_locks;}else ++s.animation_locks;}; //DDD70/DDD80 reference counter; does not remove collision.
    h.animation_owner=[](void*p,void*a)->void*{auto&b=*static_cast<Behaviour*>(p);require(a==b.actor,"NPC loop animation actor");return &b.actor->movement->graph;};
    h.animation_begin=[](void*p,void*owner,void*clip,void**out){auto&b=*static_cast<Behaviour*>(p);
        require(owner==&b.actor->movement->graph&&clip==b.loop_clip.get(),"NPC loop animation resource owner");
        b.loop_completion=b.actor->movement->graph.queue_loop(b.loop_clip);*out=b.loop_completion.get();};
    h.animation_cycle=[](void*p,void*animation){auto&b=*static_cast<Behaviour*>(p);require(animation==b.loop_completion.get(),"NPC loop completion lifetime");
        int crossed=b.loop_completion->crossed;b.loop_completion->crossed=0;return crossed;}; //2B2670
    h.condition=[](void*,void*)->int32_t{throw std::runtime_error("NPC replacement animation condition is unconnected");};
    h.interrupt=[](void*p,UCCharacterLoopController*c,int32_t count,const int32_t*flags){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;
        require(c==&s.controller,"NPC loop interrupt controller");const auto&services=s.world->script_services();
        int result=uc_character_loop_flags(services.flags,services.flag_count,count,flags);require(result>=0,"NPC loop authored interrupt flags");return result;};
    h.transition=[](void*p,UCCharacterLoop*,int32_t source){auto&b=*static_cast<Behaviour*>(p);if(source==-1)return 0;b.actor->transition(source);return 1;};
    h.base_exit=[](void*p,UCCharacterLoop*){static_cast<Behaviour*>(p)->base_exit();};
    h.delta=[](void*p){return *static_cast<Behaviour*>(p)->world->script_services().delta;};
    h.actor_world=[](void*p,void*a)->const float*{auto&b=*static_cast<Behaviour*>(p);require(a==b.actor,"NPC loop actor world");return b.actor->entity().transform;};return h;
}
UCCharacterFollowServices Behaviour::follow_services(){UCCharacterFollowServices h{};h.owner=this;
    h.position=[](void*p,int32_t id)->const float*{auto&s=*static_cast<Behaviour*>(p)->actor;auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return nullptr;
        auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"Follow target transform");return v->transform+12;};
    h.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->actor->transition(next);};
    h.retarget=[](void*p,const float*t){auto&s=*static_cast<Behaviour*>(p)->actor;s.navigator->retarget(t,s.services.navigation->query(s.entity().id));};
    h.status=[](void*p){return static_cast<Behaviour*>(p)->actor->navigator->status();};
    h.waypoint=[](void*p,float*t){auto&s=*static_cast<Behaviour*>(p)->actor;std::memcpy(t,s.navigator->waypoint(s.entity().transform+12),12);};
    h.near_goal=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->navigator->near_goal();};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.move=[](void*p,const float*t,int32_t mode,uint8_t close){auto&s=*static_cast<Behaviour*>(p)->actor;NativeCharacterMoveTarget target;
        std::copy_n(t,3,target.position);target.mode=mode;target.near_target=close;s.movement->request(target);};
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    h.aim=[](void*p,const float*t){static_cast<Behaviour*>(p)->actor->movement->graph.aim(t,4);};
    h.property=[](void*p,const char*name,uint8_t fallback){auto&b=*static_cast<Behaviour*>(p);
        require(!std::strcmp(name,"should duck")||!std::strcmp(name,"should sidestep"),"Follow secondary property");
        require(!b.child||b.child->kind==Condition||b.child->kind==PlayerRange||b.child->kind==LineOfSight,"Follow secondary property owner");return fallback;};
    h.side_step=[](void*p){auto&b=*static_cast<Behaviour*>(p);b.side_step.update(b.side_services());};
    h.current_target=[](void*p){return static_cast<Behaviour*>(p)->actor->current_target();};
    h.idle_look=[](void*p){static_cast<Behaviour*>(p)->actor->idle_look();};return h;
}
NativeCharacterSideStep::Services Behaviour::side_services(){auto&s=*actor;NativeCharacterSideStep::Services h{};
        h.owner=this;h.actor_world=s.entity().transform;h.region=s.entity().actor.group94;h.child=child?child->entity.id:-1;h.delta=*s.world->script_services().delta;
        h.busy=[](void*p){return static_cast<Behaviour*>(p)->actor->movement->graph.movement_busy();};
        h.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
        h.world=[](void*p,int32_t id)->const float*{auto&s=*static_cast<Behaviour*>(p)->actor;auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return nullptr;
            auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"SideStep target world");return v->transform;};
        h.movement=[](void*p,int32_t id,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;require(s.services.target_vector_property,"SideStep movement owner");return s.services.target_vector_property(s.services.owner,id,"movement",out);};
        h.reset_navigation=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();};
        h.ray=[](void*p,const float*a,const float*z,uint32_t region,uint32_t shape,uint32_t type,uint8_t*hit,int32_t*entity){return CharacterState::ray(static_cast<Behaviour*>(p)->actor,a,z,region,shape,type,hit,entity);};
        h.approach=[](void*p,const float*t,const float*d,NativeCharacterAnimationCallback*c,float rate){auto&s=*static_cast<Behaviour*>(p)->actor;s.movement->graph.queue_approach(*s.body,t,d,c,rate);};return h;
}
UCCharacterPlayerFollowServices Behaviour::player_follow_services(){UCCharacterPlayerFollowServices h{};h.owner=this;
    h.delta=[](void*p)->double{return *static_cast<Behaviour*>(p)->world->script_services().delta;};
    h.player=[](void*p)->const float*{auto&s=*static_cast<Behaviour*>(p)->actor;const float*m=s.live_world(s.services.player_id(s.services.owner));return m?m+12:nullptr;};
    h.actor=[](void*p)->const float*{return static_cast<Behaviour*>(p)->actor->entity().transform+12;};
    h.side_step_tick=[](void*){}; //3B050 is an empty source method.
    h.retarget=[](void*p,const float*t){auto&s=*static_cast<Behaviour*>(p)->actor;s.navigator->retarget(t,s.services.navigation->query(s.entity().id));};
    h.status=[](void*p){return static_cast<Behaviour*>(p)->actor->navigator->status();};
    h.waypoint=[](void*p,float*t){auto&s=*static_cast<Behaviour*>(p)->actor;std::copy_n(s.navigator->waypoint(s.entity().transform+12),3,t);};
    h.near_goal=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->navigator->near_goal();};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.move=[](void*p,const float*t,int32_t mode,uint8_t close){auto&s=*static_cast<Behaviour*>(p)->actor;NativeCharacterMoveTarget target;std::copy_n(t,3,target.position);target.mode=mode;target.near_target=close;s.movement->request(target);};
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    h.look=[](void*p,const float*t,uint8_t force){static_cast<Behaviour*>(p)->actor->movement->graph.aim(t,force?1u:0u);};
    h.property=[](void*p,const char*name,uint8_t fallback){auto&s=*static_cast<Behaviour*>(p)->actor;
        require(s.root&&s.root->medic&&!s.root->plain_medic,"FollowPlayer root property domain");
        require(!std::strcmp(name,"should duck")||!std::strcmp(name,"should sidestep"),"FollowPlayer source root query");
        int32_t value=0;int found=s.owner->property(name,&value);require(found>=0,"FollowPlayer root property result");return found?uint8_t(value!=0):fallback;};
    h.side_step_run=[](void*p,uint8_t forced){auto&b=*static_cast<Behaviour*>(p);auto h=b.side_services();
        if(!forced)b.side_step.update(h);
        else if(b.side_step.force_requested(h)){b.actor->navigator->stop();auto*c=make_step_aside(b);c->parent=&b;b.child=c;b.child_priority=true;require(!c->enter(*b.actor),"StepAsideBehaviour priority-one entry");}};
    //2AB190 checks2A9E40's override handle;2A90C0 then CANCELS it. The
    //admitted native graph has no such override owner. Aim/move busy is distinct.
    h.body_aiming=[](void*)->uint8_t{return 0;};
    h.target=[](void*p){return static_cast<Behaviour*>(p)->actor->current_target();};
    h.idle_look=[](void*p){static_cast<Behaviour*>(p)->actor->idle_look();};
    h.target_head=[](void*p)->const float*{auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;int32_t id=s.current_target();const float*m=s.live_world(id);if(!m)return nullptr;
        require(s.services.target_vector_property,"FollowPlayer target head owner");int found=s.services.target_vector_property(s.services.owner,id,"head pos",b.target_head);require(found>=0,"FollowPlayer target head query");if(!found)std::copy_n(m+12,3,b.target_head);return b.target_head;};
    h.random_pair=[](void*p){int32_t value;require(!uc_game_random_range(static_cast<Behaviour*>(p)->actor->random,0,2,&value),"FollowPlayer source random pair");return value;};
    h.target_kind=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t value=0;require(s.services.dialogue_zone&&!s.services.dialogue_zone(s.services.owner,*s.owner,&value),"FollowPlayer dialogue zone34270");return value;};
    h.player_shield=[](void*p)->uint8_t{auto&s=*static_cast<Behaviour*>(p)->actor;int32_t value=0;int found=s.services.target_property(s.services.owner,s.services.player_id(s.services.owner),"shield active",&value);require(found>=0,"FollowPlayer player shield query");return found&&value;};
    h.character_type=[](void*p){int32_t value=0;int found=static_cast<Behaviour*>(p)->actor->owner->property("character type",&value);require(found>=0,"FollowPlayer actor type query");return found?value:0;};
    h.chance=[](void*p,float chance)->uint8_t{int32_t value;require(!uc_game_random_next(static_cast<Behaviour*>(p)->actor->random,&value),"FollowPlayer source chance2D37D0");float sample=float(value);sample*=0x1p-31f;return chance>sample;};
    h.emote=[](void*p,int32_t code,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(code,priority);};
    h.base=[](void*p){auto&b=*static_cast<Behaviour*>(p);std::copy_n(b.actor->entity().transform,16,b.entity.transform);};return h;
}
UCCharacterStepAsideServices Behaviour::step_aside_services(){UCCharacterStepAsideServices h{};h.owner=this;
    h.target=[](void*p,int32_t id)->const float*{return static_cast<Behaviour*>(p)->actor->live_world(id);};
    h.actor=[](void*p)->const float*{return static_cast<Behaviour*>(p)->actor->entity().transform;};
    h.busy=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->movement->graph.movement_busy();};
    h.clear=[](void*p,const float*point)->uint8_t{auto&s=*static_cast<Behaviour*>(p)->actor;
        for(unsigned i=0;i<3;i++)if(!std::isfinite(point[i]))return 0; //Coincident source radial normalization cannot form a physics query.
        auto q=s.services.navigation->query(s.entity().id);int clear=uc_character_route_clear_options(&q,point,0,0,0);require(clear>=0,"StepAsideBehaviour clearance32D60");return uint8_t(clear);};
    h.side=[](void*p,uint8_t left){auto&b=*static_cast<Behaviour*>(p);if(b.turn_completion)b.turn_completion->release();b.turn_completion=new CharacterCompletion;
        b.actor->movement->graph.queue_action(left?"SidestepLeft":"SidestepRight",&b.turn_completion->callback);};
    h.move=[](void*p,const float*point){auto&b=*static_cast<Behaviour*>(p);if(b.turn_completion)b.turn_completion->release();b.turn_completion=new CharacterCompletion;
        NativeCharacterMoveTarget target;std::copy_n(point,3,target.position);b.actor->movement->request(target,&b.turn_completion->callback);};
    h.notify=[](void*p,uint8_t moved){auto&b=*static_cast<Behaviour*>(p);require(b.actor->root,"StepAsideBehaviour root event owner");uint8_t handled=0;uint32_t sender=uint32_t(b.entity.id);
        require(!uc_entity_event_send(&b.world->registry(),b.actor->root->entity.id,77,&moved,1,&sender,&handled),"StepAsideBehaviour source event77");};
    h.completion=[](void*p){auto*c=static_cast<Behaviour*>(p)->turn_completion;return c?c->status:-1;};
    h.exit=[](void*p){static_cast<Behaviour*>(p)->exit();};return h;
}
UCCharacterInteractionServices Behaviour::interaction_services(){UCCharacterInteractionServices h{};h.owner=this;
    h.player_present=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return uc_entity_event_lookup(&s.world->registry(),s.services.player_id(s.services.owner))?1:0;};
    h.busy=[](void*p){return static_cast<Behaviour*>(p)->actor->controller_tasks.busy()?1:0;};
    h.flags_ready=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;const auto&h=s.world->script_services();return uc_character_loop_flags(h.flags,h.flag_count,0,nullptr);};
    h.in_range=[](void*p,float range,float facing){auto&s=*static_cast<Behaviour*>(p)->actor;
        auto*t=uc_entity_event_lookup(&s.world->registry(),s.services.player_id(s.services.owner));require(t,"NPC interaction player lifetime");
        auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"NPC interaction player position");
        uint8_t inside;int result=uc_character_player_range(s.body?s.body->world+12:s.entity().transform+12,v->transform+12,range,facing,&s,
            [](void*p,float*m,uint8_t*found){auto&s=*static_cast<CharacterState*>(p);require(s.services.target_matrix_property,"NPC interaction Look point owner");
                int r=s.services.target_matrix_property(s.services.owner,s.services.player_id(s.services.owner),"Look point",m);if(r<0)return 1;*found=r!=0;return 0;},&inside);
        require(!result,"NPC interaction player range");return int(inside);};
    h.stop_turn=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();return 0;}; //17BF69 inner90.
    h.turn=[](void*p,int begin){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;require(s.services.target_vector_property,"NPC interaction head property");float point[3];
        int found=s.services.target_vector_property(s.services.owner,s.services.player_id(s.services.owner),"head pos",point);require(found>=0,"NPC interaction head query");if(!found)return 0;
        NativeCharacterAnimationCallback*callback=nullptr;
        if(begin){if(b.turn_completion)b.turn_completion->release();b.turn_completion=new CharacterCompletion;callback=&b.turn_completion->callback;}
        s.movement->graph.aim(point,0,callback);return 0;};
    h.turn_ready=[](void*p){auto*c=static_cast<Behaviour*>(p)->turn_completion;return !c||c->status!=0?1:0;}; //36250 NULL returns3.
    h.speech_ready=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;const int32_t*ids;uint32_t count;
        require(!uc_entity_names_view(s.world->names(),"FRIENDLY_DEATH_CUTSCENE",&ids,&count),"NPC interaction death-cutscene name");return s.owner->dialogue_ready(count!=0)?1:0;};
    h.say=[](void*p,const char*text){static_cast<Behaviour*>(p)->actor->owner->dialogue_request(text,-1,false,false);return 0;}; //DEC10 result is ignored by17C0A0.
    h.speaking=[](void*p){return static_cast<Behaviour*>(p)->actor->owner->dialogue_playing()?1:0;};
    h.script=[](void*p,uint32_t type,int32_t index){return static_cast<Behaviour*>(p)->script_runner.request((int32_t)type,index,true);};
    h.cancel_speech=[](void*p){static_cast<Behaviour*>(p)->actor->owner->dialogue_cancel();return 0;};
    h.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->actor->transition(next);return 0;};
    h.base_update=[](void*p,uint8_t){auto&b=*static_cast<Behaviour*>(p);if(b.actor)std::memcpy(b.entity.transform,b.actor->entity().transform,64);return 0;};return h;
}
void Behaviour::update_condition(){
    //39190 owns a separate insertion-ordered list. The admitted constructors
    //append one actual condition; it receives expensive=true independently
    //of the foreground/overlay budget. Its own exit unlinks this borrow.
    if(condition_child&&condition_child->active)require(!condition_child->foreground(true),"NPC condition update");
}
UCCopAttackServices Behaviour::attack_services(){UCCopAttackServices h{};h.owner=this;h.delta=*world->script_services().delta;
    h.stance_busy=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->movement->graph.cop_stance_busy();};
    h.target=[](void*p){return static_cast<Behaviour*>(p)->actor->current_target();};
    h.player_position=[](void*p)->const float*{auto&s=*static_cast<Behaviour*>(p)->actor;auto*t=uc_entity_event_lookup(&s.world->registry(),s.services.player_id(s.services.owner));if(!t)return nullptr;
        auto&c=s.world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"Cop attack player position");return v->transform+12;};
    h.dialogue_mode=[](void*p)->int32_t{auto&s=*static_cast<Behaviour*>(p)->actor;int32_t value=0;require(s.services.dialogue_zone&&!s.services.dialogue_zone(s.services.owner,*s.owner,&value),"Cop dialogue zone34270");return value;};
    h.dialogue=[](void*p,int32_t code,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(code,priority);};
    h.search_cover=[](void*p){auto&b=*static_cast<Behaviour*>(p);require(b.cover!=nullptr,"Cop cover owner");b.cover->step(b.attack);};
    h.nav_status=[](void*p){return static_cast<Behaviour*>(p)->actor->navigator->status();};
    h.waypoint=[](void*p,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;std::copy_n(s.navigator->waypoint(s.entity().transform+12),3,out);};
    h.near_goal=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->navigator->near_goal();};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.move=[](void*p,const float*point,int32_t mode,uint8_t nearby){auto&s=*static_cast<Behaviour*>(p)->actor;NativeCharacterMoveTarget target;
        std::copy_n(point,3,target.position);target.mode=mode;target.near_target=nearby;s.movement->request(target);};
    h.crouch=[](void*){throw std::runtime_error("Cop attack crouch bodyBC task is unconnected");};
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    h.random=[](void*p,float a,float z){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t draw;require(!uc_game_random_next(s.random,&draw),"Cop attack shared random");return float((double(draw)*(double(z)-a))*0x1p-31+a);};
    h.gun_target=[](void*,int32_t)->uint8_t{throw std::runtime_error("Cop gun target eligibility virtual24 is unconnected");};
    h.stand_if_crouching=[](void*)->uint8_t{throw std::runtime_error("Cop attack crouch predicate is unconnected");};
    h.clear_target=[](void*p){static_cast<Behaviour*>(p)->actor->targets.current=-1;};
    h.stance_mode=[](void*p){return static_cast<Behaviour*>(p)->actor->movement->graph.cop_stance_mode();};
    h.idle_look=[](void*p){static_cast<Behaviour*>(p)->actor->idle_look();};
    h.conditions=[](void*p){static_cast<Behaviour*>(p)->update_condition();};return h;
}
NativeCharacterThreatServices Behaviour::threat_services(){auto&s=*actor;NativeCharacterThreatServices h{};h.owner=&s;h.actor=s.entity().id;
    h.delta=*world->script_services().delta;h.time=world->time();h.position=s.entity().transform+12;
    h.target=[](void*p){return static_cast<CharacterState*>(p)->current_target();};
    h.resolve=[](void*p,int32_t id,float*out){const float*m=static_cast<CharacterState*>(p)->live_world(id);if(!m)return 0;std::copy_n(m+12,3,out);return 1;};
    h.body_forward=[](void*p,int32_t id,float*out){const float*m=static_cast<CharacterState*>(p)->live_world(id);if(!m)return 1;std::copy_n(m+8,3,out);return 0;};
    h.direction=[](void*p,bool right,float*out){auto&s=*static_cast<CharacterState*>(p);uc_character_threat_direction(s.entity().transform+8,right,out);return 0;};
    h.property=[](void*p,int32_t id,const char*name,int32_t*out){auto&s=*static_cast<CharacterState*>(p);return s.services.target_property(s.services.owner,id,name,out);};
    h.point=[](void*p,int32_t id,const char*name,float*out){auto&s=*static_cast<CharacterState*>(p);return s.services.target_vector_property(s.services.owner,id,name,out);};
    h.visible=[](void*p,int32_t id,float cosine,bool head,const float*offset,bool characters){auto&s=*static_cast<CharacterState*>(p);
        require(s.services.visible_query,"NPC parameterized visibility service");return s.services.visible_query(s.services.owner,*s.owner,id,cosine,head,offset,characters);};
    h.near_line=[](void*p,int32_t id,float radius){auto&s=*static_cast<CharacterState*>(p);float position[3],target[3];require(s.point(s.entity().id,"target position",position)==1,"NPC own target position");
        const float*m=s.live_world(id);if(!m)return 0;require(s.point(id,"target position",target)==1,"NPC near target position");return uc_character_threat_line_near(position,target,m+8,radius);};
    h.recent=[](void*p,float*out){auto&s=*static_cast<CharacterState*>(p);*out=s.recent_attention_time;return s.recent_attention?1:0;};
    h.segment_clear=[](void*p,const float*from,const float*to){auto&s=*static_cast<CharacterState*>(p);require(s.services.hazard_segment,"NPC hazard segment owner");return s.services.hazard_segment(s.services.owner,from,to);};
    h.difficulty=[](void*p,float minimum,float maximum){auto&s=*static_cast<CharacterState*>(p);double normalized;require(!s.services.difficulty(s.services.owner,&normalized),"NPC threat session difficulty");return (double(maximum)-minimum)*normalized+minimum;};
    h.random=[](void*p,float minimum,float maximum){auto&s=*static_cast<CharacterState*>(p);int32_t value;require(!uc_game_random_next(s.random,&value),"NPC threat shared random");return (double(value)*(double(maximum)-minimum))*0x1p-31+minimum;};return h;
}
UCBlazerThrow Behaviour::throw_state(){auto&s=*actor;UCBlazerThrow v{};v.target=threat.current;v.actor_flags=s.actor_status;
    std::copy_n(s.entity().transform+12,3,v.position);std::copy_n(threat.target,3,v.target_position);std::copy_n(threat.last_clear,3,v.last_clear);
    v.distance=threat.distance;v.facing_dot=threat.facing_dot;v.shield_time=threat.shield_time;v.near_line=threat.near_line;v.throw_low=blazer_throw_low;v.melee_enabled=blazer_melee_enabled;v.cooldown=blazer_cooldown;
    v.aim_task=(uintptr_t)blazer_tasks[0];v.wait_task=(uintptr_t)blazer_tasks[1];v.side_task=(uintptr_t)blazer_tasks[2];return v;
}
UCBlazerThrowServices Behaviour::throw_services(){UCBlazerThrowServices h{};h.owner=this;
    h.active=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->root->thrown->no_primary();};
    h.ready=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->root->thrown->ready();};
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    h.task_status=[](void*,uintptr_t task){return static_cast<CharacterCompletion*>((void*)task)->status;};
    h.task_create=[](void*p,uintptr_t*slot)->uintptr_t{auto&b=*static_cast<Behaviour*>(p);auto*&retained=b.blazer_tasks[b.throw_slot];require(*slot==(uintptr_t)retained,"Blazer throw task slot identity");
        if(retained){auto*old=retained;retained=nullptr;old->release();}retained=new CharacterCompletion;*slot=(uintptr_t)retained;return *slot;};
    h.play=[](void*p,const char*name,uintptr_t token,uint8_t overlay){auto&b=*static_cast<Behaviour*>(p);b.actor->movement->graph.queue_action(name,token?&reinterpret_cast<CharacterCompletion*>(token)->callback:nullptr,overlay!=0);};
    h.prime=[](void*p){static_cast<Behaviour*>(p)->actor->root->thrown->prime();};
    h.target=[](void*p){return static_cast<Behaviour*>(p)->actor->current_target();};
    h.set_target=[](void*p,int32_t id,float velocity,float seconds,uint8_t flags){static_cast<Behaviour*>(p)->actor->root->thrown->target(id,velocity,seconds,flags);};
    h.can_throw=[](void*p,int32_t id,float velocity)->uint8_t{return static_cast<Behaviour*>(p)->actor->root->thrown->can_throw(id,velocity);};
    h.throw_target=[](void*p,int32_t id,float velocity,uint32_t flags){static_cast<Behaviour*>(p)->actor->root->thrown->throw_target(id,velocity,flags);};
    h.throw_point=[](void*p,const float*point,float velocity,uint32_t flags){static_cast<Behaviour*>(p)->actor->root->thrown->throw_point(point,velocity,flags);};
    h.sound=[](void*p,const char*name){auto&s=*static_cast<Behaviour*>(p)->actor;require(s.services.character_sound&&!s.services.character_sound(s.services.owner,*s.owner,s.character_audio,name,false),"Blazer actor sound event");};
    h.speech=[](void*p,int32_t code,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(code,priority);};
    h.speech_pair=[](void*p,int32_t a,int32_t b,float chance,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say_pair(a,b,chance,priority);};
    h.primary_target=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
    h.random=[](void*p,float lo,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t value;require(!uc_game_random_next(s.random,&value),"Blazer throw shared random");return (double(value)*(double(hi)-lo))*0x1p-31+lo;};
    h.difficulty=[](void*p,float lo,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;double value=0;require(s.services.throw_difficulty&&!s.services.throw_difficulty(s.services.owner,lo,hi,&value),"Blazer throw difficulty168B0");return value;};
    h.difficulty3=[](void*p,float lo,float mid,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;double value=0;require(s.services.throw_curve&&!s.services.throw_curve(s.services.owner,lo,mid,hi,&value),"Blazer throw difficulty169C0");return value;};return h;
}
int Behaviour::prepare_throw(){throw_slot=1;auto value=throw_state();auto h=throw_services();int result=uc_blazer_throw_prepare(&value,&h);require(result>=0,"Blazer prepare throw1D4120");blazer_melee_enabled=value.melee_enabled;return result;}
int Behaviour::request_throw(uint8_t close,uint8_t alternate){throw_slot=2;auto value=throw_state();auto h=throw_services();int result=uc_blazer_throw_request(&value,&h,close,alternate);require(result>=0,"Blazer request throw1D4310");return result;}
float Behaviour::reset_throw_cooldown(){auto value=throw_state();auto h=throw_services();require(!uc_blazer_throw_cooldown(&value,&h),"Blazer throw cooldown1D39F0");return value.cooldown;}
UCBlazerCombatServices Behaviour::combat_services(){UCBlazerCombatServices h{};h.owner=this;
    h.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.reset_navigation=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();};
    h.task_release=[](void*p,uintptr_t token){auto&b=*static_cast<Behaviour*>(p);auto*task=reinterpret_cast<CharacterCompletion*>(token);bool found=false;
        for(auto*&slot:b.blazer_tasks)if(slot==task){slot=nullptr;found=true;}require(found,"Blazer released task identity");task->release();};
    h.task_status=[](void*,uintptr_t token){return reinterpret_cast<CharacterCompletion*>(token)->status;};
    h.aim_task_create=[](void*p,uintptr_t*slot)->uintptr_t{auto&b=*static_cast<Behaviour*>(p);require(*slot==(uintptr_t)b.blazer_tasks[0],"Blazer aim task slot");
        auto*old=b.blazer_tasks[0];b.blazer_tasks[0]=nullptr;if(old)old->release();b.blazer_tasks[0]=new CharacterCompletion;*slot=(uintptr_t)b.blazer_tasks[0];return *slot;};
    h.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->blazer_transition(next);};
    h.attention=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->attention_active;};
    h.look=[](void*p,const float*point){static_cast<Behaviour*>(p)->actor->movement->graph.look_at(point);};
    h.cancel_position_search=[](void*p,uint8_t count){static_cast<Behaviour*>(p)->positioning->request(count);};
    h.throw_active=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->root->thrown->no_primary();};
    h.throw_ready=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->root->thrown->ready();};
    h.throw_elapsed=[](void*p)->double{return static_cast<Behaviour*>(p)->actor->root->thrown->fuse();};
    h.throw_reset=[](void*p){auto&b=*static_cast<Behaviour*>(p);b.actor->root->thrown->pause_fuses(*b.world->script_services().delta);};
    h.throw_target=[](void*p,int32_t id,float velocity,float delay,uint8_t flags){static_cast<Behaviour*>(p)->actor->root->thrown->target(id,velocity,delay,flags);};
    h.publish_shield=[](void*p,uint8_t shield){static_cast<Behaviour*>(p)->actor->target_cue(shield!=0);};
    h.aim=[](void*p,const float*point,uintptr_t task,uint32_t mode){static_cast<Behaviour*>(p)->actor->movement->graph.aim(point,mode,task?&reinterpret_cast<CharacterCompletion*>(task)->callback:nullptr);};
    h.throw_predicate=[](void*p)->uint8_t{return (uint8_t)static_cast<Behaviour*>(p)->prepare_throw();};
    h.throw_request=[](void*p,uint8_t close,uint8_t alternate)->uint8_t{return (uint8_t)static_cast<Behaviour*>(p)->request_throw(close,alternate);};
    h.reset_cooldown=[](void*p){return static_cast<Behaviour*>(p)->reset_throw_cooldown();};return h;
}
void Behaviour::blazer_transition(int32_t next){auto&s=*actor;
    require(next>=0&&next<8,"Blazer state transition range");
    if(blazer_stage==5)require(s.services.character_sound&&!s.services.character_sound(s.services.owner,*s.owner,s.character_audio,"FireBreathLoop",true),"Blazer FireBreathLoop stop216060");
    blazer_stage=next;blazer_idle.entered=0;blazer_elapsed=0;blazer_hit=0;blazer_melee_elapsed=0;
    if(!s.attention_active)s.movement->graph.stop_look();
    static const char*labels[]={"STATE: Route to first target","STATE: No Target","STATE: New Target","STATE: Move to Attack","STATE: Attack","STATE: Melee","STATE: SideStep","STATE: Duck"};
    s.debug_states.clear();s.debug_states.push_back({labels[next],0,true});
}
int Behaviour::blazer_update(bool expensive){auto&s=*actor;double delta=*world->script_services().delta;
    blazer_elapsed=float(double(blazer_elapsed)+delta);
    //Source772 has no defended entity, fixed-position or initial-route flags;
    //1D3D40 therefore performs no mutable work before the actual acquisition.
    if(threat.unseen_time>3){s.acquire(true,true);if(s.current_target()!=threat.current)blazer_transition(2);}
    else if(expensive||blazer_stage==1){s.acquire(false,false);if(s.current_target()!=threat.current)blazer_transition(2);}
    auto h=threat_services();require(!native_character_threat_step(threat,h,expensive),"Blazer threat owner2FE60");
    if(threat.current!=-1){uint32_t sender=(uint32_t)entity.id,payload=(uint32_t)s.entity().id;uint8_t handled=0;
        require(!uc_entity_event_send(&world->registry(),threat.current,0x35,&payload,4,&sender,&handled),"Blazer targeted contact event35");}
    else if(blazer_stage!=1)blazer_transition(1);
    if(s.debug_states.size()<10)s.debug_states.push_back({threat.in_cone?"Has LOS":threat.visible?"Available LOS":"No LOS",(float)delta,false});
    //1D40D0 consumes the actual shared-manager job before state dispatch.
    require(positioning!=nullptr,"Blazer retained attack-position owner");
    if(positioning->status()==3){std::copy_n(positioning->point(),3,attack_position);positioning->clear();position_ready=position_changed=true;}
    blazer_cooldown=float(double(blazer_cooldown)-delta);
    UCBlazerAttackIdleServices idle{};idle.owner=this;
    idle.target=[](void*p){auto&b=*static_cast<Behaviour*>(p);return b.blazer_stage==2?b.actor->current_target():b.threat.current;};
    idle.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->blazer_transition(next);};
    idle.reset_navigation=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();};
    idle.stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->stop();};
    idle.nav_status=[](void*p){return static_cast<Behaviour*>(p)->actor->navigator->status();};
    idle.waypoint=[](void*p,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;std::copy_n(s.navigator->waypoint(s.entity().transform+12),3,out);};
    idle.near_goal=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->navigator->near_goal();};
    idle.move=[](void*p,const float*point,int32_t mode,uint8_t nearby){auto&s=*static_cast<Behaviour*>(p)->actor;NativeCharacterMoveTarget target;std::copy_n(point,3,target.position);target.mode=mode;target.near_target=nearby;s.movement->request(target);};
    idle.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    int result=0;
    if(blazer_stage==1)result=uc_blazer_attack_idle(&blazer_idle,&idle);
    else if(blazer_stage==2)result=uc_blazer_attack_new_target(&blazer_idle,&blazer_first,blazer_tasks[2]?blazer_tasks[2]->status:-1,&idle);
    else if(blazer_stage==3){UCBlazerAttackMove value{};value.entered=blazer_idle.entered;value.in_cone=threat.in_cone;value.position_ready=position_ready;value.position_changed=position_changed;
        value.target=threat.current;value.distance=threat.distance;value.elapsed=blazer_elapsed;std::copy_n(attack_position,3,value.position);std::copy_n(threat.target,3,value.target_position);
        UCBlazerAttackMoveServices moving_services{};moving_services.owner=this;moving_services.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
        moving_services.dialogue=[](void*p,int32_t code,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(code,priority);};
        moving_services.paired_dialogue=[](void*p,int32_t a,int32_t b,float chance,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say_pair(a,b,chance,priority);};
        moving_services.retarget=[](void*p,const float*point){auto&s=*static_cast<Behaviour*>(p)->actor;s.navigator->retarget(point,s.services.navigation->query(s.entity().id));};
        moving_services.run=[](void*p){const uint8_t flags[5]={0,1,0,1,0};return static_cast<Behaviour*>(p)->actor->run_flags(flags);};moving_services.transition=idle.transition;
        result=uc_blazer_attack_move(&value,&moving_services);if(blazer_stage==3)blazer_idle.entered=value.entered;position_ready=value.position_ready!=0;position_changed=value.position_changed!=0;
    }else if(blazer_stage==4){UCBlazerCombat value{};value.entered=blazer_idle.entered;value.threw=blazer_threw;value.visible=threat.visible;value.in_cone=threat.in_cone;value.facing=threat.facing;
        value.position_ready=position_ready;value.position_changed=position_changed;value.melee_enabled=blazer_melee_enabled;value.target=threat.current;
        std::copy_n(threat.aim,3,value.aim);value.distance=threat.distance;value.facing_dot=threat.facing_dot;value.near_time=threat.near_time;value.target_y=threat.target[1];value.actor_y=s.entity().transform[13];
        value.shield_time=threat.shield_time;value.cooldown=blazer_cooldown;value.aim_task=(uintptr_t)blazer_tasks[0];value.wait_task=(uintptr_t)blazer_tasks[1];value.side_task=(uintptr_t)blazer_tasks[2];
        auto combat=combat_services();result=uc_blazer_combat_step(&value,&combat);if(blazer_stage==4)blazer_idle.entered=value.entered;blazer_threw=value.threw;blazer_cooldown=value.cooldown;
    }else if(blazer_stage==6){UCBlazerEvade value{};value.entered=blazer_idle.entered;value.near_line=threat.near_line;value.target=threat.current;value.task=(uintptr_t)blazer_tasks[3];
        std::copy_n(s.entity().transform+12,3,value.position);std::copy_n(threat.direction,3,value.direction);std::copy_n(threat.aim,3,value.aim);
        UCBlazerEvadeServices evade{};evade.owner=this;
        evade.random_side=[](void*p)->uint8_t{auto&s=*static_cast<Behaviour*>(p)->actor;int32_t side;require(!uc_game_random_boolean(s.random,&side),"Blazer sidestep shared random");return side!=0;};
        evade.right=[](void*p,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;uc_character_threat_direction(s.entity().transform+8,1,out);};
        evade.clear=[](void*p,const float*point){auto&s=*static_cast<Behaviour*>(p)->actor;auto q=s.services.navigation->query(s.entity().id);return uc_character_route_clear_options(&q,point,q.radius,1,0);};
        evade.inside_defend=[](void*,const float*)->uint8_t{return 1;}; //12EF00: authored772's defend flag is zero.
        evade.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
        evade.pair=[](void*p,int32_t a,int32_t b,float chance,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say_pair(a,b,chance,priority);};
        evade.task_create=[](void*p,uintptr_t*slot)->uintptr_t{auto&b=*static_cast<Behaviour*>(p);auto*&task=b.blazer_tasks[3];require(*slot==(uintptr_t)task,"Blazer sidestep task identity");auto*old=task;task=nullptr;if(old)old->release();task=new CharacterCompletion;*slot=(uintptr_t)task;return *slot;};
        evade.approach=[](void*p,const float*point,const float*direction,uintptr_t task,float rate){auto&s=*static_cast<Behaviour*>(p)->actor;s.movement->graph.queue_approach(*s.body,point,direction,&reinterpret_cast<CharacterCompletion*>(task)->callback,rate);};
        evade.aim=[](void*p,const float*point){static_cast<Behaviour*>(p)->actor->movement->graph.aim(point,0);};
        evade.attention=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->attention_active;};
        evade.look=[](void*p,const float*point){static_cast<Behaviour*>(p)->actor->movement->graph.look_at(point);};
        evade.task_status=[](void*,uintptr_t task){return reinterpret_cast<CharacterCompletion*>(task)->status;};evade.transition=idle.transition;
        result=uc_blazer_evade(&value,&evade);if(blazer_stage==6)blazer_idle.entered=value.entered;
    }else if(blazer_stage==7){UCBlazerDuck value{};value.entered=blazer_idle.entered;value.thrown=blazer_throw;value.near_line=threat.near_line;value.target=threat.current;
        value.delay=blazer_melee_elapsed;value.quiet=blazer_duck_quiet;value.throw_task=(uintptr_t)blazer_tasks[2];value.duck_task=(uintptr_t)blazer_tasks[4];value.stop_task=(uintptr_t)blazer_tasks[5];
        UCBlazerDuckServices duck{};duck.owner=this;duck.delta=delta;
        duck.status=[](void*,uintptr_t task){return reinterpret_cast<CharacterCompletion*>(task)->status;};
        duck.release=[](void*p,uintptr_t token){auto&b=*static_cast<Behaviour*>(p);auto*task=reinterpret_cast<CharacterCompletion*>(token);bool found=false;for(auto*&slot:b.blazer_tasks)if(slot==task){slot=nullptr;found=true;}require(found,"Blazer duck task identity");task->release();};
        duck.create=[](void*p,unsigned index,uintptr_t*slot)->uintptr_t{auto&b=*static_cast<Behaviour*>(p);require(index==1||index==2,"Blazer duck task slot");auto*&task=b.blazer_tasks[index+3];require(*slot==(uintptr_t)task,"Blazer duck retained task");auto*old=task;task=nullptr;if(old)old->release();task=new CharacterCompletion;*slot=(uintptr_t)task;return *slot;};
        duck.duck=[](void*p,uintptr_t task){auto&s=*static_cast<Behaviour*>(p)->actor;float head[16];require(s.owner->posed_bone_local(s.head_bone,head),"Blazer duck posed head");s.movement->graph.queue_duck(head[13]<130.f,&reinterpret_cast<CharacterCompletion*>(task)->callback);};
        duck.stop=[](void*p,uintptr_t task){static_cast<Behaviour*>(p)->actor->movement->graph.stop(&reinterpret_cast<CharacterCompletion*>(task)->callback);};
        duck.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
        duck.pair=[](void*p,int32_t a,int32_t b,float chance,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say_pair(a,b,chance,priority);};
        duck.health_cue=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;if(double(s.entity().actor.maximumA0)*.25>double(s.entity().actor.health9C))s.dialogue->say(10,25);};
        duck.bullet_by=[](void*p){return static_cast<Behaviour*>(p)->actor->bullet_by144;};
        duck.difficulty=[](void*p,float lo,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;double value=0;require(s.services.throw_difficulty&&!s.services.throw_difficulty(s.services.owner,lo,hi,&value),"Blazer duck difficulty168B0");return value;};duck.transition=idle.transition;
        result=uc_blazer_duck(&value,&duck);if(blazer_stage==7){blazer_idle.entered=value.entered;blazer_melee_elapsed=value.delay;}blazer_duck_quiet=value.quiet;
    }else throw std::runtime_error("Blazer attack state "+std::to_string(blazer_stage)+" owner is not connected");
    if(!result)std::memcpy(entity.transform,s.entity().transform,64);return result;
}
void Behaviour::brute_transition(int32_t next){
    require(next>=0&&next<8,"Brute state transition range");brute_stage=next;brute.entered=0;brute.elapsed=brute.since_action=brute_side.cooldown=0;
    static const char*labels[]={"STATE: NoTarget","STATE: NewTarget","STATE: MoveToCombat","STATE: InCombat","STATE: ThrowCleaver","STATE: Taunt","STATE: Jump Attack","STATE: Blocked Taunt"};
    actor->debug_states.clear();actor->debug_states.push_back({labels[next],0,true});
}
void Behaviour::brute_acquire(bool prefer,bool visible){
    actor->acquire(prefer,visible); //1D1180 invokes2FE00 when changed; that source function is empty.
}
void Behaviour::brute_refresh(){auto&s=*actor;
    brute.target=threat.current;brute.actor_flags=s.actor_status;brute.in_cone=threat.in_cone;
    brute.distance=threat.distance;brute.facing_dot=threat.facing_dot;brute.shield_time=threat.shield_time;brute.target_y=threat.target[1];
    std::copy_n(s.entity().transform+12,3,brute.position);std::copy_n(threat.aim,3,brute.aim);
}
UCBruteChaseServices Behaviour::brute_chase_services(){UCBruteChaseServices h{};h.owner=this;h.delta=*world->script_services().delta;
    h.retarget=[](void*p,const float*t){auto&s=*static_cast<Behaviour*>(p)->actor;s.navigator->retarget(t,s.services.navigation->query(s.entity().id));};
    h.random_boolean=[](void*p)->uint8_t{int32_t v=0;require(!uc_game_random_boolean(static_cast<Behaviour*>(p)->actor->random,&v),"Brute Boolean random2D3800");return uint8_t(v);};
    h.movement_mode=[](void*p,uint8_t mode){static_cast<Behaviour*>(p)->actor->movement->graph.brute_movement_mode(mode);};
    h.say=[](void*p,int32_t a,int32_t b){static_cast<Behaviour*>(p)->actor->dialogue->say(a,b);};
    h.pair=[](void*p,int32_t a,int32_t b,float chance,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say_pair(a,b,chance,priority);};
    h.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->brute_transition(next);};
    h.visible=[](void*p,int32_t target){auto&s=*static_cast<Behaviour*>(p)->actor;const float zero[3]{};return s.services.visible_query(s.services.owner,*s.owner,target,-1,false,zero,false);};
    h.charge_allowed=[](void*p){auto&b=*static_cast<Behaviour*>(p);b.brute_refresh();return int(uc_brute_charge_predicate(&b.brute));};
    h.aim=[](void*p,const float*t){static_cast<Behaviour*>(p)->actor->movement->graph.aim(t,0);};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.attention=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->attention_active;};
    h.look=[](void*p,const float*t){static_cast<Behaviour*>(p)->actor->movement->graph.look_at(t);};
    h.stop_look=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_look();};
    h.nav_status=[](void*p){return static_cast<Behaviour*>(p)->actor->navigator->status();};
    h.navigation_busy=[](void*p,const float*t){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;const bool busy=s.movement->graph.movement_busy();
        if(busy&&!b.brute_nav_paused){s.navigator->stop();b.brute_nav_paused=1;}
        else if(!busy&&b.brute_nav_paused){s.navigator->retarget(t,s.services.navigation->query(s.entity().id));b.brute_nav_paused=0;}}; //1D1EF0 / inner78=2ACBB0.
    h.nav_stop=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();};
    h.acquire=[](void*p,uint8_t a,uint8_t b){static_cast<Behaviour*>(p)->brute_acquire(a!=0,b!=0);};
    h.evade_allowed=[](void*p){auto&b=*static_cast<Behaviour*>(p);b.brute_refresh();auto s=b.brute_combat_services();return int(uc_brute_throw_predicate(&b.brute,&s));};
    h.waypoint=[](void*p,float*t){auto&s=*static_cast<Behaviour*>(p)->actor;std::copy_n(s.navigator->waypoint(s.entity().transform+12),3,t);};
    h.near_goal=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->navigator->near_goal();};
    h.move=[](void*p,const float*t,int32_t mode,uint8_t close){auto&s=*static_cast<Behaviour*>(p)->actor;NativeCharacterMoveTarget target;std::copy_n(t,3,target.position);target.mode=mode;target.near_target=close;s.movement->request(target);};
    h.route_clear=[](void*p,const float*t){auto&s=*static_cast<Behaviour*>(p)->actor;auto q=s.services.navigation->query(s.entity().id);return uc_character_route_clear_options(&q,t,0,0,0);};
    h.difficulty=[](void*p,float lo,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;double v=0;require(s.services.brute_difficulty&&!s.services.brute_difficulty(s.services.owner,lo,hi,&v),"Brute channel0 difficulty168B0");return v;};
    h.difficulty_range=[](void*p,float lo,float hi,float begin,float end){auto&s=*static_cast<Behaviour*>(p)->actor;double v=0;require(s.services.brute_range&&!s.services.brute_range(s.services.owner,lo,hi,begin,end,&v),"Brute channel0 range168D0");return v;};
    h.nav_final=[](void*p){return int(static_cast<Behaviour*>(p)->actor->navigator->blocked());};
    h.target_line=[](void*p,int32_t id,float radius){auto&b=*static_cast<Behaviour*>(p);auto services=b.threat_services();return services.near_line(services.owner,id,radius);};
    h.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
    h.target_world=[](void*p,int32_t id,float*out){const float*m=static_cast<Behaviour*>(p)->actor->live_world(id);if(!m)return 1;std::copy_n(m,16,out);return 0;};return h;
}
UCBruteCombatServices Behaviour::brute_combat_services(){UCBruteCombatServices h{};h.owner=this;
    h.transition=[](void*p,int32_t next){static_cast<Behaviour*>(p)->brute_transition(next);};
    h.reset_navigation=[](void*p){static_cast<Behaviour*>(p)->actor->navigator->stop();};
    h.attention=[](void*p)->uint8_t{return static_cast<Behaviour*>(p)->actor->attention_active;};
    h.clear_attention=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;if(s.attention)uc_character_attention_reset(s.attention.get());};
    h.look=[](void*p,const float*t){static_cast<Behaviour*>(p)->actor->movement->graph.look_at(t);};
    h.stop_aim=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();};
    h.stop_look=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_look();};
    h.aim=[](void*p,const float*t){static_cast<Behaviour*>(p)->actor->movement->graph.aim(t,0);};
    h.pressure=[](void*p,float limit){auto&s=*static_cast<Behaviour*>(p)->actor;
        if(s.current_target()==-1)return;const float*target=s.live_world(s.current_target());if(!target)return;
        float out[16];uint8_t changed=0;
        require(!uc_character_face_target(s.entity().transform,s.entity().transform+12,target+12,limit,out,&changed),"Brute facing33E70");if(!changed)return;
        //118/virtual4C=2AAFA0:2AAEA0 sets physical position;2AAF10 sets
        //both normalized heading slots. The next body step publishes rotation.
        std::copy_n(out+12,3,s.body->world+12);uc_collision_to_physics(out+12,s.body->physics.position);std::fill_n(s.body->velocity,3,0.f);
        float yaw=(float)-std::atan2((double)out[8],(double)out[10]);
        constexpr double pi=3.1415927410125732,tau=6.2831854820251465;
        yaw=yaw>0?(float)(std::fmod((double)yaw+pi,tau)-pi):(float)-(std::fmod(pi-(double)yaw,tau)-pi);
        s.body->heading.value=s.body->heading.target=yaw;
    };
    h.delta=[](void*p){return float(*static_cast<Behaviour*>(p)->world->script_services().delta);};
    h.task_release=[](void*,uintptr_t t){reinterpret_cast<CharacterCompletion*>(t)->release();};
    h.task_status=[](void*,uintptr_t t){return reinterpret_cast<CharacterCompletion*>(t)->status;};
    h.task_create=[](void*,uintptr_t*slot)->uintptr_t{if(*slot)reinterpret_cast<CharacterCompletion*>(*slot)->release();*slot=(uintptr_t)new CharacterCompletion;return *slot;};
    h.melee_reset=[](void*p){uc_npc_melee_cancel(static_cast<Behaviour*>(p)->actor->root->melee.get());};
    h.melee_facing=[](void*p){return uc_npc_melee_facing(static_cast<Behaviour*>(p)->actor->root->melee.get());};
    h.melee_distance=[](void*p){return uc_npc_melee_range(static_cast<Behaviour*>(p)->actor->root->melee.get());};
    h.melee_prepare=[](void*p,int32_t target){uc_npc_melee_target(static_cast<Behaviour*>(p)->actor->root->melee.get(),target,1,0);};
    h.melee_action=[](void*,uint32_t,uintptr_t){throw std::runtime_error("Brute melee animation2A9710 is unconnected");};
    h.melee_move=[](void*,uint32_t){throw std::runtime_error("Brute melee movement2A97A0 is unconnected");};
    h.sound=[](void*p,const char*name){auto&s=*static_cast<Behaviour*>(p)->actor;require(s.services.character_sound&&!s.services.character_sound(s.services.owner,*s.owner,s.character_audio,name,false),"Brute sound216BD0");};
    h.dialogue=[](void*p,uint32_t event,uint32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say((int32_t)event,(int32_t)priority);};
    h.random_int=[](void*p,int32_t first,int32_t last){int32_t v=0;require(!uc_game_random_range(static_cast<Behaviour*>(p)->actor->random,first,last,&v),"Brute integer random2D37B0");return v;};
    h.random_bool=[](void*p)->uint8_t{int32_t v=0;require(!uc_game_random_boolean(static_cast<Behaviour*>(p)->actor->random,&v),"Brute Boolean random2D3800");return uint8_t(v);};
    h.random_float=[](void*p,float lo,float hi){int32_t v=0;require(!uc_game_random_next(static_cast<Behaviour*>(p)->actor->random,&v),"Brute shared random2D3780");return (double(v)*(double(hi)-lo))*0x1p-31+lo;};
    h.difficulty_delay=[](void*p,float lo,float mid,float hi){auto&s=*static_cast<Behaviour*>(p)->actor;double v=0;require(s.services.brute_curve&&!s.services.brute_curve(s.services.owner,lo,mid,hi,&v),"Brute channel0 curve169C0");return v;};
    h.target_is_player=[](void*p)->uint8_t{auto&b=*static_cast<Behaviour*>(p);return b.threat.current==b.actor->services.player_id(b.actor->services.owner);};
    h.throw_clip=[](void*p,uintptr_t t){auto&b=*static_cast<Behaviour*>(p);b.actor->movement->graph.queue_clip(b.loop_clip,&reinterpret_cast<CharacterCompletion*>(t)->callback,false,false);};
    h.action=[](void*p,const char*name,uintptr_t t){static_cast<Behaviour*>(p)->actor->movement->graph.queue_action(name,&reinterpret_cast<CharacterCompletion*>(t)->callback);};return h;
}
int Behaviour::brute_update(bool expensive){auto&s=*actor;const double delta=*world->script_services().delta;
    //1D33B0: use the faction's retained coattacker list and real properties.
    if(s.faction_joined){auto h=s.target_services();int32_t count=s.services.factions->attackers((int32_t)s.faction,s.entity().id,h),closer=0;brute.route=0;brute.retreat=0;
        if(expensive)s.navigator->clear_influences();
        for(int32_t i=0;i<count;i++){int32_t id=s.services.factions->attacker((int32_t)s.faction,s.entity().id,(uint32_t)i,h);if(id==s.entity().id)continue;
            auto flag=[&](const char*name){int32_t v=0;int found=s.services.target_property(s.services.owner,id,name,&v);require(found>=0,"Brute coattacker property");return found&&uint8_t(v);};
            if(flag("team player")){if(flag("throwing cleaver"))++brute.route;
                if(flag("melee")){double range=threat.distance;int found=s.services.target_scalar_property(s.services.owner,id,"target range",&range);require(found>=0,"Brute coattacker target range");if(threat.distance>range)++closer;}
                if(flag("taunting")){auto random=brute_combat_services();brute.taunt_delay=(float)random.random_float(this,2,5);}}
            if(expensive)s.navigator->influence(id,500,30);
        }brute.retreat=closer>=2;
    }
    //1D21A0 compares against the previous threat target before perception.
    if(threat.unseen_time>3){brute_acquire(true,true);if(s.current_target()!=threat.current)brute_transition(1);}
    else if(expensive||brute_stage==0){brute_acquire(false,false);if(s.current_target()!=threat.current)brute_transition(1);}
    if(s.current_target()!=threat.current)brute_transition(1);
    auto threat_hook=threat_services();require(!native_character_threat_step(threat,threat_hook,expensive),"Brute threat2FE60");
    if(threat.current!=-1){uint32_t sender=(uint32_t)entity.id,payload=(uint32_t)s.entity().id;uint8_t handled=0;
        require(!uc_entity_event_send(&world->registry(),threat.current,0x35,&payload,4,&sender,&handled),"Brute contact event35");}
    else if(brute_stage!=0)brute_transition(0);
    if(brute_throw)throw std::runtime_error("Brute thrown Cleaver factory and physics1D2380 are unconnected");
    if(brute_restock){uc_npc_melee_cancel(s.root->melee.get());brute_restock=0;}
    brute.elapsed=float(double(brute.elapsed)+delta);brute.since_attack=float(double(brute.since_attack)+delta);brute.throw_cooldown=float(double(brute.throw_cooldown)-delta);
    brute_refresh();int result=0;
    if(brute_stage==0)brute.entered=1;
    else if(brute_stage==1)brute_transition(2);
    else if(brute_stage==2){UCBruteChase value{};value.entered=brute.entered;value.charge_enabled=brute.can_charge;value.charge=brute_charge;
        value.special_close=brute.retreat;value.target_moved=threat.moved;value.near_line=threat.near_line;value.target=threat.current;
        value.elapsed=brute.elapsed;value.distance=threat.distance;value.facing_dot=threat.facing_dot;value.seen_time=threat.seen_time;
        std::copy_n(threat.target,3,value.target_position);std::copy_n(brute.position,3,value.position);std::copy_n(threat.aim,3,value.aim);std::copy_n(brute.saved_position,3,value.destination);value.side=brute_side;
        auto h=brute_chase_services();result=uc_brute_chase_step(&value,&h);if(brute_stage==2)brute.entered=value.entered;
        brute_charge=value.charge;brute_side=value.side;std::copy_n(value.destination,3,brute.saved_position);
    }else{auto h=brute_combat_services();result=uc_brute_combat_step(&brute,&h,(uint32_t)brute_stage);}
    if(!result)std::copy_n(s.entity().transform,16,entity.transform);return result;
}
void Behaviour::exit(){
    if(kind==Loop){auto h=loop_services();require(!uc_character_loop_exit(&loop,&h),"NPC authored loop exit");}
    else if(kind==HealDialogue){auto h=heal_dialogue_services();require(!uc_medic_heal_dialogue_exit(&heal_dialogue,&h),"MedicHealPlayer exit168F20");}
    else if(kind==HealPlayer){auto h=heal_player_services();require(!h.remove(this,heal_player.pillbox),"HealPlayer pillbox exit");heal_player.pillbox=-1;
        actor->navigator->stop();actor->movement->stop();actor->movement->graph.stop_aim();actor->unbind_trigger(&heal_player.delivered);actor->unbind_trigger(&heal_player.spawn);base_exit();}
    else if(kind==Overlay){actor->movement->graph.cop_stance(0);actor->movement->graph.stop_aim();base_exit();}
    else if(kind==Follow){actor->navigator->stop();actor->movement->stop();base_exit();}
    else if(kind==Interaction){actor->controller_tasks.clear();actor->movement->graph.stop_look();base_exit();}
    else if(kind==Scripted){actor->controller_tasks.clear();base_exit();}
    else if(kind==PlayAnim){auto&s=*actor;require(s.services.loop_sound&&!s.services.loop_sound(s.services.owner,*s.owner,loop_bank.data(),loop_bank.size(),"Stop"),"NPC one-shot soundStop144B5C");
        s.unbind_trigger(&animation_end);if(s.animation_locks)--s.animation_locks;base_exit();}
    else if(kind==Attack){if(condition_child&&condition_child->active)condition_child->exit();condition_child=nullptr;if(cover)cover->close(attack);base_exit();}
    else if(kind==BlazerAttack){actor->navigator->stop();actor->unbind_trigger(&blazer_melee);actor->unbind_trigger(&blazer_throw);actor->attention.reset();base_exit();}
    else if(kind==BruteAttack){uc_npc_melee_cancel(actor->root->melee.get());actor->navigator->stop();actor->navigator->clear_influences();
        actor->unbind_trigger(&brute_throw);actor->unbind_trigger(&brute_restock);actor->attention.reset();base_exit();}
    else base_exit();
}
void Behaviour::base_exit(){
    require(active&&actor,"NPC behavior exit lifetime");if(replacement){auto*other=replacement;replacement=nullptr;other->parent=nullptr;if(other->active)other->exit();}if(child&&child->active)child->exit();child=nullptr;if(condition_child&&condition_child->active)condition_child->exit();condition_child=nullptr;request_remove(*world,entity);
    if(parent&&parent->replacement==this)parent->replacement=nullptr; //326B0: all admitted suspension classes have no-op A4.
    if(parent&&parent->child==this){parent->child=nullptr;parent->child_priority=false;}if(parent&&parent->condition_child==this)parent->condition_child=nullptr;parent=nullptr;
    if(actor->foreground==this)actor->foreground=nullptr;
    if(actor->root&&actor->root->medic_speech==this)actor->root->medic_speech=nullptr;
    move.controller=nullptr;move.actor=nullptr;move.active=0;appearance.controller=nullptr;appearance.actor=nullptr;appearance.active=0;
    loop.controller=nullptr;loop.actor=nullptr;loop.active=0;
    actor=nullptr;active=false;
}
int Behaviour::enter(CharacterState &s){
    require(!active&&!actor,"NPC duplicate behavior entry");actor=&s;active=true;
    if(kind==HealDialogue){heal_dialogue.phase=0;heal_dialogue.head_only=0;return 0;} //1689E0; admitted foreground lacks head aim only.
    if(kind==HealPlayer){heal_player.phase=0;s.bind_trigger("GunHideTrigger",&heal_player.delivered);s.bind_trigger("GunUnHideTrigger",&heal_player.spawn);return 0;}
    if(kind==StepAside){step_aside.first=1;return 0;} //10A970.
    if(kind==FollowPlayer){std::copy_n(s.entity().transform+12,3,player_follow.last_target);s.navigator->stop();
        player_follow.moving=player_follow.turned=0;side_step.reset(s.services.player_id(s.services.owner),7.5f);return 0;} //193710,Wait at start0.
    if(kind==Restrained){
        //136AB0 switches the body to its source ragdoll component, interrupts
        //speech, drops held slots and applies2A3760/2A3590 at scale1.
        s.ragdoll_begin();s.navigator->stop();s.movement->stop();s.movement->graph.stop_aim();s.movement->graph.stop_look();
        s.controller_tasks.clear();s.dialogue->cancel();s.dialogue_face=s.dialogue_look=false;s.pending_events={};s.hit_time=0;
        if(s.root->thrown)s.root->thrown->drop();
        s.dialogue->say(int32_t(0x80000927u),100);
        require(uc_ragdoll_forces(s.hit_response.impulse,1.f,s.corpse_forces),"RestrainedBehaviour authored forces136B56");
        generic_event=1;s.model->reset_presentation();return 0;
    }
    if(kind==MedicCheck){uc_medic_heal_init(&heal_check);return 0;}
    if(kind==Heartbeat){auto h=heartbeat_services();return uc_character_heartbeat_enter(&heartbeat,&h);}
    if(kind==Interaction){
        require(!script_runner.bind(entity,world->metadata((uint32_t)entity.source),world->script_services(),s.controller_tasks),"NPC interaction script binding");
        if(entity.flags&8){auto&r=*world->context().registration;*r.creation={0,1,entity.parent};
            require(!r.insert_record(r.owner,&entity.view.target,entity.id,1),"NPC interactive update insertion");entity.flags&=~8u;}
        interaction.phase=0;return 0;
    }
    if(kind==Scripted){
        require(!script_runner.bind(entity,world->metadata((uint32_t)entity.source),world->script_services(),s.controller_tasks),"NPC scripted behavior32710 binding");
        if(entity.flags&8){auto&r=*world->context().registration;*r.creation={0,1,entity.parent};
            require(!r.insert_record(r.owner,&entity.view.target,entity.id,1),"NPC scripted update207F00 insertion");entity.flags&=~8u;}
        return script_runner.request(script_type,script_index,entity.parent!=-1); //120AC4/329F0.
    }
    if(kind==GenericExit){
        int inserted=0;require(!uc_entity_listener_register(&world->registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions),world->listeners(),&inserted)&&inserted,"NPC generic exit listener18C97D");
        const UCEntityEventSubscription event{9,[](void*p,const void*payload,uint32_t*){auto&b=*static_cast<Behaviour*>(p);if(!b.actor)return 0;require(payload,"NPC generic exit event9 payload");
            uint32_t type;std::memcpy(&type,static_cast<const uint8_t*>(payload)+28,4);if(type!=4)return 0;
            float point[3];std::memcpy(point,payload,12);const float*a=b.actor->entity().transform+12;
            float x=point[0]-a[0],y=point[1]-a[1],z=point[2]-a[2];if((z*z+x*x)+y*y<40000.f)b.generic_event=1;return 0;}};
        return uc_entity_subscriptions_append(subscriptions,&event); //18BD40; this profile does not enable the gas predicate.
    }
    if(kind==Face){float point[3];bool found=false;
        if(uint32_t(follow_source)&0xF0000u){const UCLevelNode*n=nullptr;auto status=uc_level_node_find(world->script_services().logic,uint32_t(follow_source),&n);
            require(status==UC_LEVEL_OK||status==UC_LEVEL_NOT_FOUND,"NPC face authored node");if(n){std::copy_n(n->position,3,point);found=true;}}
        else{int32_t id=-1;require(!uc_level_entity_lookup(&world->slots(),uint32_t(follow_source),&id),"NPC face entity slot");const float*m=s.live_world(id);if(m){std::copy_n(m+12,3,point);found=true;}}
        if(found){point[1]+=player_range;s.movement->graph.stop();if(turn_completion)turn_completion->release();turn_completion=new CharacterCompletion;
            s.movement->graph.aim(point,narrow_speech?0:2,&turn_completion->callback);}return 0; //1A6D0D/1A6D68.
    }
    if(kind==PlayAnim){
        require(s.services.loop_sound&&!s.services.loop_sound(s.services.owner,*s.owner,loop_bank.data(),loop_bank.size(),"Start"),"NPC one-shot soundStart144CFA");
        s.movement->graph.stop_aim();if(turn_completion)turn_completion->release();turn_completion=new CharacterCompletion;
        s.movement->graph.queue_clip(loop_clip,&turn_completion->callback,hold_last);
        s.bind_trigger("AnimEnd",&animation_end);++s.animation_locks;return 0; //144CD0; direct clip2ADE70, counterDDD70.
    }
    if(kind==Loop){auto h=loop_services();return uc_character_loop_enter(&loop,&s.controller,&h);}
    if(kind==Move||kind==Walk){auto h=s.move_services();return uc_character_move_enter(&move,&s.controller,&h);}
    if(kind==LineOfSight){require(!uc_level_entity_lookup(&world->slots(),uint32_t(follow_source),&follow.target),"LOS target189590");return 0;}
    if(kind==Follow){
        if(condition!=-1){child=s.enter_child(condition);child->parent=this;}
        require(!uc_level_entity_lookup(&world->slots(),(uint32_t)follow_source,&follow.target),"Follow live target slot");
        std::copy_n(s.entity().transform+12,3,follow.last_target);health_snapshot=s.entity().actor.health9C;
        s.navigator->stop();side_step.reset(follow.target,7.5f);follow.moving=0;follow.actor_world=s.entity().transform;return 0;
    }
    if(kind==CopMove){
        if(condition!=-1)child=s.enter_child(condition);move.index=0;move.phase=0;s.navigator->stop();
        auto*next=make_overlay(*world,entity.id);
        if(child&&child->active)child->exit();child=next;child->parent=this;return child->enter(s);
    }
    if(kind==Attack){
        if(condition!=-1){condition_child=s.enter_child(condition);condition_child->parent=this;}
        if(!child){child=make_overlay(*world,entity.id);child->overlay.allow_relax=can_relax;child->parent=this;require(!child->enter(s),"Cop attack overlay entry");}
        require(s.root&&s.root->gun,"Cop attack actual gun controller");weapon_controller=s.root->gun.get();
        require(s.services.cover_points,"Cop attack cover registry");
        cover=std::make_unique<NativeCopAttackCover>(*world,*s.services.cover_points,*s.services.navigation,*s.navigator,
            s.entity().id,entity.id,cover_source);
        attack.actor_world=s.entity().transform;return 0;
    }
    if(kind==BruteAttack){
        require(s.root&&s.root->brute&&s.root->melee,"Brute attack actual melee controller");
        if(condition!=-1){child=s.enter_child(condition);child->parent=this;}
        s.navigator->stop();std::copy_n(s.entity().transform+12,3,brute.saved_position);
        if(!loop_clip){const char*name="Assets_CharacterAnimation/CharacterSpecific/CCA_BurnerBrute/cca_burnerbrute_throw_cleaver.kfa";
            size_t index=find_resource(s.archive,name);BytesOwner bytes;require(index!=SIZE_MAX&&uc_archive_read(s.archive,index,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,"Brute authored throw clip47CE70");
            loop_clip=std::make_shared<NativeFirstpersonClip>();loop_clip->bytes.assign(bytes.value.data,bytes.value.data+bytes.value.size);
            UCAnimationClip clip{};std::unique_ptr<UCAnimationClip,decltype(&uc_animation_clip_free)> cleanup(&clip,uc_animation_clip_free);
            require(uc_animation_clip_load(loop_clip->bytes.data(),loop_clip->bytes.size(),&clip)==UC_ANIMATION_OK&&uc_animation_annotations_load(&clip,&loop_clip->annotations)==UC_ANIMATION_OK,"Brute throw clip annotations");}
        s.bind_trigger("ThrownWeaponThrow",&brute_throw);s.bind_trigger("ThrownWeaponRestock",&brute_restock);brute.throws_left=3;
        uc_npc_melee_cancel(s.root->melee.get());brute_transition(2);brute_side.left=1;brute_side.near_line=0;brute_side.near_time=brute_side.away_time=0;
        auto perception=threat_services();require(!native_character_threat_begin(threat,perception,false,false),"Brute threat constructor2FCF0");
        auto chase=brute_chase_services();require(!uc_brute_side_reset(&brute_side,&chase),"Brute entry sidestep1D29DD");
        brute.throw_cooldown=0;brute.route=0;brute_nav_paused=0;brute.since_attack=0;
        int32_t mission=0;require(s.services.mission_ordinal&&!s.services.mission_ordinal(s.services.owner,&mission),"Brute mission54BF0");brute.can_charge=mission>=1;s.attention_begin();return 0;
    }
    if(kind==BlazerAttack){
        if(condition!=-1){child=s.enter_child(condition);child->parent=this;}
        auto h=threat_services();require(!native_character_threat_begin(threat,h,true,false),"Blazer threat constructor2FCF0");
        std::copy_n(s.entity().transform+12,3,attack_position);position_ready=true;position_changed=false;
        NativeCharacterAttackPosition::Services position{};position.owner=&s;
        position.target=[](void*p){return static_cast<CharacterState*>(p)->current_target();};
        position.position=[](void*p,int32_t id)->const float*{const float*m=static_cast<CharacterState*>(p)->live_world(id);return m?m+12:nullptr;};
        position.random=[](void*p,float lo,float hi){auto&s=*static_cast<CharacterState*>(p);int32_t value;require(!uc_game_random_next(s.random,&value),"Blazer positioning shared random");return (double(value)*(double(hi)-lo))*0x1p-31+lo;};
        position.ray=[](void*p,const float*from,const float*to,uint32_t type,uint32_t region,uint32_t mask,float impulse,float*out){auto&s=*static_cast<CharacterState*>(p);
            require(!s.services.refresh_collision(s.services.owner),"Blazer positioning collision refresh");UCCollisionResult hit{};int32_t id=-1;
            int found=s.services.collision->query(from,to,type,region,mask,impulse,hit,id);if(found==1)std::copy_n(hit.point,3,out);return found;};
        positioning=std::make_unique<NativeCharacterAttackPosition>(*world->script_services().logic,
            *s.services.navigation,position,s.entity().id,uint16_t(s.entity().actor.group94),1000.f);positioning->ranges(2000,3000);
        require(s.root&&s.root->thrown,"Blazer actual thrown handler2EE80");weapon_controller=s.root->thrown.get();
        s.bind_trigger("MeleeAttackStartHit",&blazer_melee);blazer_hit=0;blazer_transition(threat.current==-1?1:2);
        s.bind_trigger("ThrownWeaponThrow",&blazer_throw);blazer_cooldown=.5f;blazer_first=1;s.attention_begin();return 0;
    }
    if(kind==Overlay){
        require(s.root&&s.root->gun,"Cop overlay actual weapon controller");weapon_controller=s.root->gun.get();compliance_allowed=s.root->compliance;
        int inserted=0;require(!uc_entity_listener_register(&world->registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions),world->listeners(),&inserted)&&inserted,"Cop overlay listener entry");
        const UCEntityEventSubscription events[]={{0x4c,[](void*p,const void*,uint32_t*){return uc_cop_overlay_event(&static_cast<Behaviour*>(p)->overlay,0x4c);}},
            {0x49,[](void*p,const void*,uint32_t*){return uc_cop_overlay_event(&static_cast<Behaviour*>(p)->overlay,0x49);}}};
        for(const auto&e:events)require(!uc_entity_subscriptions_append(subscriptions,&e),"Cop overlay event subscription");return 0;
    }
    if(kind==Appearance){UCCharacterAppearanceServices h{};h.owner=this;h.random=s.random;
        h.has_body=[](void*p,void*a){auto&b=*static_cast<Behaviour*>(p);require(a==b.actor,"NPC appearance actor");return b.actor->body?1:0;};
        h.skin=[](void*p,void*a,int32_t skin){auto&b=*static_cast<Behaviour*>(p);require(a==b.actor,"NPC skin actor");b.actor->model->select_skin(skin);};
        //1D3770 overrides Use head gear1=true. The other three are genuine
        //missing Boolean properties and receive the caller's default zero.
        h.property=[](void*p,UCCharacterLoopController*c,const char*name)->int{auto&b=*static_cast<Behaviour*>(p);require(c==&b.actor->controller,"NPC appearance controller");
            require(!std::strcmp(name,"Use head gear 1")||!std::strcmp(name,"Use head gear 2")||!std::strcmp(name,"Use head gear 3")||!std::strcmp(name,"Dont use head gear"),"NPC appearance property");return !b.actor->root->cop&&!std::strcmp(name,"Use head gear 1");};
        h.equipment=[](void*p,void*a,unsigned group,int32_t value){auto&b=*static_cast<Behaviour*>(p);require(a==b.actor&&group<9,"NPC equipment owner");b.actor->equipment.enabled[group]=uint8_t(value);};
        h.exit=[](void*p,UCCharacterAppearance*){static_cast<Behaviour*>(p)->exit();};return uc_character_appearance_enter(&appearance,&s.controller,&h);
    }
    if(kind==Named){
        require(named==2||named==4||named==5||named==7,"NPC named character profile");s.named_character=named;
        s.model->select_skin(named==5?1:named==7?2:0);
        //162FB0/2AADB0 remaps the model material skin, then enables equipment.
        //The material operation65360 does not reset equipment visibility.
        if(named==2)s.equipment.enabled[2]=1;else if(named==7)s.equipment.enabled[0]=1;exit();
    }
    return 0; //32660 scalar entry; secondary owners wait for their actual caller.
}
UCCopOverlayCombatServices Behaviour::overlay_services(){
    auto&s=*actor;require(s.root&&s.root->gun&&weapon_controller==s.root->gun.get()&&s.services.npc_gun,"Cop overlay retained weapon controller");
    UCCopOverlayCombatServices h{};auto&c=h.common;c.owner=this;h.actor=s.entity().id;h.root=s.root->entity.id;
    h.child=child&&child->active?child->entity.id:-1;h.compliance_allowed=compliance_allowed;
    c.player_target=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t result=-1;
        int found=s.services.target_property(s.services.owner,s.services.player_id(s.services.owner),"target id",&result);require(found>=0,"Cop player target property");return found?result:-1;};
    c.target=[](void*p){return static_cast<Behaviour*>(p)->actor->current_target();};
    c.acquire=[](void*p,uint8_t prefer,uint8_t visible){static_cast<Behaviour*>(p)->actor->acquire(prefer!=0,visible!=0);return 0;};
    c.aim_stop=[](void*p){static_cast<Behaviour*>(p)->actor->movement->graph.stop_aim();return 0;};
    c.stance_busy=[](void*p,uint8_t*out){*out=static_cast<Behaviour*>(p)->actor->movement->graph.cop_stance_busy();return 0;};
    c.weapon_mode=[](void*p,int32_t*out){*out=static_cast<Behaviour*>(p)->actor->movement->graph.cop_weapon_mode();return 0;};
    c.stance=[](void*p,int32_t mode,uint8_t transition,void*callback){auto&s=*static_cast<Behaviour*>(p)->actor;require(!callback,"Cop stance external clip");
        require(s.head_bone>=0&&s.model->prepared.ready&&s.model->prepared.world_valid[s.head_bone],"Cop stance posed head");
        s.movement->graph.cop_stance(mode,transition!=0,s.model->prepared.world[s.head_bone][13]<130.f);return 0;};
    h.player=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.player_id(s.services.owner);};
    h.position=[](void*p,int32_t id,float*out){const float*m=static_cast<Behaviour*>(p)->actor->live_world(id);if(!m)return 0;std::copy_n(m+12,3,out);return 1;};
    h.integer=[](void*p,int32_t id,const char*name,int32_t fallback){auto&s=*static_cast<Behaviour*>(p)->actor;
        if(id==s.root->entity.id){require(!std::strcmp(name,"order id"),"Cop root integer property");return s.root->order_id;}
        int32_t result=0;int found=s.services.target_property(s.services.owner,id,name,&result);require(found>=0,"Cop target integer property");return found?result:fallback;};
    h.boolean=[](void*p,int32_t id,const char*name,uint8_t fallback)->uint8_t{auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;
        if(b.child&&id==b.child->entity.id)throw std::runtime_error("Cop overlay child controlling-attitude property requires its real owner");
        int32_t result=0;int found=s.services.target_property(s.services.owner,id,name,&result);require(found>=0,"Cop target Boolean property");return found?uint8_t(result):fallback;};
    h.number=[](void*,int32_t id,const char*name,float fallback)->double{require(!std::strcmp(name,"standing time"),"Cop overlay float property");
        require(id==-1,"Cop overlay child standing-time owner is unconnected");return fallback;};
    h.point=[](void*p,int32_t id,const char*name,float*out){auto&s=*static_cast<Behaviour*>(p)->actor;return s.services.target_vector_property(s.services.owner,id,name,out);};
    h.can_shoot=[](void*p,int32_t target){auto&s=*static_cast<Behaviour*>(p)->actor;unsigned answer=0;
        require(!uc_npc_gun_controller_eligible(s.root->gun.get(),target,0,0,s.services.npc_gun,&answer),"Cop GunController29470 eligibility");return int(answer);};
    h.visible=[](void*p,int32_t target){auto&s=*static_cast<Behaviour*>(p)->actor;const float offset[3]{};
        return s.services.visible_query(s.services.owner,*s.owner,target,-1,false,offset,false);};
    h.facing=[](void*p)->double{auto&s=*static_cast<Behaviour*>(p)->actor;int32_t target=s.current_target();const float*m=s.live_world(target);
        return m?uc_cop_overlay_facing(s.entity().transform+12,s.entity().transform+8,m+12):1.;};
    h.task_status=[](void*p){auto*t=static_cast<Behaviour*>(p)->overlay_task;return t?t->status:-1;};
    h.gun_status=[](void*p,unsigned offset){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t answer=0;
        require(!uc_npc_gun_controller_status(s.root->gun.get(),offset,&answer),"Cop GunController status");return answer;};
    h.reload=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;s.movement->graph.cop_reload();
        require(!uc_npc_gun_controller_reload(s.root->gun.get(),0,1,s.services.npc_gun),"Cop GunController reload");return 0;};
    h.target_contact=[](void*p,int32_t target){auto&b=*static_cast<Behaviour*>(p);uint32_t payload=(uint32_t)b.actor->entity().id,sender=(uint32_t)b.entity.id;uint8_t handled=0;
        return uc_entity_event_send(&b.world->registry(),target,0x35,&payload,sizeof payload,&sender,&handled);};
    h.melee_response=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t target=s.current_target();if(target==-1)return 0;
        const float*m=s.live_world(target);if(!m)return 0;int32_t theirs=-1;int found=s.services.target_property(s.services.owner,target,"target ID",&theirs);
        require(found>=0,"Cop tazer target-ID property");if(!found||theirs!=s.entity().id)return 0;
        float x=m[12]-s.entity().transform[12],y=m[13]-s.entity().transform[13],z=m[14]-s.entity().transform[14];
        if(40000.f>((x*x+y*y)+z*z))throw std::runtime_error("Cop dynamic CopTazerCharacter close-target transition is unconnected");return 0;};
    h.aim=[](void*p,const float*point){static_cast<Behaviour*>(p)->actor->movement->graph.aim(point,0);return 0;};
    h.player_in_line=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;int32_t player=s.services.player_id(s.services.owner);const float*pm=s.live_world(player);if(!pm)return 0;
        const float*tm=s.live_world(s.current_target());if(!tm)return 0;float muzzle[3];const auto&g=*s.services.npc_gun;
        require(g.fire_position&&!g.fire_position(g.owner,s.root->gun->weapon,muzzle),"Cop original gun muzzle");return uc_cop_overlay_player_in_line(muzzle,tm+12,pm+12);};
    h.fire=[](void*p,int32_t target,float scale){auto&b=*static_cast<Behaviour*>(p);auto&s=*b.actor;
        if(b.overlay_task)b.overlay_task->release();b.overlay_task=new CharacterCompletion;s.movement->graph.cop_shoot(&b.overlay_task->callback);
        unsigned answer=0;require(!uc_npc_gun_controller_fire(s.root->gun.get(),target,0,1,scale,s.services.npc_gun,&answer),"Cop GunController29F80 shot");return 0;};
    h.say=[](void*p,int32_t code,int32_t priority){static_cast<Behaviour*>(p)->actor->dialogue->say(code,priority);return 0;};
    h.random=[](void*p,float lo,float hi){int32_t value=0;require(!uc_game_random_next(static_cast<Behaviour*>(p)->actor->random,&value),"Cop overlay shared random");return (double(value)*(double(hi)-lo))*0x1p-31+lo;};
    h.coin=[](void*p)->uint8_t{int32_t value=0;require(!uc_game_random_boolean(static_cast<Behaviour*>(p)->actor->random,&value),"Cop overlay random Boolean");return uint8_t(value);};
    h.clear_target=[](void*p){static_cast<Behaviour*>(p)->actor->targets.current=-1;return 0;}; //32DE0.
    h.base=[](void*p,uint8_t){auto&b=*static_cast<Behaviour*>(p);std::memcpy(b.entity.transform,b.actor->entity().transform,64);return 0;};return h;
}
int Behaviour::foreground(bool expensive){
    require(active&&actor,"NPC active foreground owner");
    if(replacement)return replacement->foreground(expensive); //323E0 E0 precedes ordinary priority child.
    if(child_priority&&child)return child->foreground(expensive); //323E0 priority-one child replaces this update.
    if(kind==StepAside){auto h=step_aside_services();return uc_character_step_aside_update(&step_aside,&h);}
    if(kind==HealDialogue){auto h=heal_dialogue_services();int result=uc_medic_heal_dialogue_update(&heal_dialogue,*world->script_services().delta,&h);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==HealPlayer){auto h=heal_player_services();return uc_medic_heal_player_update(&heal_player,*world->script_services().delta,&h);}
    if(kind==FollowPlayer){auto h=player_follow_services();int result=uc_character_player_follow_update(&player_follow,&h);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==Restrained){generic_event=0;std::copy_n(actor->entity().transform,16,entity.transform);return 0;} //136BB0 ->32680; no timed recovery.
    if(kind==Scripted){auto&s=*actor;if(!s.controller_tasks.busy()&&range_next!=-1)s.transition(range_next);
        std::copy_n(s.entity().transform,16,entity.transform);return 0;} //120AF0.
    if(kind==Face){auto&s=*actor;if((!turn_completion||turn_completion->status)&&range_next!=-1){const int32_t next=range_next;range_next=-1;s.transition(next);}
        std::copy_n(s.entity().transform,16,entity.transform);return 0;} //1A6D80.
    if(kind==PlayAnim){auto&s=*actor;if(animation_end||(turn_completion&&turn_completion->status))s.transition(range_next);
        std::copy_n(s.entity().transform,16,entity.transform);return 0;} //144B90 includes completion and cancellation.
    if(kind==GenericExit){auto&s=*actor;const auto&h=world->script_services();
        const bool bullet=(s.bullet_by148&4)!=0;s.bullet_by148=0; //18BF12: consume the retained mask, even when flag is already set.
        int flags=uc_character_loop_flags(h.flags,h.flag_count,(int32_t)nodes.size(),nodes.data());require(flags>=0,"NPC generic exit authored flags");
        if(bullet||flags){s.transition(range_next);if(active)exit();return 0;}
        std::copy_n(s.entity().transform,16,entity.transform);return 0;}
    if(kind==JoinTeam){actor->movement->stop();uint8_t handled=0;uint32_t from=(uint32_t)entity.id;
        require(actor->root&&!uc_entity_event_send(&world->registry(),actor->root->entity.id,74,&team_join,sizeof team_join,&from,&handled),"JoinPlayersTeam source root event74");exit();return 0;}
    if(kind==MedicCheck){auto h=heal_services();return uc_medic_heal_update(&heal_check,float(expensive),&h);}
    if(kind==Heartbeat){auto h=heartbeat_services();return uc_character_heartbeat_update(&heartbeat,uint8_t(expensive),&h);}
    if(kind==DisableHeal){auto&s=*actor;
        if(s.root&&s.root->entity.id!=0){uint8_t value=uint8_t(can_heal),handled=0;uint32_t sender=uint32_t(entity.id);
            require(!uc_entity_event_send(&world->registry(),s.root->entity.id,115,&value,1,&sender,&handled),"Medic autoheal event");
            if(range_next!=-1)s.transition(range_next);}
        std::copy_n(s.entity().transform,16,entity.transform);return 0; //16B0E0 always32680 after325A0.
    }
    if(kind==Invulnerable){auto&s=*actor;if(s.root&&uc_entity_event_lookup(&world->registry(),s.root->entity.id))
        uc_character_invulnerable(&s.root->protection,&s.reaction_suppression,invulnerable[0],invulnerable[1],invulnerable[2],invulnerable[3]);
        if(range_next!=-1)s.transition(range_next);return 0;}

    if(kind==Interaction){auto h=interaction_services();return uc_character_interaction_update(&interaction,*world->script_services().delta,expensive,&h);}
    if(kind==None){actor->transition(-1);return 0;} //15DF00 ->32CB0(-1), not a persistent inert owner.
    if(kind==FlagCondition){auto&s=*actor;const auto&h=world->script_services();
        int ready=uc_character_loop_flags(h.flags,h.flag_count,(int32_t)nodes.size(),nodes.data());require(ready>=0,"NPC flag condition array");
        if(ready){s.transition(range_next);if(active)exit();return 0;}
        std::memcpy(entity.transform,s.entity().transform,64);return 0;}
    if(kind==LineOfSight){auto&s=*actor;const float zero[3]{};
        //1895C0: E05C0(target, cosine bits3F35C28F, false, zero, false).
        uint32_t bits=0x3f35c28f;float cosine;memcpy(&cosine,&bits,4);
        int visible=s.services.visible_query(s.services.owner,*s.owner,follow.target,cosine,false,zero,false);
        require(visible>=0,"LOS visibility1895C0");if(visible){s.transition(range_next);if(active)exit();return 0;}
        memcpy(entity.transform,s.entity().transform,64);return 0;}
    if(kind==PlayerRange){auto&s=*actor;auto*t=uc_entity_event_lookup(&world->registry(),s.services.player_id(s.services.owner));
        if(t){auto&c=world->context();auto*v=c.resolve(c.owner,t);require(v&&v->transform,"NPC range player transform");
            float x=s.entity().transform[12]-v->transform[12],y=s.entity().transform[13]-v->transform[13],z=s.entity().transform[14]-v->transform[14];
            float squared=(z*z+x*x)+y*y;if(player_range*player_range>squared){s.transition(range_next);if(active)exit();return 0;}}
        std::memcpy(entity.transform,s.entity().transform,64);return 0;}
    if(kind==Follow){auto h=follow_services();int result=uc_character_follow_update(&follow,&h);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==Attack){auto h=attack_services();int result=uc_cop_attack_update(&attack,&h);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==BlazerAttack){int result=blazer_update(expensive);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==BruteAttack){int result=brute_update(expensive);if(!result&&active&&child)result=child->foreground(expensive);return result;}
    if(kind==CopMove){auto&s=*actor;auto h=s.move_services();
        if(move.phase==2){s.transition(move.next);move.next=-1;return 0;}
        if(move.phase==0){if(move.index>=move.count)move.phase=2;
            else{const float*point=h.resolve(h.owner,move.nodes[move.index++]);if(point){h.target(h.owner,&s.controller,point);move.phase=1;}}}
        if(move.phase==1&&s.run(true)==4)move.phase=0;
        if(expensive&&s.targets.current==-1)s.movement->graph.stop_aim();std::memcpy(entity.transform,s.entity().transform,64);
        if(child)return child->foreground(expensive);return 0;
    }
    if(kind==Overlay){auto h=overlay_services();return uc_cop_overlay_update(&overlay,*world->script_services().delta,expensive,&h);}
    if(kind==Loop){auto h=loop_services();int r=uc_character_loop_update(&loop,&h);
        //323E0 follows the CURRENT priority-zero child after its parent update.
        if(!r&&active&&child)r=child->foreground(expensive);return r;}
    if(kind==Move||kind==Walk){auto h=actor->move_services();if(kind==Move)return uc_character_move_update(&move,&h);
        UCCharacterWalkServices walk{};walk.owner=actor;
        walk.status=[](void*p){return static_cast<CharacterState*>(p)->navigator->status();};
        walk.waypoint=[](void*p,float*out){auto&s=*static_cast<CharacterState*>(p);std::copy_n(s.navigator->waypoint(s.entity().transform+12),3,out);};
        walk.near_target=[](void*p){return int32_t(static_cast<CharacterState*>(p)->navigator->near_goal());};
        walk.request=[](void*p,const float*point,int32_t mode,int32_t near_target){auto&s=*static_cast<CharacterState*>(p);NativeCharacterMoveTarget target;std::copy_n(point,3,target.position);target.mode=mode;target.near_target=uint8_t(near_target);s.movement->request(target);};
        walk.stop=[](void*p){static_cast<CharacterState*>(p)->movement->stop();};
        walk.clear_heading=[](void*p){static_cast<CharacterState*>(p)->movement->graph.stop_aim();};
        return uc_character_walk_update(&move,&h,&walk);
    }
    if(kind==NoSecondary||kind==Condition){exit();return 0;} //19B5C0.
    if(kind==MedicSpeech){auto&s=*actor;if(!(s.actor_status&0x20007)&&!s.dialogue->playing()){s.dialogue->say(1,50);exit();}return 0;} //1690E0
    if(kind==Speech)throw std::runtime_error("CopGenericSpeech DED50 emote request requires NPC dialogue owner");
    require(kind==Delete,"NPC appearance remained active after exit");
    //1B3DA0 queries the real33020 controller task list, not an animation slot.
    if(actor->controller_tasks.empty())request_remove(*world,actor->entity());std::memcpy(entity.transform,actor->entity().transform,64);return 0;
}
//1687E0 suffix; its allocation outlives the root until the client completes
//the world's deferred retirement. Parent lookup guards every actor access.
struct MedicClient {
    NativeMissionWorld *world;Root *root;int32_t root_id;const UCEntityEventTarget *root_identity;
    NativeMissionCharacterServices services;NativeOrderClient *client=nullptr;
    UCMedicOrder order{0,0,-1};
    Root *live()const{return uc_entity_event_lookup(&world->registry(),root_id)==root_identity?root:nullptr;}
    UCMedicOrderServices hooks(){UCMedicOrderServices h{};h.owner=this;
        h.root_boolean=[](void*p,const char*n,uint8_t*out,uint8_t*found){auto*s=static_cast<MedicClient*>(p);auto*r=s->live();*found=0;if(!r)return 0;
            int32_t value=0;int result=r->actor->owner->property(n,&value);if(result<0)return 1;*found=result!=0;*out=value!=0;return 0;};
        h.manager_integer=[](void*p,const char*n,int32_t*out,uint8_t*found){auto&s=*static_cast<MedicClient*>(p);auto*m=NativeOrderManager::find(*s.world,s.client->manager());*found=0;return m?m->integer(n,out,found):0;};
        h.player_health=[](void*p,float*out,uint8_t*found){auto&s=*static_cast<MedicClient*>(p);require(s.services.target_scalar_property&&s.services.player_id,"Medic player health owner");double value;
            int result=s.services.target_scalar_property(s.services.owner,s.services.player_id(s.services.owner),"percent health remaining",&value);if(result<0)return 1;*found=result!=0;if(*found)*out=float(value);return 0;};
        h.tutorial=[](void*p,int32_t command){auto&s=*static_cast<MedicClient*>(p);require(s.services.medic_tutorial,"Medic tutorial profile owner");return s.services.medic_tutorial(s.services.owner,uint32_t(command));};
        h.complete=[](void*p,int32_t command){return static_cast<MedicClient*>(p)->client->complete(command);};return h;
    }
    int choose(int32_t value){auto h=hooks();return uc_medic_order_choose(&order,value,&h);}
    int attached(){
        const NativeOrderAction actions[]={
            {1,"follow me",2,"Hud\\HUD_Orders_Follow_Fireman.shader",0},
            {2,"wait",3,"Hud\\HUD_Orders_Cover_Fireman.shader",0},
            {3,"heal person",0,"Hud\\HUD_Icon_Heal_NPC.shader",1},
            {4,"heal player",0,"Hud\\HUD_Icon_Heal_Player.shader",1},
            {7,"open door",0,"HUD\\HUD_Operate_Door.shader",1}};
        for(const auto&a:actions)if(int result=client->advertise(a))return result;
        return choose(1);
    }
    static int create(Root&r,int32_t*out){
        auto*s=new MedicClient{r.world,&r,r.entity.id,&r.entity.identity,r.actor->services};
        NativeOrderClientCallbacks h{};h.owner=s;
        h.accept=[](void*,int32_t,const float*,uint8_t*out){*out=1;return 0;}; //15DFC0.
        h.attached=[](void*p){return static_cast<MedicClient*>(p)->attached();};
        h.choose=[](void*p,int32_t value){return static_cast<MedicClient*>(p)->choose(value);};
        h.completed=[](void*p){static_cast<MedicClient*>(p)->order.command=0;return 0;}; //112020.
        h.manager_position=[](void*p,int32_t id,float*out,uint8_t*found){auto&s=*static_cast<MedicClient*>(p);
            auto*m=NativeOrderManager::find(*s.world,id);*found=0;return m?m->vector("Position",out,found):0;};
        h.integer=[](void*p,const char*name,int32_t*out,uint8_t*found){auto&s=*static_cast<MedicClient*>(p);auto h=s.hooks();return uc_medic_order_integer(&s.order,name,out,found,&h);};
        h.vector=[](void*p,const char*name,float*out,uint8_t*found){auto&s=*static_cast<MedicClient*>(p);*found=0;
            if(std::strcmp(name,"Position")&&std::strcmp(name,"Player Look Point"))return 0;
            auto*m=NativeOrderManager::find(*s.world,s.client->manager());return m?m->vector(name,out,found):0;};
        h.closed=[](void*p){delete static_cast<MedicClient*>(p);};
        h.removing=[](void*p){auto&s=*static_cast<MedicClient*>(p);require(s.services.tags,"Medic client tag owner");return s.services.tags(s.services.owner,s.client->entity.id,"MEDIC_ORDER_CLIENT",false);};
        int result=NativeOrderClient::create(*r.world,r.entity.id,"MedicOrderClient",h,&s->client);
        if(result){if(!s->client)delete s;return result;}
        require(s->services.tags&&!s->services.tags(s->services.owner,s->client->entity.id,"MEDIC_ORDER_CLIENT",true),"MedicOrderClient source tag");
        *out=s->client->entity.id;return 0;
    }
};
int Root::join_team(const UCMedicTeamJoin&j){
    require(actor&&active&&medic&&!plain_medic,"Team medic join event owner");UCMedicTeamServices h{};h.owner=this;
    h.heartbeat=[](void*p,const char*a,const char*b,const char*c){auto&s=*static_cast<Root*>(p)->actor;require(s.services.heartbeat_join,"Medic team heartbeat owner");return s.services.heartbeat_join(s.services.owner,s.entity().id,a,b,c);};
    h.client=[](void*p,int32_t*out){return MedicClient::create(*static_cast<Root*>(p),out);};
    h.copy_flags=[](void*p,int32_t count,const int32_t*flags,int32_t quit){auto&r=*static_cast<Root*>(p);r.quit_flags.clear();if(count)r.quit_flags.assign(flags,flags+count);r.quit_behaviour=quit;return 0;};
    h.condition=[](void*p,int32_t source){auto&r=*static_cast<Root*>(p);if(source!=-1)r.medic_children.push_back(r.actor->enter_child(source));return 0;};
    return uc_medic_team_join(&order_id,medic_policy.cues,&j,&h);
}
int Root::heals(){require(actor&&actor->services.medic_heals,"Medic shared heal owner");int32_t value=0;
    require(!actor->services.medic_heals(actor->services.owner,actor->named_character,&value),"Medic shared heals query");return value;}
UCNpcMeleeServices Root::melee_services(){UCNpcMeleeServices h{};h.owner=this;
    h.resolve=[](void*p,int32_t id){auto&r=*static_cast<Root*>(p);return uc_entity_event_lookup(&r.world->registry(),id)?1:0;};
    h.weapon_type=[](void*p,int32_t id,int32_t*out){auto&s=*static_cast<Root*>(p)->actor;require(s.services.melee_weapon_type,"Medic weapon type owner");return s.services.melee_weapon_type(s.services.owner,id,out);};
    h.retire=[](void*p,int32_t id){auto&s=*static_cast<Root*>(p)->actor;require(s.services.melee_weapon_flags,"Medic weapon flag owner");return s.services.melee_weapon_flags(s.services.owner,id,1);};
    h.watch=[](void*p,uint8_t*flag,const char*name,int32_t id){auto&s=*static_cast<Root*>(p)->actor;require(id==s.entity().id,"Medic melee event owner");s.bind_trigger(name,flag);return 0;};
    h.unwatch=[](void*p,uint8_t*flag){static_cast<Root*>(p)->actor->unbind_trigger(flag);return 0;};
    h.eligible=[](void*p,int32_t target,uint8_t*out){auto&r=*static_cast<Root*>(p);auto&s=*r.actor;require(r.melee&&s.services.melee_eligible,"NPC melee hit predicate2BDC0 owner");return s.services.melee_eligible(s.services.owner,*s.owner,*r.melee,target,out);};
    h.strike=[](void*p){auto&r=*static_cast<Root*>(p);auto&s=*r.actor;require(r.melee&&s.services.melee_strike,"NPC melee strike2BF60 owner");return s.services.melee_strike(s.services.owner,*s.owner,*r.melee);};return h;
}
UCMedicRootServices Root::medic_services(){require(actor,"Medic root actor binding");UCMedicRootServices h{};h.owner=this;
    h.actor_support=&actor->actor_support;h.character_type=&actor->named_character;h.order=&order_id;h.delta=world->script_services().delta;
    h.base_enter=[](void*p,void*c){auto&r=*static_cast<Root*>(p);require(c==&r.actor->controller&&!r.active,"Medic common root entry");r.active=true;return 0;};
    h.bank=[](void*p,unsigned mode,const char*name){static_cast<Root*>(p)->actor->dialogue->configure(mode,name);return 0;};
    h.body=[](void*p,unsigned type,const char*catalog,uint8_t a,uint8_t b){auto&s=*static_cast<Root*>(p)->actor;
        require(type==11&&s.character_class==11&&!a&&!b&&!std::strcmp(catalog,"Assets_CharacterAnimation\\CharacterSpecific\\CCA_Paramedic\\CCA_Paramedic_Animations.sttxt"),"Medic body construction profile");s.create_body();return 0;};
    h.initial=[](void*p,int32_t source,void*c){auto&s=*static_cast<Root*>(p)->actor;require(c==&s.controller,"Medic appearance controller");s.enter_skin(source);return 0;};
    h.support_create=[](void*p,uintptr_t*out){auto&r=*static_cast<Root*>(p);auto&s=*r.actor;require(!r.melee,"Medic duplicate support owner");r.melee=std::make_unique<UCNpcMeleeController>();
        uc_npc_melee_construct(r.melee.get(),s.entity().id,s.body.get(),&s.triggers,&s.character_audio);*out=reinterpret_cast<uintptr_t>(r.melee.get());return 0;};
    h.support_attach=[](void*p,uintptr_t token){auto&r=*static_cast<Root*>(p);auto&s=*r.actor;require(token==reinterpret_cast<uintptr_t>(r.melee.get())&&s.services.medic_weapon,"Medic support attachment owner");int32_t id=-1;
        require(!s.services.medic_weapon(s.services.owner,*r.world,*s.owner,&id),"Medic real Medkit2B180 factory");auto h=r.melee_services();return uc_npc_melee_bind(r.melee.get(),id,&h);};
    h.set_health=[](void*p,float value){auto&e=static_cast<Root*>(p)->actor->entity().actor;e.health9C=value;if(value>e.maximumA0)e.maximumA0=value;return 0;};
    h.integer=[](void*p,const char*name,int32_t,int32_t*out){auto&r=*static_cast<Root*>(p);require(!std::strcmp(name,"heals left"),"Medic root integer query");*out=r.heals();return 0;};
    h.tag=[](void*p,const char*name,uint8_t add){auto&s=*static_cast<Root*>(p)->actor;return s.services.tags(s.services.owner,s.entity().id,name,add!=0);};
    h.secondary_create=[](void*p,const char*name,int32_t*out){auto&r=*static_cast<Root*>(p);require(!std::strcmp(name,"CheckHealPlayer")&&!r.secondary,"Medic secondary class");r.secondary=Behaviour::make_medic_check(*r.world,r.entity.id);*out=r.secondary->entity.id;return 0;};
    h.start=[](void*p,int32_t id,void*c){auto&r=*static_cast<Root*>(p);require(c==&r.actor->controller&&r.secondary&&r.secondary->entity.id==id,"Medic secondary entry identity");return r.secondary->enter(*r.actor);};
    h.watch=[](void*p,uint8_t*flag,const char*name){static_cast<Root*>(p)->actor->bind_trigger(name,flag);return 0;};
    h.stop=[](void*p,int32_t id){auto&r=*static_cast<Root*>(p);if(!uc_entity_event_lookup(&r.world->registry(),id))return 0;Behaviour**slot=r.medic_speech&&r.medic_speech->entity.id==id?&r.medic_speech:&r.secondary;require(*slot&&(*slot)->entity.id==id,"Medic secondary exit identity");if((*slot)->active)(*slot)->exit();*slot=nullptr;return 0;};
    h.clear_children=[](void*p){auto&r=*static_cast<Root*>(p);for(auto*c:r.medic_children)if(c->active)c->exit();r.medic_children.clear();return 0;};
    h.flags=[](void*p,uint32_t child){auto&r=*static_cast<Root*>(p);if(!uc_entity_event_lookup(&r.world->registry(),(int32_t)child))return 0;
        auto*client=NativeOrderClient::find(*r.world,(int32_t)child);require(client,"Medic original order child20A170");return client->remove();};
    h.release_support=[](void*p,uintptr_t token,uint8_t deleting){auto&r=*static_cast<Root*>(p);require(deleting==1&&token==reinterpret_cast<uintptr_t>(r.melee.get()),"Medic support release identity");auto h=r.melee_services();int result=uc_npc_melee_close(r.melee.get(),&h);if(!result)r.melee.reset();return result;};
    h.unwatch=[](void*p,uint8_t*flag){static_cast<Root*>(p)->actor->unbind_trigger(flag);return 0;};
    h.base_exit=[](void*p){auto&r=*static_cast<Root*>(p);request_remove(*r.world,r.entity);r.actor->root=nullptr;r.actor=nullptr;r.active=false;return 0;};
    h.dialogue=[](void*p,const char*text,int32_t speaker){static_cast<Root*>(p)->actor->dialogue->request(text,speaker);return 0;};
    h.damage=[](void*,uint32_t,float)->int{throw std::runtime_error("Medic common health371B0 adapter is unconnected");};
    h.authored_secondary=[](void*p,int32_t source,void*c,int32_t*out){auto&r=*static_cast<Root*>(p);require(c==&r.actor->controller,"Plain medic speech controller");if(source==-1){*out=-1;return 0;}r.medic_speech=r.actor->enter_child(source);*out=r.medic_speech->entity.id;return 0;};return h;
}
UCCharacterHeartbeatServices Behaviour::heartbeat_services(){UCCharacterHeartbeatServices h{};h.owner=this;
    h.enter=[](void*){return 0;}; //32660 lifetime was established byBehaviour::enter.
    h.root_exists=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;return int(s.root&&uc_entity_event_lookup(&s.world->registry(),s.root->entity.id));};
    h.regeneration_gate=[](void*p,int32_t flag){static_cast<Behaviour*>(p)->actor->root->root_update.regeneration_flag=flag>0?flag:-1;return 0;};
    h.health=[](void*p)->double{return static_cast<Behaviour*>(p)->actor->entity().actor.health9C;};
    h.maximum=[](void*p)->double{return static_cast<Behaviour*>(p)->actor->entity().actor.maximumA0;};
    h.set_health=[](void*p,float value){auto&a=static_cast<Behaviour*>(p)->actor->entity().actor;a.health9C=value;if(value>a.maximumA0)a.maximumA0=value;return 0;};
    h.monitor_ready=[](void*p){auto&s=*static_cast<Behaviour*>(p)->actor;require(s.services.heartbeat_ready,"NPC heartbeat manager readiness");return s.services.heartbeat_ready(s.services.owner);};
    h.resource=[](void*p,const char**text){*text=static_cast<Behaviour*>(p)->actor->owner->heartbeat_resource();return int(*text!=nullptr);};
    h.show=[](void*p,const char*text){auto&s=*static_cast<Behaviour*>(p)->actor;require(s.services.heartbeat_show,"NPC heartbeat display owner");return s.services.heartbeat_show(s.services.owner,s.entity().id,text);};
    h.transition=[](void*p,int32_t next){if(next==-1)return 0;static_cast<Behaviour*>(p)->actor->transition(next);return 1;};
    h.exit=[](void*p){static_cast<Behaviour*>(p)->exit();return 0;};
    h.base=[](void*p,uint8_t){auto&b=*static_cast<Behaviour*>(p);std::copy_n(b.actor->entity().transform,16,b.entity.transform);return 0;};return h;
}
UCMedicUpdateServices Root::medic_update_services(bool){UCMedicUpdateServices h{};h.owner=this;
    h.unrestrain=[](void*)->int{throw std::runtime_error("Medic unrestrain2F3D0 is unconnected");};
    h.children_ready=[](void*p,uint8_t*out){auto&r=*static_cast<Root*>(p);for(auto*c:r.medic_children){if(!c->active)break;require(!c->foreground(true),"Medic condition update39190");}
        *out=0;auto i=std::find_if(r.medic_children.begin(),r.medic_children.end(),[](const Behaviour*b){return !b->active;});if(i!=r.medic_children.end()){r.medic_children.erase(i);*out=1;}return 0;};
    h.foreground=[](void*p,int32_t*out){auto&s=*static_cast<Root*>(p)->actor;*out=s.foreground?s.foreground->entity.id:-1;return 0;};
    h.complete=[](void*p,int32_t id,int32_t state,int32_t sender){auto&r=*static_cast<Root*>(p);uint32_t from=(uint32_t)sender;uint8_t handled=0;return uc_entity_event_send(&r.world->registry(),id,126,&state,sizeof state,&from,&handled);};
    h.reactions=[](void*p,uint8_t hurt,uint8_t regenerate){return static_cast<Root*>(p)->update_health(hurt,regenerate);};
    h.integer=[](void*p,const char*name,int32_t,int32_t*out){auto&r=*static_cast<Root*>(p);require(!std::strcmp(name,"heals left"),"Medic update integer query");*out=r.heals();return 0;};
    h.clear_debug=[](void*p){static_cast<Root*>(p)->actor->debug_states.clear();return 0;};
    h.debug=[](void*p,const char*text){static_cast<Root*>(p)->actor->debug_states.push_back({text,0,true});return 0;};
    h.body_ready=[](void*,uint8_t*)->int{throw std::runtime_error("Medic medkit putdown body64 is unconnected");};
    h.drop_allowed=[](void*,int32_t,uint8_t*)->int{throw std::runtime_error("Medic foreground medkit drop property is unconnected");};
    h.box_event=[](void*p,uint8_t down,int32_t sender){auto&r=*static_cast<Root*>(p);require(r.melee!=nullptr,"Medic box controller");uint8_t handled=0;uint32_t from=(uint32_t)sender;return uc_entity_event_send(&r.world->registry(),r.melee->weapon,216,&down,1,&from,&handled);};
    h.secondary=[](void*p,int32_t id,float expensive){auto&r=*static_cast<Root*>(p);if(!uc_entity_event_lookup(&r.world->registry(),id))return 0;auto*b=r.medic_speech&&r.medic_speech->entity.id==id?r.medic_speech:r.secondary;require(b&&b->entity.id==id,"Medic secondary identity");if(!b->active)return 0;return b->foreground(expensive!=0);};
    h.base=[](void*p,float){auto&r=*static_cast<Root*>(p);std::copy_n(r.actor->entity().transform,16,r.entity.transform);return 0;};
    h.foreground_busy=[](void*p,uint8_t*out){auto&s=*static_cast<Root*>(p)->actor;*out=s.foreground&&s.foreground->replacement;return 0;}; //32560 current foreground E0.
    h.should_run=[](void*p,int32_t id,uint8_t*out){auto&r=*static_cast<Root*>(p);auto&s=*r.actor;auto*b=r.medic_speech;
        require(b&&b->entity.id==id&&b->kind==Behaviour::MedicSpeech,"Plain medic authored speech property");*out=0;
        //E2FC0 initializes1EC=0; admitted MoveRun/Scripted/Loop/Follow do not
        //set it. Retain the original gate before169050 range/timer work.
        if(!s.generic_speech||!b->active||s.actor_status&0x20007)return 0;
        auto h=b->heal_services();int is_near=h.nearby(h.owner,300.f,b->narrow_speech?15.f:360.f);if(is_near<0)return 1;
        b->speech_elapsed=is_near?float(double(b->speech_elapsed)+*r.world->script_services().delta):0.f;*out=b->speech_elapsed>1.f;return 0;};return h;
}
int Root::update_health(uint8_t hurt,uint8_t regenerate){
    root_update.order=order_id;root_update.damage_time=damage_elapsed;root_update.status=&actor->actor_status;
    UCCharacterRootUpdateServices h{};h.owner=this;
    h.health=[](void*p)->double{return static_cast<Root*>(p)->actor->entity().actor.health9C;};
    h.maximum=[](void*p)->double{return static_cast<Root*>(p)->actor->entity().actor.maximumA0;};
    h.common=[](void*p){auto&r=*static_cast<Root*>(p);r.common(false);r.root_update.damage_time=r.damage_elapsed;r.root_update.order=r.order_id;return 0;};
    h.emote=[](void*p,uint32_t code,uint32_t priority){static_cast<Root*>(p)->actor->dialogue->say((int32_t)code,(int32_t)priority);return 0;};
    h.flag=[](void*p,int32_t flag){const auto&h=static_cast<Root*>(p)->world->script_services();if(flag<0||uint32_t(flag)>=h.flag_count)return -1;return int(h.flags[flag]!=0);};
    h.delta=[](void*p)->double{return *static_cast<Root*>(p)->world->script_services().delta;};
    h.set_health=[](void*p,float value){auto&a=static_cast<Root*>(p)->actor->entity().actor;a.health9C=value;if(value>a.maximumA0)a.maximumA0=value;return 0;};
    return uc_character_root_update(&root_update,hurt,regenerate,&h);
}
void Root::update(bool expensive){if(!medic){common();return;}medic_state.root=entity.id;medic_state.order=order_id;medic_state.reactions=reactions;medic_state.status=&actor->actor_status;
    auto h=medic_update_services(expensive);int result=plain_medic?uc_medic_plain_update(&medic_state,&medic_policy,medic_speech?medic_speech->entity.id:-1,float(expensive),&h):uc_medic_unassigned_update(&medic_state,&medic_policy,float(expensive),&h);require(!result,"Medic root source update");}
void Root::exit(){
    if(medic){auto h=medic_services();int result=plain_medic?uc_medic_plain_exit(&medic_policy,medic_speech?medic_speech->entity.id:-1,&h):uc_medic_root_exit(&medic_policy,&h);require(!result,"Medic root source exit");return;}
    require(active&&actor,"NPC root exit lifetime");
    if(brute){require(!actor->services.tags(actor->services.owner,actor->entity().id,"MELEE_ENEMY",false),"Brute enemy tag removal1D0D0F");
        if(melee){auto h=melee_services();require(!uc_npc_melee_close(melee.get(),&h),"Brute melee release1D0D20");melee.reset();}actor->actor_support=0;}
    if(secondary&&secondary->active)secondary->exit();secondary=nullptr;gun.reset();
    if(thrown){thrown->close();for(auto*flag=thrown->flags;flag!=thrown->flags+7;flag++)actor->triggers.erase(std::remove_if(actor->triggers.begin(),actor->triggers.end(),[flag](const CharacterState::Trigger&t){return t.flag==flag;}),actor->triggers.end());}thrown.reset();
    request_remove(*world,entity);actor->root=nullptr;actor=nullptr;active=false;
}
void Root::common(bool publish){
    require(active&&actor,"NPC root controller identity");auto&s=*actor;auto&e=s.entity();float delta=*world->script_services().delta;
    if(s.actor_status&1)return;
    uint32_t flags=reactions;float amount=0;require(!uc_character_injury_read(&injury,&flags,&amount),"NPC queued injury36800");amount+=damage;
    damage_elapsed=damage_elapsed<0?0:float(double(damage_elapsed)+delta);elapsed=float(double(elapsed)+delta);
    if(flags&0x1fffe)damage_elapsed=0;
    const bool protected_hit=(flags&0x40000)?(protection&1)!=0:(protection&2)!=0;
    if((flags||amount)&&!protected_hit){
        const bool was_alive=e.actor.health9C>0;
        health_state.order=order_id;health_state.enemy=uint8_t(enemy);health_state.elapsed=elapsed;
        UCCharacterHealthServices h{};h.id=e.id;h.health=&e.actor.health9C;h.actor_status=&s.actor_status;h.death_kind=&s.death_kind;
        require(s.services.health_policy&&!s.services.health_policy(s.services.owner,*s.owner,health_state,flags,amount,h),"NPC health policy371B0");elapsed=health_state.elapsed;
        //38B8A..38C11 arms the force packet only on the living->dead health
        //transition. Scripted set-health deaths do not invent a hit impulse.
        if(was_alive&&e.actor.health9C<=0)s.death_forces=true;s.death_gore_disabled=h.gore_disabled!=0;
        if(e.actor.health9C<=0){s.die();}
        else if((flags&0x1F0FE)==0x10000&&enemy&&!(protection&4)&&!(s.actor_status&4)){
            require(!s.restrain(0,0),"NPC root restraint37E38");injury.owner=s.services.player_id(s.services.owner);
        }
        else if((flags&0x1fffe)&&amount>0&&s.reaction_suppression==0&&!(s.actor_status&4)){
            //PC reaction adapter: the original hit queue37AF0 ultimately needs
            //the unfinished body override. Use its actual authored quick-hit
            //overlay where present; class1 has no such clip and gets a short
            //visible recoil. Neither branch changes damage or AI state.
            if(!brute)s.movement->graph.queue_action("QuickHitOverlay1",nullptr,true);
            else s.hit_time=.25f;
        }
    }
    reactions=0;damage=0;injury.reactions=0;injury.damage=0;
    if(s.actor_status&1)return;
    require((s.actor_status==0||s.actor_status==4||(s.burning&&s.actor_status==0x10000))&&e.transform[13]>=-100000&&timed==0,"NPC non-bullet root reaction state is unconnected");
    require(s.services.hazard,"NPC environmental volume owner");uint8_t hit=0;float head[3];s.head_position(head);
    require(!s.services.hazard(s.services.owner,head,0,&hit),"NPC environmental volume query");
    if(!hit)require(!s.services.hazard(s.services.owner,head,1,&hit),"NPC environmental volume query");
    require(!hit,"NPC environmental dialogue request is unconnected");
    if(cop&&secondary&&secondary->active&&secondary->kind==Behaviour::Speech){
        //1BD380 ->33B60 first rejects the real player/body distance. Close
        //speech additionally needs the actual Player 'Look point' property.
        int32_t id=s.services.player_id(s.services.owner);auto*t=uc_entity_event_lookup(&world->registry(),id);
        require(t,"NPC generic speech player registry");const auto&c=world->context();auto*v=c.resolve(c.owner,t);require(v,"NPC generic speech player position");
        float x=v->transform[12]-s.body->world[12],y=v->transform[13]-s.body->world[13],z=v->transform[14]-s.body->world[14];
        float distance=(z*z+y*y)+x*x;
        require(!(90000.f>distance),"CopGenericSpeech nearby Player Look point and dialogue are unconnected");secondary->speech_elapsed=0;
    }
    if(publish){
        if(blast_notice.remaining>0){
            if(!damage_enabled)blast_notice.remaining=0;
            else {const float*source=s.live_world(blast_notice.sender);
                require(!uc_character_blast_notice_step(&blast_notice,e.transform+12,source?source+12:nullptr,delta),"NPC explosion notice36F80");}
        }
        std::memcpy(entity.transform,e.transform,64); //37160
    }
}
void CharacterState::die(){
    auto&e=entity();require(root&&body&&movement&&model,"NPC death owner lifetime");
    if(actor_status&1)return;
    if(!death_kind){static const int32_t kinds[]={6,1,1,5,1,5,7,7,1,1,1,2};
        const uint32_t index=uint32_t(root->injury.kind)-2u;death_kind=index<12?kinds[index]:1;} //385C0 table386BC.
    ragdoll_begin();actor_status|=1;
    targets.current=targets.preferred=-1;navigator->stop();
    if(foreground)foreground->exit();movement->stop();movement->graph.stop_aim();movement->graph.stop_look();
    if(root->thrown)root->thrown->drop(); //DeadBehaviour1B5460 drops both held slots through virtual8.
    controller_tasks.clear();
    dialogue->cancel();dialogue_face=dialogue_look=false;pending_events={};
    std::memcpy(corpse_world,e.transform,64);corpse_ground=e.transform[13];corpse_time=0;model->reset_presentation();
    require(services.character_died&&!services.character_died(services.owner,*owner,root->injury.owner,death_kind==9||death_kind==10),"NPC death event385C0");
}
void CharacterState::ragdoll_begin(){
    if(ragdoll){
        //A restrained actor already owns the articulated pose. Death must not
        //recreate bodies or erase their velocity before applying the new hit.
        if(death_forces&&death_kind!=9)require(uc_ragdoll_forces(hit_response.impulse,death_kind==10&&!death_gore_disabled?.4f:1.f,corpse_forces),"NPC retained ragdoll death forces");return;
    }
    if(!services.ragdoll_world&&!services.ragdoll_asset)return;
    require(services.ragdoll_world&&services.ragdoll_asset&&model->prepared.ready,"NPC ragdoll services");
    const auto&a=*services.ragdoll_asset;const auto&source=model->source;const uint32_t n=source.bone_count;
    require(n&&n==bank.skeleton.bone_count&&model->binding.local&&a.body_count,"NPC ragdoll live skeleton");
    corpse_local.reset(new float[n][16]);corpse_pose.reset(new float[n][16]);corpse_body.assign(n,-1);corpse_order.clear();
    std::memcpy(corpse_local.get(),model->binding.local,n*64);
    std::vector<uint8_t> done(n);while(corpse_order.size()<n){bool progress=false;
        for(uint32_t i=0;i<n;i++)if(!done[i]){const uint32_t parent=source.bones[i].parent;
            require(parent<n&&(i==0||parent!=i),"NPC ragdoll skeleton parent");
            if(i&&!done[parent])continue;done[i]=1;corpse_order.push_back(i);progress=true;}
        require(progress,"NPC ragdoll acyclic skeleton");}
    auto initial=std::make_unique<float[][16]>(a.body_count);
    for(uint32_t i=0;i<a.body_count;i++){
        std::string name=a.bodies[i].name;std::replace(name.begin(),name.end(),'_',' ');int32_t bone=-1;
        for(uint32_t j=0;j<n;j++)if(!std::strcmp(name.c_str(),bank.skeleton.bone_names[j])){require(bone==-1,"NPC ragdoll unique bone name");bone=int32_t(j);}
        require(bone>=0&&corpse_body[bone]<0&&model->prepared.world_valid[bone],"NPC ragdoll authored bone mapping");corpse_body[bone]=int32_t(i);
        UCAnimationKey key;require(uc_ragdoll_pose_key(model->prepared.world[bone],entity().transform,&key)&&!uc_animation_blend_matrix(&key,initial[i]),"NPC ragdoll animation handoff2A51C0");
    }
    auto created=services.ragdoll_world->create(a,initial.get());require(created&&created->body_count()==a.body_count,"NPC ragdoll articulated body owner");
    const float zero[3]={};for(uint32_t i=0;i<a.body_count;i++)created->velocity(i,zero,zero); //2A5583 motion58.
    corpse_forces[0]={};corpse_forces[1]={};
    if(death_forces&&death_kind!=9)require(uc_ragdoll_forces(hit_response.impulse,death_kind==10&&!death_gore_disabled?.4f:1.f,corpse_forces),"NPC authored death forces1B5460");
    ragdoll=std::move(created);
}
void CharacterState::ragdoll_step(float delta){
    float inverse_actor[16];require(uc_point_light_inverse(entity().transform,inverse_actor),"NPC ragdoll actor inverse2A55A0");
    for(auto&force:corpse_forces)require(uc_ragdoll_force_step(&force,delta),"NPC ragdoll timed force2A33A0");
    for(uint32_t bone:corpse_order){const uint32_t parent=model->source.bones[bone].parent;
        if(corpse_body[bone]>=0){float physics[16],inverse_parent[16];ragdoll->transform(unsigned(corpse_body[bone]),physics);
            for(const auto&force:corpse_forces){float sampled[4],point[4]={physics[12],physics[13],physics[14],0};
                require(uc_ragdoll_force_sample(&force,point,sampled),"NPC ragdoll spatial force2A3480");
                if(sampled[0]!=0||sampled[1]!=0||sampled[2]!=0)ragdoll->add_force(unsigned(corpse_body[bone]),sampled);}
            require(uc_ragdoll_pose_world(physics,inverse_actor,corpse_pose[bone]),"NPC ragdoll physics pose2A55A0");
            if(bone){require(uc_point_light_inverse(corpse_pose[parent],inverse_parent),"NPC ragdoll parent inverse");uc_ragdoll_multiply(corpse_pose[bone],inverse_parent,corpse_local[bone]);}
            else std::memcpy(corpse_local[bone],corpse_pose[bone],64);
        }else if(bone)uc_ragdoll_multiply(corpse_local[bone],corpse_pose[parent],corpse_pose[bone]);
        else std::memcpy(corpse_pose[bone],corpse_local[bone],64);
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};model->prepare(*renderer,corpse_local.get(),identity);
}
void CharacterState::corpse_step(){
    auto&e=entity();const float delta=*world->script_services().delta;std::memcpy(previous,e.transform,64);
    const bool settled=corpse_time>=.55f;corpse_time+=delta;
    if(corpse_time>30){request_remove(*world,e);return;} //Original corpse age limitE7830.
    if(ragdoll){ragdoll_step(delta);return;}
    if(settled)return;
    //Legacy fixture path without a configured solver. Full native runtime
    //always supplies the authored ragdoll services above.
    using namespace DirectX;
    XMVECTOR direction=XMVectorSet(hit_response.direction[0],0,hit_response.direction[2],0);
    if(XMVectorGetX(XMVector3LengthSq(direction))<.0001f)direction=XMVectorSet(corpse_world[8],0,corpse_world[10],0);
    XMVECTOR axis=XMVector3Normalize(XMVector3Cross(XMVectorSet(0,1,0,0),direction));
    XMFLOAT4X4 original,result;std::memcpy(&original,corpse_world,64);original._41=original._42=original._43=0;
    const float phase=std::min(1.f,corpse_time/.55f),angle=XM_PIDIV2*(phase*phase*(3-2*phase));
    XMStoreFloat4x4(&result,XMLoadFloat4x4(&original)*XMMatrixRotationAxis(axis,angle));
    float minimum=FLT_MAX;for(uint32_t i=0;i<model->prepared.vertex_count;i++){const float*p=model->prepared.vertices[i].position;
        minimum=std::min(minimum,p[0]*result._12+p[1]*result._22+p[2]*result._32);}
    require(minimum!=FLT_MAX&&std::isfinite(minimum),"NPC corpse posed bounds");
    float from[3]={corpse_world[12],corpse_world[13]+100,corpse_world[14]},to[3]={from[0],from[1]-400,from[2]};
    UCCollisionResult ground{};int32_t id=-1;
    require(!services.refresh_collision(services.owner),"NPC corpse collision refresh");
    int hit=services.collision->query(from,to,9,e.actor.group94,0x80,0,ground,id);
    require(hit>=0,"NPC corpse floor query");if(hit)corpse_ground=ground.point[1];
    result._41=corpse_world[12];result._42=corpse_ground-minimum;result._43=corpse_world[14];std::memcpy(e.transform,&result,64);
}
void CharacterState::create_body(){
    auto&e=entity();require(!body,"NPC body already exists");
    const float yaw=(float)-std::atan2(double(e.transform[8]),double(e.transform[10]));
    float position[3];std::copy_n(e.transform+12,3,position);
    if(snap_to_ground){UCCharacterGroundServices h{};h.owner=this;
        h.ray=[](void*p,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse,float*point,float*normal){auto&s=*static_cast<CharacterState*>(p);
            require(!s.services.refresh_collision(s.services.owner),"NPC initial ground collision refresh");UCCollisionResult hit{};int32_t id=-1;
            int found=s.services.collision->query(from,to,type,group,mask,impulse,hit,id);if(found==1){std::copy_n(hit.point,3,point);std::copy_n(hit.normal,3,normal);}return found;};
        require(!uc_character_ground(position,e.actor.group94,&h,position+1,nullptr,nullptr),"NPC initial ground query2B50E0");}
    body=std::make_unique<NativeCharacterBody>(position,yaw);
    {const size_t dot=model_name.find_last_of('.');require(dot!=std::string::npos,"NPC collision resource suffix");
        const std::string name=model_name.substr(0,dot)+".mdlhk";BytesOwner bytes;UCModelCollisionAsset asset{};
        size_t index=find_resource(archive,name.c_str());require(index!=SIZE_MAX&&uc_archive_read(archive,index,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,"NPC authored collision resource");
        require(!uc_model_collision_load(bytes.value.data,bytes.value.size,0,&asset),"NPC collision resource decode");
        try{collision=std::make_unique<NativeCharacterCollision>(asset,model->source.bone_count,e.actor.group94,e.id,&e);}
        catch(...){uc_model_collision_free(&asset);throw;}uc_model_collision_free(&asset);
    }
    int32_t root_bone=model->binding.skeleton.root_bone;
    require(root_bone>=0,"NPC authored root bone");movement=std::make_unique<NativeCharacterMovement>(bank,character_class==11?"Paramedic_":root->cop?"Cop_":root->brute?"BurnerBrute_":"BurnerBlazer_",root_bone,*body);
    movement->graph.initialize_baseline(*body,*vectors,character_class);
}
void CharacterState::enter_skin(int32_t source){
    ActiveDescriptor restore(*world);int32_t id;require(source>=0&&!world->ensure(source,&id),"NPC appearance factory");
    auto*b=world->binding(source);require(b&&b->step==Behaviour::step,"NPC appearance registry");auto*skin=static_cast<Behaviour*>(b->owner);
    require((skin->kind==Behaviour::Appearance||skin->kind==Behaviour::Named)&&!skin->enter(*this),"NPC appearance entry");
}
void CharacterState::start(){
    auto&e=entity();
    //DF1A0 resolves an authored root before tag/spawn/faction effects. Dedicated
    //CharacterBurnerBlazer already owns its dynamic root from its constructor.
    if(root_source!=-1){ActiveDescriptor restore(*world);int32_t id=-1;
        int code=world->ensure((uint32_t)root_source,&id);
        if(code)throw std::runtime_error("NPC authored root factory source "+std::to_string(root_source)+" "+
            world->metadata((uint32_t)root_source).definition->class_name+" code "+std::to_string(code));
        auto*b=world->binding((uint32_t)root_source);
        require(b&&b->step==Root::step&&*b->view->id==id,"NPC authored root registry type");root=static_cast<Root*>(b->owner);root_source=-1;}
    require(root&&!root->active&&!body,"NPC activation state");
    require(services.tags&&!services.tags(services.owner,e.id,"AUTOAIM",true),"NPC automatic aiming tag registration");
    if(spawn!=-1){ActiveDescriptor restore(*world);int32_t id=-1;
        require(!world->ensure((uint32_t)spawn,&id),"NPC authored spawn factory");auto*b=world->binding((uint32_t)spawn);
        require(b&&b->step==SpawnPoint::step&&*b->view->id==id,"NPC authored spawn registry type");std::memcpy(e.transform,b->view->transform,64);}
    require(services.faction&&!services.faction(services.owner,e.id,faction,true),"NPC faction registration");faction_joined=true;
    if(root->cop){
        dialogue->configure(2,root->emotes.c_str());
    }
    root->actor=this;
    if(root->medic){auto h=root->medic_services();int32_t speech=-1;int result=root->plain_medic?uc_medic_plain_enter(&root->medic_policy,root->secondary_source,&speech,&controller,&h):uc_medic_root_enter(&root->medic_policy,&controller,&h);require(!result,"Medic root source entry");
        transition(starting);e.flags|=0x800;started=true;return;}
    root->active=true;create_body();enter_skin(root->skin);
    if(root->cop){
        if(root->has_gun){
            require(services.npc_gun,"Cop concrete weapon controller services");auto g=std::make_unique<UCNpcGunController>();
            require(!uc_npc_gun_controller_construct(g.get(),e.id,body.get(),0,root->skill,services.npc_gun),"Cop weapon controller292F0");root->gun=std::move(g);
            int32_t id=-1;require(services.cop_gun&&!services.cop_gun(services.owner,*world,*owner,&id),"Cop dynamic Gun owner");
            require(uc_entity_event_lookup(&world->registry(),id),"Cop gun registry lifetime");
            require(!uc_npc_gun_controller_bind(root->gun.get(),id,services.npc_gun),"Cop weapon controller293D0");
        }
        if(named_character==2){root->death_cues[0]="Heartbeat-Cop-1";root->death_cues[1]="Obituaries-HalForrester-1";
            root->death_cues[2]="Obituaries-HalForrester-2";root->death_cues[3]="Obituaries-HalForrester-3";root->named_cues=true;}
        else{require(named_character==0||named_character==7,"Cop named death cue profile");root->death_cues[0]="Heartbeat-Cop-3";}
        root->secondary=enter_child(root->secondary_source);e.actor.health9C=900;if(e.actor.health9C>e.actor.maximumA0)e.actor.maximumA0=e.actor.health9C;
    }else if(root->brute){
        //1D0E40: class1 body and appearance precede the real2A100 melee
        //controller/2A5E0 weapon. No Blazer projectile controller is involved.
        dialogue->configure(0,nullptr); //1D0EBB..1D0EBF:21A030 precedes the melee owner.
        root->melee=std::make_unique<UCNpcMeleeController>();
        uc_npc_melee_construct(root->melee.get(),e.id,body.get(),&triggers,&character_audio);
        int32_t id=-1;require(services.brute_weapon&&!services.brute_weapon(services.owner,*world,*owner,&id),"Brute real melee weapon2A5E0 factory");
        auto h=root->melee_services();require(!uc_npc_melee_bind(root->melee.get(),id,&h),"Brute melee weapon attachment1D0F46");
        actor_support=reinterpret_cast<uintptr_t>(root->melee.get());
        e.actor.health9C=250;if(e.actor.health9C>e.actor.maximumA0)e.actor.maximumA0=e.actor.health9C;
        root->damage_enabled=true;root->enemy=true;
        require(!services.tags(services.owner,e.id,"MELEE_ENEMY",true),"Brute enemy tag registration1D0F8B");
        require(services.ensure_brute_template&&!services.ensure_brute_template(services.owner,*world,root->entity.id),"Brute Cleaver template1D0FE5");
    }else{
    require(services.npc_throw,"NPC real thrown item services");root->thrown=std::make_unique<NativeNpcThrowHandler>(e.id,11,*services.npc_throw);
    const char*thrown_triggers[]={"ThrownWeaponRestock","ThrownWeaponIgnite","ThrownWeaponRestockSecondaryWeapon","ThrownWeaponIgniteSecondaryWeapon","ThrownWeaponThrow","ThrownWeaponExplode","ThrownWeaponLighterSound"};
    for(unsigned i=0;i<7;i++)bind_trigger(thrown_triggers[i],root->thrown->flags+i);
    e.actor.health9C=250;if(e.actor.health9C>e.actor.maximumA0)e.actor.maximumA0=e.actor.health9C; //DE540→2180A0 updates the retained maximum too.
    dialogue->configure(0,nullptr);root->enemy=true;
    root->damage_enabled=true;require(services.ensure_molotov_template&&!services.ensure_molotov_template(services.owner,*world,root->entity.id),"NPC Molotov template factory");
    }
    transition(starting);e.flags|=0x800;started=true;
}
void CharacterState::release(){
    if(foreground)foreground->exit();if(root&&root->active)root->exit();
    ragdoll.reset();corpse_local.reset();corpse_pose.reset();navigator.reset();movement.reset();body.reset();collision.reset();model.reset();entity().model=nullptr;
    if(faction_joined){require(!services.faction(services.owner,entity().id,faction,false),"NPC faction removal");faction_joined=false;}
    if(controller_registered){require(!services.controller_registration(services.owner,entity().id,false),"NPC controller removal");controller_registered=false;}
}
}

struct NativeMissionCharacter::Impl:CharacterState{};
NativeMissionCharacter::NativeMissionCharacter():p_(std::make_unique<Impl>()){p_->owner=this;}
NativeMissionCharacter::~NativeMissionCharacter()=default;
int native_construct_character_behaviour(NativeMissionWorld&w,uint32_t source,const UCArchive *archive){
    if(source>=w.source_count())return 1;const auto&m=w.metadata(source);const auto&d=*m.definition;
    const bool interactive=!std::strcmp(d.class_name,"InteractiveScriptedBehaviour"),scripted=!std::strcmp(d.class_name,"ScriptedBehaviour");
    if(m.asset_present||(!interactive&&!scripted&&(m.condition_count||d.script_count||!empty_scripts(d.script_records))))return 1;
    if(interactive&&(m.condition_count||d.script_count!=1||!d.scripts||d.link_count!=1))return 1;
    if(scripted&&(m.condition_count||d.script_count!=1||!d.scripts||d.link_count))return 1;
    const bool plain_medic=!std::strcmp(d.class_name,"Medic"),cop_root=!std::strcmp(d.class_name,"Cop"),medic_root=plain_medic||!std::strcmp(d.class_name,"MedicForPlayersTeam"),brute_root=!std::strcmp(d.class_name,"BurnersBruteBehaviour");
    if(medic_root||cop_root||brute_root||!std::strcmp(d.class_name,"BurnersBlazerBehaviour")){
        if(cop_root){if(d.properties.size!=32||word(d.properties,0)>1||word(d.properties,1)>1||word(d.properties,6)||word(d.properties,7)||word(d.properties,5)>=d.link_count)return 1;}
        else if(plain_medic){if(d.properties.size!=20||word(d.properties,2)>=d.link_count||word(d.properties,3)>1||word(d.properties,4))return 1;}
        else if(medic_root){if(d.properties.size!=8||word(d.properties,1)>=d.link_count)return 1;}
        else if(d.properties.size!=4)return 1;
        auto*r=new Root;r->world=&w;r->cop=cop_root;r->medic=medic_root;r->plain_medic=plain_medic;r->brute=brute_root;r->skin=(int32_t)word(d.properties,cop_root?2:0);
        if(cop_root){r->has_gun=word(d.properties,0)!=0;r->compliance=word(d.properties,1)!=0;r->secondary_source=(int32_t)word(d.properties,3);
            r->skill=real(d.properties,4);r->emotes=d.links[word(d.properties,5)];if(!std::isfinite(r->skill)){delete r;return 1;}}
        if(medic_root){r->emotes=d.links[word(d.properties,plain_medic?2:1)];if(plain_medic){r->secondary_source=(int32_t)word(d.properties,1);r->move_to_heal=word(d.properties,3)!=0;}r->medic_policy.initial=r->skin;r->medic_policy.bank=r->emotes.c_str();r->medic_policy.secondary=-1;
            r->medic_state.order=-1;r->medic_state.can_heal=1;}
        if(w.bind(source,{&r->entity.view,r,Root::step,Root::late,Root::close,nullptr,Root::remove,Root::close})){delete r;return 1;}
        if(uc_entity_base_construct_authored(&w.context(),&r->entity.view,w.active_descriptor())||
           w.retirement().remove_update_record(w.retirement().owner,r->entity.id))return 2;
        if(medic_root){
            require(!uc_entity_subscriptions_create(r,&r->subscriptions),"Medic root listener owner");int inserted=0;
            require(!uc_entity_listener_register(&w.registry(),r->entity.id,r->entity.id,uc_entity_subscriptions_listener(r->subscriptions),w.listeners(),&inserted)&&inserted,"Medic root listener registration");
            const UCEntityEventSubscription events[]={
                {147,[](void*p,const void*,uint32_t*){auto&r=*static_cast<Root*>(p);require(r.actor&&r.actor->services.medic_set_heals,"Medic heal-count setter owner");auto&s=*r.actor;
                    require(!s.services.medic_set_heals(s.services.owner,s.named_character,r.heals()-1),"Medic consumed heal169570");return 0;}},
                {74,[](void*p,const void*payload,uint32_t*){require(payload,"Medic team join payload");return static_cast<Root*>(p)->join_team(*static_cast<const UCMedicTeamJoin*>(payload));}},
                {115,[](void*p,const void*payload,uint32_t*){require(payload,"Medic allow-heal payload");static_cast<Root*>(p)->medic_state.can_heal=*static_cast<const uint8_t*>(payload);return 0;}}};
            for(const auto&e:events)if(!plain_medic||e.event!=74)require(!uc_entity_subscriptions_append(r->subscriptions,&e),"Medic root event binding");
        }
        *w.property_cursor(source)=d.properties.data+d.properties.size;return 0;
    }
    if(!std::strcmp(d.class_name,"CharacterSpawnPoint")){
        if(d.properties.size)return 1;auto*s=new SpawnPoint;s->world=&w;
        if(w.bind(source,{&s->entity.view,s,SpawnPoint::step,SpawnPoint::step,SpawnPoint::close,nullptr,SpawnPoint::remove,SpawnPoint::close})){delete s;return 1;}
        if(uc_entity_base_construct_authored(&w.context(),&s->entity.view,w.active_descriptor()))return 2;
        s->entity.flags|=0x100; //1C73B0:20A140(100), then207F70 dormancy.
        if(w.retirement().remove_update_record(w.retirement().owner,s->entity.id))return 2;
        *w.property_cursor(source)=d.properties.data+d.properties.size;return 0;
    }
    Behaviour::Kind kind;if(interactive)kind=Behaviour::Interaction;
    else if(scripted)kind=Behaviour::Scripted;
    else if(!std::strcmp(d.class_name,"GenericExitCondition"))kind=Behaviour::GenericExit;
    else if(!std::strcmp(d.class_name,"FaceEntityBehaviour"))kind=Behaviour::Face;
    else if(!std::strcmp(d.class_name,"PlayAnimBehaviour"))kind=Behaviour::PlayAnim;
    else if(!std::strcmp(d.class_name,"MoveRunBehaviour"))kind=Behaviour::Move;
    else if(!std::strcmp(d.class_name,"MoveWalkBehaviour"))kind=Behaviour::Walk;
    else if(!std::strcmp(d.class_name,"JoinPlayersTeam"))kind=Behaviour::JoinTeam;
    else if(!std::strcmp(d.class_name,"RandomSkinAndEquipment"))kind=Behaviour::Appearance;
    else if(!std::strcmp(d.class_name,"DeleteMyself"))kind=Behaviour::Delete;
    else if(!std::strcmp(d.class_name,"Named_Character"))kind=Behaviour::Named;
    else if(!std::strcmp(d.class_name,"CopNoSecondary"))kind=Behaviour::NoSecondary;
    else if(!std::strcmp(d.class_name,"NoCondition"))kind=Behaviour::Condition;
    else if(!std::strcmp(d.class_name,"CopGenericSpeech"))kind=Behaviour::Speech;
    else if(!std::strcmp(d.class_name,"MedicGenericSpeech"))kind=Behaviour::MedicSpeech;
    else if(!std::strcmp(d.class_name,"PlayLoopedAnimBehaviour"))kind=Behaviour::Loop;
    else if(!std::strcmp(d.class_name,"NoBehaviour"))kind=Behaviour::None;
    else if(!std::strcmp(d.class_name,"ShowHeartbeatMonitor"))kind=Behaviour::Heartbeat;
    else if(!std::strcmp(d.class_name,"MedicDisableAutoHeal")||!std::strcmp(d.class_name,"MedicEnableAutoHeal"))kind=Behaviour::DisableHeal;
    else if(!std::strcmp(d.class_name,"MakeInvulnerable"))kind=Behaviour::Invulnerable;
    else if(!std::strcmp(d.class_name,"GotLineOfSight"))kind=Behaviour::LineOfSight;
    else if(!std::strcmp(d.class_name,"FollowEntity"))kind=Behaviour::Follow;
    else if(!std::strcmp(d.class_name,"WhenPlayerInRangeRunBehaviour"))kind=Behaviour::PlayerRange;
    else if(!std::strcmp(d.class_name,"WhenFlagSetRunBehaviour"))kind=Behaviour::FlagCondition;
    else if(!std::strcmp(d.class_name,"CopAttackBehaviour"))kind=Behaviour::Attack;
    else if(!std::strcmp(d.class_name,"BurnersBlazerAttackBehaviour"))kind=Behaviour::BlazerAttack;
    else if(!std::strcmp(d.class_name,"BurnersBruteAttackBehaviour"))kind=Behaviour::BruteAttack;
    else if(!std::strcmp(d.class_name,"CopMoveAndShoot"))kind=Behaviour::CopMove;else return 1;
    if(kind==Behaviour::Move||kind==Behaviour::Walk||kind==Behaviour::CopMove){if(d.properties.size<8||word(d.properties,0)>64||d.properties.size!=(word(d.properties,0)+(kind==Behaviour::CopMove?3:2))*4)return 1;}
    else if(kind==Behaviour::Scripted){if(d.properties.size!=12||word(d.properties,0)!=1||word(d.properties,1)!=0)return 1;}
    else if(kind==Behaviour::Face){if(d.properties.size!=16||word(d.properties,1)>1||!std::isfinite(real(d.properties,2)))return 1;}
    else if(kind==Behaviour::PlayAnim){if(d.properties.size!=20||word(d.properties,0)>=d.link_count||word(d.properties,1)>1||word(d.properties,2)>1||word(d.properties,4)>=d.link_count)return 1;}
    else if(kind==Behaviour::GenericExit){
        //18C3E0/18BDB0: OR of player bullet-by and authored flag. Keep the
        //unimplemented range/LOS/health/gas variants outside this admission.
        if(d.properties.size!=88||word(d.properties,1)||word(d.properties,2)!=1||word(d.properties,3)!=1||word(d.properties,4)||
            word(d.properties,6)!=1||word(d.properties,7)!=1||word(d.properties,11)!=UINT32_MAX)return 1;
        for(unsigned i:{9u,10u,12u,13u,14u,15u,16u,17u,18u,19u,20u,21u})if(word(d.properties,i))return 1;
    }
    else if(kind==Behaviour::Interaction){
        if(d.properties.size!=92)return 1;
        for(unsigned i:{0u,2u,6u,8u,10u,11u,12u,14u,18u,19u,21u})if(word(d.properties,i))return 1;
        for(unsigned i:{1u,3u,7u,15u,20u,22u})if(word(d.properties,i)!=UINT32_MAX)return 1;
        if(word(d.properties,4)!=1||word(d.properties,5)||word(d.properties,13)!=1||word(d.properties,17)!=1||!std::isfinite(real(d.properties,9)))return 1;
    }
    else if(kind==Behaviour::Attack){if(d.properties.size!=12||(word(d.properties,1)!=UINT32_MAX&&word(d.properties,1)>=w.source_count())||word(d.properties,2)>1)return 1;}
    else if(kind==Behaviour::BruteAttack){if(d.properties.size!=8||word(d.properties,0)>1)return 1;}
    else if(kind==Behaviour::BlazerAttack){if(d.properties.size!=32||word(d.properties,0)>1||word(d.properties,2)!=UINT32_MAX||
        !std::isfinite(real(d.properties,3))||word(d.properties,4)||word(d.properties,5)||word(d.properties,6)!=UINT32_MAX||word(d.properties,7))return 1;}
    else if(kind==Behaviour::Named){if(d.properties.size!=4||(word(d.properties,0)!=2&&word(d.properties,0)!=4&&word(d.properties,0)!=5&&word(d.properties,0)!=7))return 1;}
    else if((kind==Behaviour::Speech||kind==Behaviour::MedicSpeech)){if(d.properties.size!=4||word(d.properties,0)>1)return 1;}
    else if(kind==Behaviour::FlagCondition){if(d.properties.size<8||word(d.properties,0)>64||d.properties.size!=(word(d.properties,0)+2)*4)return 1;}
    else if(kind==Behaviour::PlayerRange){if(d.properties.size!=8||!std::isfinite(real(d.properties,0)))return 1;}
    else if(kind==Behaviour::LineOfSight){if(d.properties.size!=8)return 1;}
    else if(kind==Behaviour::Follow){if(d.properties.size!=44||word(d.properties,0)>=w.source_count()||word(d.properties,1)||
        !std::isfinite(real(d.properties,2))||word(d.properties,3)>1||word(d.properties,4)>1||
        word(d.properties,7)||word(d.properties,8)||word(d.properties,9)>1||word(d.properties,10))return 1;}
    else if(kind==Behaviour::Loop){if(!archive||d.properties.size<32||word(d.properties,3)>64||d.properties.size!=(word(d.properties,3)+8)*4||
        word(d.properties,0)>=d.link_count||word(d.properties,5+word(d.properties,3))>=d.link_count||
        word(d.properties,6+word(d.properties,3))>1||word(d.properties,7+word(d.properties,3))>1||!std::isfinite(real(d.properties,1)))return 1;}
    else if(kind==Behaviour::Heartbeat){if(d.properties.size<16||word(d.properties,1)>=d.link_count||word(d.properties,2)>64||d.properties.size!=(word(d.properties,2)+4)*4||!std::isfinite(real(d.properties,3+word(d.properties,2))))return 1;}
    else if(kind==Behaviour::JoinTeam){if(d.properties.size<32||word(d.properties,0)>64||d.properties.size!=(word(d.properties,0)+8)*4)return 1;
        for(unsigned i=0;i<3;i++){uint32_t link=word(d.properties,word(d.properties,0)+5+i);if(link!=UINT32_MAX&&link>=d.link_count)return 1;}}
    else if(kind==Behaviour::DisableHeal){if(d.properties.size!=4)return 1;}
    else if(kind==Behaviour::Invulnerable){if(d.properties.size!=20||word(d.properties,0)>1||word(d.properties,1)>1||word(d.properties,2)>1||word(d.properties,3)>1)return 1;}
    else if(d.properties.size)return 1;
    auto*b=new Behaviour(kind);b->world=&w;
    if(w.bind(source,{&b->entity.view,b,Behaviour::step,Behaviour::late,Behaviour::close,&b->entity.script,Behaviour::remove,Behaviour::close})){delete b;return 1;}
    auto failed=[source,&d](const char*stage,int code){throw std::runtime_error("NPC behavior construction source "+std::to_string(source)+" "+d.class_name+" "+stage+" code "+std::to_string(code));};
    if(int r=uc_entity_fresh_construct_authored(&w.context(),&b->entity.view,w.active_descriptor()))failed("fresh owner",r);
    if(kind==Behaviour::Move||kind==Behaviour::Walk||kind==Behaviour::CopMove){for(uint32_t i=0;i<word(d.properties,0);i++)b->nodes.push_back((int32_t)word(d.properties,1+i));
        b->move.world=b->entity.transform;b->move.nodes=b->nodes.data();b->move.count=(int32_t)b->nodes.size();b->move.next=(int32_t)word(d.properties,(kind==Behaviour::CopMove?2:1)+b->nodes.size());
        if(kind==Behaviour::CopMove)b->condition=(int32_t)word(d.properties,1+b->nodes.size());}
    if(kind==Behaviour::Interaction){b->interaction.script_type=word(d.properties,4);b->interaction.script_index=(int32_t)word(d.properties,5);
        b->interaction.range=real(d.properties,9);b->interaction.facing=15;b->interaction.next=(int32_t)word(d.properties,16);b->interaction.text=d.links[word(d.properties,19)];}
    if(kind==Behaviour::Scripted){b->script_type=(int32_t)word(d.properties,0);b->script_index=(int32_t)word(d.properties,1);b->range_next=(int32_t)word(d.properties,2);}
    if(kind==Behaviour::Face){b->follow_source=(int32_t)word(d.properties,0);b->narrow_speech=word(d.properties,1)!=0;b->player_range=real(d.properties,2);b->range_next=(int32_t)word(d.properties,3);}
    if(kind==Behaviour::GenericExit){b->range_next=(int32_t)word(d.properties,0);b->nodes.push_back((int32_t)word(d.properties,8));require(!uc_entity_subscriptions_create(b,&b->subscriptions),"NPC generic exit listener storage");}
    if(kind==Behaviour::Attack){b->condition=(int32_t)word(d.properties,0);b->cover_source=(int32_t)word(d.properties,1);b->can_relax=word(d.properties,2)!=0;b->attack.cover=-1;b->attack.world=b->entity.transform;}
    if(kind==Behaviour::BruteAttack){b->brute.can_throw=(uint8_t)word(d.properties,0);b->condition=(int32_t)word(d.properties,1);}
    if(kind==Behaviour::BlazerAttack){b->blazer_throw_low=(uint8_t)word(d.properties,0);b->condition=(int32_t)word(d.properties,1);b->blazer_idle.route_entity=-1;}
    if(kind==Behaviour::Heartbeat){b->heartbeat.next=(int32_t)word(d.properties,0);b->heartbeat.text=d.links[word(d.properties,1)];
        for(uint32_t i=0;i<word(d.properties,2);i++)b->nodes.push_back((int32_t)word(d.properties,3+i));
        b->heartbeat.flags=b->nodes.data();b->heartbeat.count=(int32_t)b->nodes.size();b->heartbeat.percent=real(d.properties,3+b->nodes.size());}
    if(kind==Behaviour::JoinTeam){unsigned count=word(d.properties,0);for(unsigned i=0;i<count;i++)b->nodes.push_back((int32_t)word(d.properties,1+i));
        b->team_join.count=(int32_t)count;b->team_join.flags=b->nodes.data();b->team_join.quit=(int32_t)word(d.properties,count+1);
        for(unsigned i=0;i<3;i++){b->team_join.conditions[i]=(int32_t)word(d.properties,count+2+i);uint32_t link=word(d.properties,count+5+i);if(link!=UINT32_MAX)b->team_text[i]=d.links[link];}
        if(!b->team_text[0].empty()){b->team_join.heartbeat=b->team_text[0].c_str();b->team_join.joined=b->team_text[1].c_str();b->team_join.left=b->team_text[2].c_str();}}
    if(kind==Behaviour::DisableHeal){b->range_next=(int32_t)word(d.properties,0);b->can_heal=!std::strcmp(d.class_name,"MedicEnableAutoHeal");}
    if(kind==Behaviour::Invulnerable){for(unsigned i=0;i<4;i++)b->invulnerable[i]=(uint8_t)word(d.properties,i);b->range_next=(int32_t)word(d.properties,4);}
    if(kind==Behaviour::Named)b->named=(int32_t)word(d.properties,0);
    if(kind==Behaviour::FlagCondition){for(uint32_t i=0;i<word(d.properties,0);i++)b->nodes.push_back((int32_t)word(d.properties,1+i));b->range_next=(int32_t)word(d.properties,1+b->nodes.size());}
    if(kind==Behaviour::PlayerRange){b->player_range=real(d.properties,0);b->range_next=(int32_t)word(d.properties,1);}
    if(kind==Behaviour::LineOfSight){b->follow_source=(int32_t)word(d.properties,0);b->range_next=(int32_t)word(d.properties,1);}
    if(kind==Behaviour::Follow){b->follow_source=(int32_t)word(d.properties,0);b->follow.stop_range=real(d.properties,2);
        b->follow.side_step=(uint8_t)word(d.properties,3);b->follow.run=(uint8_t)word(d.properties,4);b->follow.next=(int32_t)word(d.properties,5);
        b->condition=(int32_t)word(d.properties,6);b->friendly_bulletby=word(d.properties,9)!=0;b->follow.world=b->entity.transform;}
    if(kind==Behaviour::Speech||kind==Behaviour::MedicSpeech)b->narrow_speech=word(d.properties,0)!=0;
    if(kind==Behaviour::PlayAnim){
        auto read=[archive](const char*name,std::vector<uint8_t>&out){size_t i=find_resource(archive,name);BytesOwner bytes;
            require(i!=SIZE_MAX&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,"NPC one-shot authored resource");out.assign(bytes.value.data,bytes.value.data+bytes.value.size);};
        b->loop_clip=std::make_shared<NativeFirstpersonClip>();read(d.links[word(d.properties,0)],b->loop_clip->bytes);
        UCAnimationClip clip{};std::unique_ptr<UCAnimationClip,decltype(&uc_animation_clip_free)> cleanup(&clip,uc_animation_clip_free);
        auto check_clip=[&](UCAnimationStatus code,const char*stage){if(code!=UC_ANIMATION_OK)throw std::runtime_error("NPC one-shot source "+std::to_string(source)+" "+d.links[word(d.properties,0)]+" "+stage+" code "+std::to_string(code));};
        check_clip(uc_animation_clip_load(b->loop_clip->bytes.data(),b->loop_clip->bytes.size(),&clip),"clip load");check_clip(uc_animation_annotations_load(&clip,&b->loop_clip->annotations),"annotation load");
        b->hold_last=word(d.properties,1)!=0;b->ragdoll_when_tazered=word(d.properties,2)!=0;b->range_next=(int32_t)word(d.properties,3);read(d.links[word(d.properties,4)],b->loop_bank);
    }
    if(kind==Behaviour::Loop){
        auto read=[archive](const char*name,std::vector<uint8_t>&out){size_t i=find_resource(archive,name);BytesOwner bytes;
            require(i!=SIZE_MAX&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,"NPC loop authored resource");
            out.assign(bytes.value.data,bytes.value.data+bytes.value.size);};
        b->loop_clip=std::make_shared<NativeFirstpersonClip>();read(d.links[word(d.properties,0)],b->loop_clip->bytes);
        UCAnimationClip clip{};std::unique_ptr<UCAnimationClip,decltype(&uc_animation_clip_free)> cleanup(&clip,uc_animation_clip_free);
        auto check_clip=[&](UCAnimationStatus code,const char*stage){if(code!=UC_ANIMATION_OK)throw std::runtime_error("NPC loop source "+std::to_string(source)+" "+d.links[word(d.properties,0)]+" "+stage+" code "+std::to_string(code));};
        check_clip(uc_animation_clip_load(b->loop_clip->bytes.data(),b->loop_clip->bytes.size(),&clip),"clip load");check_clip(uc_animation_annotations_load(&clip,&b->loop_clip->annotations),"annotation load");
        b->loop.duration=real(d.properties,1);b->loop.next=(int32_t)word(d.properties,2);
        for(uint32_t i=0;i<word(d.properties,3);i++)b->nodes.push_back((int32_t)word(d.properties,4+i));
        b->loop.interrupt_count=(int32_t)b->nodes.size();b->loop.interrupts=b->nodes.data();b->loop.exit_condition=(int32_t)word(d.properties,4+b->nodes.size());
        read(d.links[word(d.properties,5+b->nodes.size())],b->loop_bank);
        b->can_heal=word(d.properties,6+b->nodes.size())!=0;
        b->loop.disable_collision=(uint8_t)word(d.properties,7+b->nodes.size());b->loop.world=b->entity.transform;
        b->loop.clip=b->loop_clip.get();b->loop.bank=b->loop_bank.data();
        require(!uc_entity_subscriptions_create(b,&b->subscriptions),"NPC loop event owner");int inserted=0;
        require(!uc_entity_listener_register(&w.registry(),b->entity.id,b->entity.id,uc_entity_subscriptions_listener(b->subscriptions),w.listeners(),&inserted)&&inserted,"NPC loop event registration");
        const UCEntityEventSubscription replacement{4,[](void*,const void*,uint32_t*)->int{
            throw std::runtime_error("NPC loop replacement-animation event13E6E0 is unconnected");}};
        require(!uc_entity_subscriptions_append(b->subscriptions,&replacement),"NPC loop replacement-animation subscription");
    }
    if(int r=b->entity.script.attach_actor(b->entity.view,m,w.script_services()))failed("script owner",r);
    if(int r=w.retirement().remove_update_record(w.retirement().owner,b->entity.id))failed("dormant record",r);
    *w.property_cursor(source)=d.properties.data+d.properties.size;return 0;
}
int NativeMissionCharacter::construct(NativeRenderer &renderer,NativeMissionWorld&w,uint32_t source,const UCArchive *archive,
    const uint8_t hash[32],const std::filesystem::path&catalog,const UCAnimationBlendMath&math,UCGameRandom&random,UCVectorRandom&vectors,const NativeMissionCharacterServices&services){
    auto failure=[source](const char*stage,int code)->int{throw std::runtime_error("NPC construction source "+std::to_string(source)+" "+stage+" code "+std::to_string(code));};
    if(p_->world||source>=w.source_count()||!archive||!hash||!services.collision||!services.collision->scenery||!services.navigation||!services.doors||!services.refresh_collision||!services.texture_lookup||services.texture_lookup->archive!=archive)return failure("services",1);
    const auto&m=w.metadata(source);const auto&d=*m.definition;
    const bool generic=!std::strcmp(d.class_name,"Character"),cop=!std::strcmp(d.class_name,"CharacterCop"),dedicated_medic=!std::strcmp(d.class_name,"CharacterMedic");
    if((!generic&&!cop&&!dedicated_medic&&std::strcmp(d.class_name,"CharacterBurnerBlazer"))||!m.asset_present||m.condition_count||d.script_count||!empty_scripts(d.script_records))return failure("authored metadata",1);
    bool class10=cop,medic=false,brute=false;
    if(generic||cop||dedicated_medic){
        //1CAFF0 shared Character descriptor; the authored root selects its
        //actual body, equipment and behavior profile below.
        if(d.properties.size!=(!generic?28u:40u)||(word(d.properties,0)!=0&&word(d.properties,0)!=2&&word(d.properties,0)!=160&&word(d.properties,0)!=200&&word(d.properties,0)!=201)||word(d.properties,4)||word(d.properties,5)>1||word(d.properties,6))return failure("authored properties",1);
        if(generic&&(word(d.properties,7)>1||word(d.properties,9)>1||word(d.properties,8)>=d.link_count))return failure("generic properties",1);
        uint32_t root=word(d.properties,2),spawn=word(d.properties,3);
        if(root>=w.source_count())return failure("root source",1);class10=!std::strcmp(w.metadata(root).definition->class_name,"Cop");medic=!std::strcmp(w.metadata(root).definition->class_name,"MedicForPlayersTeam")||!std::strcmp(w.metadata(root).definition->class_name,"Medic");
        brute=!std::strcmp(w.metadata(root).definition->class_name,"BurnersBruteBehaviour");
        if((cop&&!class10)||(dedicated_medic&&!medic)||(!class10&&!medic&&!brute&&std::strcmp(w.metadata(root).definition->class_name,"BurnersBlazerBehaviour"))||
           (spawn!=UINT32_MAX&&(spawn>=w.source_count()||std::strcmp(w.metadata(spawn).definition->class_name,"CharacterSpawnPoint"))))return failure("root or spawn type",1);
    }else if(d.properties.size!=32||word(d.properties,0)||word(d.properties,3)!=UINT32_MAX||word(d.properties,4)||word(d.properties,5)||word(d.properties,6)||word(d.properties,7))return failure("Blazer properties",1);
    auto&s=*p_;s.world=&w;s.renderer=&renderer;s.archive=archive;s.math=math;s.random=&random;s.vectors=&vectors;s.services=services;s.starting=(int32_t)word(d.properties,1);s.controller.actor=&s;
    uc_character_blast_initialize(&s.hit_response);
    s.character_class=medic?11:class10?10:brute?1:2;s.snap_to_ground=generic&&word(d.properties,9)!=0;
    s.player_pov_death=generic&&word(d.properties,7)!=0; //1CB15E; consumed only by the explicit death path.
    if(generic||cop||dedicated_medic){s.root_source=(int32_t)word(d.properties,2);s.spawn=(int32_t)word(d.properties,3);s.faction=word(d.properties,0);s.allow_deactivation=word(d.properties,5)!=0;}
    if(w.bind(source,{&entity.view,this,[](void*p){return static_cast<NativeMissionCharacter*>(p)->update();},[](void*p){return static_cast<NativeMissionCharacter*>(p)->late();},
        [](void*p){return static_cast<NativeMissionCharacter*>(p)->close_detached();},&entity.script,[](void*p){return static_cast<NativeMissionCharacter*>(p)->remove();},[](void*p){return static_cast<NativeMissionCharacter*>(p)->close_detached();}}))return failure("world binding",1);
    if(int r=uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure("fresh owner",r);
    s.contacts=std::make_unique<std::list<uint32_t>>();
    require(services.dialogue_audio&&services.dialogue_text&&services.subtitle,"NPC shared dialogue owners");
    NativeCharacterDialogueServices voice{};voice.owner=&s;
    voice.position=[](void*p,bool head,float out[3]){auto&s=*static_cast<CharacterState*>(p);if(head)s.head_position(out);else std::memcpy(out,s.entity().transform+12,12);return 0;};
    voice.subtitle=[](void*p,const NativeDialogueLine&line,int32_t id,int32_t*out){auto&s=*static_cast<CharacterState*>(p);return s.services.subtitle(s.services.owner,line,id,out);};
    voice.face=[](void*p,const NativeDialogueVariant&v){auto&s=*static_cast<CharacterState*>(p);require(s.body&&s.movement,"NPC dialogue face body");
        s.movement->graph.queue_face(v.phonemes.data(),v.ticks,*s.random);s.dialogue_face=true;return 0;};
    voice.emote_distance=[](void*p,double*out){auto&s=*static_cast<CharacterState*>(p);require(s.services.emote_distance,"NPC numeric dialogue reference matrix");return s.services.emote_distance(s.services.owner,*s.owner,out);};
    voice.emote_actor_flags=[](void*p,uint32_t*out){auto&s=*static_cast<CharacterState*>(p);if(!uc_entity_event_lookup(&s.world->registry(),s.entity().id))return 0;*out=s.actor_status;return 1;};
    voice.mission_ordinal=[](void*p,int32_t*out){auto&s=*static_cast<CharacterState*>(p);require(s.services.mission_ordinal,"NPC dialogue current mission");return s.services.mission_ordinal(s.services.owner,out);};
    voice.speech_allowed=[](void*p,uint8_t*out){auto&s=*static_cast<CharacterState*>(p);int32_t dead=0;int r=s.services.target_property(s.services.owner,s.services.player_id(s.services.owner),"dead",&dead);require(r>=0,"NPC speech player death property");
        if(r&&dead){*out=0;return 0;}uint32_t flags=0;require(s.services.world_flags&&!s.services.world_flags(s.services.owner,&flags),"NPC speech world flags");
        if(flags&0x200){int32_t target=s.current_target();if(target!=-1){int32_t type=0;r=s.services.target_property(s.services.owner,target,"character type",&type);require(r>=0,"NPC speech target character type");if(r&&type==5){*out=0;return 0;}}}
        *out=1;return 0;};
    //E321C/219E80 consumes the shared cooldown draw before dedicated Cop voice
    //selection and before root appearance/cadence draws.
    require(services.controller_registration&&!services.controller_registration(services.owner,entity.id,true),"NPC controller registration");s.controller_registered=true;
    s.dialogue=std::make_unique<NativeCharacterDialogue>(*services.dialogue_audio,*services.dialogue_text,random,entity.id,voice);
    {UCEntitySubscriptions*subscriptions=nullptr;require(!uc_entity_subscriptions_create(&s,&subscriptions),"NPC contact event owner");s.subscriptions.reset(subscriptions);
        int inserted=0;require(!uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions),w.listeners(),&inserted)&&inserted,"NPC contact listener registration");
        const UCEntityEventSubscription incoming{0x35,[](void*p,const void*payload,uint32_t*){auto&s=*static_cast<CharacterState*>(p);require(payload&&s.contacts,"NPC incoming contact E2D90");uint32_t id;std::memcpy(&id,payload,4);s.contacts->push_back(id);return 0;}};
        require(!uc_entity_subscriptions_append(subscriptions,&incoming),"NPC incoming contact subscription");
        const UCEntityEventSubscription explosion{9,[](void*p,const void*payload,uint32_t*sender){auto&s=*static_cast<CharacterState*>(p);
            if(!payload)return 1;return s.owner->blast(*static_cast<const UCExplosionBlast*>(payload),sender?(int32_t)*sender:-1);}};
        require(!uc_entity_subscriptions_append(subscriptions,&explosion),"NPC explosion E1570 subscription");}
    s.navigator=std::make_unique<NativeCharacterNavigator>(*w.script_services().logic);
    services.navigation->attach({entity.id,s.navigator.get(),entity.transform+12,entity.actor.group94,CharacterState::ray,CharacterState::scripted,CharacterState::scripted_ray,&s});
    s.bank.load(archive,medic?"Assets_CharacterAnimation\\CharacterSpecific\\CCA_Paramedic\\CCA_Paramedic_Animations.sttxt":class10?"Assets_CharacterAnimation\\CharacterSpecific\\CCA_Cop\\CCA_Cop_Animations.sttxt":brute?"Assets_CharacterAnimation\\CharacterSpecific\\CCA_BurnerBrute\\CCA_BurnerBrute_Animations.sttxt":"Assets_CharacterAnimation\\CharacterSpecific\\CCA_BurnerBlazer\\CCA_BurnerBlazer_Animations.sttxt","Assets_CharacterAnimation\\GCA_Character_template.sttxt");
    static const char*bones[]={"ITEM_Head_01","ITEM_Head_02","ITEM_Head_03","ITEM_Arm_Left","ITEM_Arm_Right","ITEM_Chest","ITEM_Back","ITEM_Leg_Left","ITEM_Leg_Right"};
    for(unsigned j=0;j<9;j++){s.equipment.bones[j]=-1;for(uint32_t i=0;i<s.bank.skeleton.bone_count;i++)if(!_stricmp(bones[j],s.bank.skeleton.bone_names[i]))s.equipment.bones[j]=(int32_t)i;}
    s.model_name=d.asset_name;s.model=std::make_unique<NativeAnimatedModel>();s.model->equipment=&s.equipment;s.model->load(renderer,archive,hash,catalog,d.asset_name,nullptr,nullptr,services.texture_lookup);entity.model=s.model.get();
    require(!uc_animation_binding_init(&s.model->source,&s.bank.skeleton,&s.model->binding),"NPC animation skeleton binding");
    for(uint32_t i=0;i<s.bank.skeleton.bone_count;i++)if(!_stricmp(s.bank.skeleton.bone_names[i],"Bip01 Head"))s.head_bone=(int32_t)i;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};s.model->bind_pose(renderer,identity);
    const char*triggers[]={"FootstepLeft","FootstepRight","BodyHitGround","HideWeaponModel","ShowWeaponModel"};for(unsigned i=0;i<5;i++)s.bind_trigger(triggers[i],s.trigger_flags+i);
    if(cop){int32_t voice_index;require(!uc_game_random_range(&random,0,3,&voice_index)&&voice_index>=0&&voice_index<3,"Cop constructor voice selection");
        const char*banks[]={"Banks\\NPC\\Cops\\CopA.amp","Banks\\NPC\\Cops\\CopB.amp","Banks\\NPC\\Cops\\CopC.amp"};s.load_audio(s.character_audio,banks[voice_index]);}
    else s.load_audio(s.character_audio,generic?d.links[word(d.properties,8)]:dedicated_medic?"Banks\\NPC\\Medics\\Medic.amp":"Banks\\NPC\\Burners\\Blazer.amp");
    if(!generic&&!cop&&!dedicated_medic){auto*r=new Root;r->world=&w;r->skin=(int32_t)word(d.properties,2);s.root=r;
        if(w.bind_dynamic({&r->entity.view,r,Root::step,Root::late,Root::close,nullptr,Root::remove,Root::close}))return 2;
        auto context=w.context();const void*definition="BurnersBlazerBehaviour";context.definition=&definition;*context.registration->creation={0,1,entity.id};
        if(uc_entity_base_construct_unplaced(&context,&r->entity.view)||w.retirement().remove_update_record(w.retirement().owner,r->entity.id))return 2;}
    require(!uc_character_look_init(&s.look),"NPC look constructor"); //E37E9/3BE30, no shared random draw.
    if(int r=entity.script.attach_actor(entity.view,m,w.script_services()))return failure("script owner",r);
    require(services.maintenance_registration&&!services.maintenance_registration(services.owner,entity.id),"NPC maintenance registration"); //E382D/3C960 append.
    std::memcpy(s.previous,entity.transform,64);*w.property_cursor(source)=d.properties.data+d.properties.size;return 0;
}
int NativeMissionCharacter::update(){auto&s=*p_;if(!s.world||s.closed||s.failure)return 1;
    if(s.actor_status&1)return entity.script.update();
    s.hit_time=std::max(0.f,s.hit_time-*s.world->script_services().delta);
    require(s.contacts!=nullptr,"NPC contact list owner");s.contacts->clear(); //E2174/2012C0.
    if(s.actor_status&4){s.pending_events={};s.dialogue->step(*s.world->script_services().delta);return entity.script.update();}
    //E2100/1F9610 consumes annotations retained by the preceding DE730 body
    //advance. Clear registered flags before matching; repeated IDs collapse.
    for(auto&t:s.triggers)*t.flag=0;
    const auto events=s.pending_events;s.pending_events.count=0;UCAnimationEventQueue footsteps{};
    for(uint32_t i=0;i<events.count;i++){const auto&r=events.events[i];require(r.owner&&r.index<r.owner->event_count,"NPC animation event reference");
        const auto&e=r.owner->events[r.index];for(auto&t:s.triggers)if(!_stricmp(t.name,e.type))*t.flag=1;
        if(!_stricmp(e.type,"FootstepLeft")||!_stricmp(e.type,"FootstepRight"))footsteps.events[footsteps.count++]=r;}
    //E2194 only enters E1240/RunFoley for flags1A8/1A9. Other annotations
    //remain published to their real registered owners (not the footstep service).
    if(footsteps.count){require(s.model&&s.services.animation_events,"NPC annotation receiver");
        require(!s.services.animation_events(s.services.owner,entity.id,footsteps,*s.model,entity.transform,s.character_audio),"NPC footstep annotation delivery");}
    for(unsigned i=2;i<5;i++)require(!s.trigger_flags[i],"NPC non-locomotion annotation is unconnected");
    if(s.burning){if(s.burn_done){
            //1D0870 waits for the actual named-animation completion, then
            //records a burning death. The corpse uses the explicit PC adapter.
            require(s.services.held_burn&&!s.services.held_burn(s.services.owner,*this,true),"NPC burning death statistic1D08EC");
            s.death_kind=4;s.death_forces=true;entity.actor.health9C=-1;s.die();
        }else s.dialogue->step(*s.world->script_services().delta);
        return entity.script.update();}
    if(s.root&&s.root->thrown)s.root->thrown->update();
    if(s.root&&s.root->gun){
        //E2260/29F30 ->29E90 precedes deferred DF1A0 startup. Newly created
        //controllers therefore receive their first timer step next frame.
        require(!uc_npc_gun_controller_update(s.root->gun.get(),s.services.npc_gun),"Cop weapon controller29F30");}
    if(s.root&&s.root->melee){auto h=s.root->melee_services();require(!uc_npc_melee_update(s.root->melee.get(),&h),"Medic support controller2C0B0");}
    if(!s.started)s.start();
    if(s.dialogue_face&&!s.dialogue->playing()){s.movement->graph.stop_face();s.dialogue_face=false;
        if(s.dialogue_look)s.movement->graph.stop_look();s.dialogue_look=false;}
    if(s.dialogue_look){require(s.services.target_vector_property,"NPC dialogue look properties");float point[3];
        require(s.services.target_vector_property(s.services.owner,s.services.player_id(s.services.owner),"head pos",point)==1,"NPC dialogue player head");
        //The admitted Cop/Blazer controllers do not override 'allow head turn
        //on speech'; originalE2340 uses its genuine missing-property default1.
        s.movement->graph.look_at(point);}
    s.dialogue->step(*s.world->script_services().delta);s.attention_step();return entity.script.update();}
int NativeMissionCharacter::controller_update(bool expensive){auto&s=*p_;if(!s.world||s.closed||s.failure)return 1;
    if(s.actor_status&1)return 0;
    if(!s.started)return 0; //35ED0: actor164 startup skips timers and the complete AI body.
    if(s.targets.player_seen>0)s.targets.player_seen-=*s.world->script_services().delta;
    if(s.threat_speech_time>0)s.threat_speech_time-=*s.world->script_services().delta;
    //E0870 ignores its expensive argument. Both admitted roots inherit the
    //missing-property defaulttrue;1948B0 Follow owns the sole live override.
    bool ignore_friendly=s.foreground&&s.foreground->kind==Behaviour::Follow&&!s.foreground->friendly_bulletby;
    require(s.services.bullet_by&&!s.services.bullet_by(s.services.owner,entity.id,ignore_friendly,&s.bullet_by144),"NPC bullet-by owner E0870");s.bullet_by148|=s.bullet_by144;
    s.root->update(expensive);if(s.actor_status&1)return 0;
    if(s.burning||(s.actor_status&4))return 0;
    if(s.foreground&&s.foreground->foreground(expensive))return s.failure=2;
    NativeCharacterScriptTasks::Services h{};h.owner=&s;
    h.foreground_run=[](void*p,int32_t state,int32_t index,int32_t*next){auto&s=*static_cast<CharacterState*>(p);
        if(!s.foreground){*next=3;return 0;}require(s.foreground->kind==Behaviour::Interaction||s.foreground->kind==Behaviour::Scripted,"NPC foreground script task owner");return s.foreground->script_runner.run(state,index,next);};
    h.idle=[](void*p){auto&s=*static_cast<CharacterState*>(p);NativeCharacterMoveTarget target;std::copy_n(s.body->world+12,3,target.position);s.movement->request(target);return 0;};
    return s.controller_tasks.update(s.controller_flags,expensive,*s.world->script_services().delta,h);}

uint32_t NativeMissionCharacter::controller_flags()const{return p_->controller_flags;}
bool NativeMissionCharacter::has_controller()const{return p_->controller_registered;}
bool NativeMissionCharacter::tasks_empty()const{require(has_controller(),"NPC task-list controller");return p_->controller_tasks.empty();}
void NativeMissionCharacter::request_retirement(){require(p_->world&&!p_->closed,"NPC retirement owner");request_remove(*p_->world,entity);}
int NativeMissionCharacter::late(){auto&s=*p_;if(s.closed||s.failure)return 1;
    if(s.actor_status&1){s.corpse_step();return 0;}
    if(s.actor_status&4){require(s.ragdoll!=nullptr,"NPC restrained body owner");std::memcpy(s.previous,entity.transform,64);s.ragdoll_step(*s.world->script_services().delta);return 0;}
    //DE730 permits a newly registered actor in the late pass before its first
    //E2100 update. Its body is still NULL;218400 applies virtual7C's actual
    //zero displacement and publishes W=1, without starting the root early.
    if(!s.started){require(!s.body&&!s.movement,"NPC pre-start body lifetime");
        if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.0f;entity.transform[15]=1;}return 0;}
    std::memcpy(s.previous,entity.transform,64);s.movement->update(*s.renderer,*s.model,s.math,*s.world->script_services().delta);
    const auto&events=s.movement->graph.events();
    require(events.count<=16-s.pending_events.count,"NPC pending annotation capacity");
    for(uint32_t i=0;i<events.count;i++)s.pending_events.events[s.pending_events.count++]=events.events[i];
    require(!s.services.refresh_collision(s.services.owner),"NPC collision body refresh");
    std::vector<const UCSolidBlockGeometry*> blocks;for(const auto&b:s.services.collision->boxes)blocks.push_back(b.geometry);
    const float displacement[3]={};s.body->step(*s.model,s.movement->graph.rotation_scale(),*s.services.collision->scenery,blocks.data(),blocks.size(),s.movement->graph.movement_helper(),displacement,s.movement->graph.heading_override(),s.movement->graph.authored_motion());
    std::memcpy(entity.transform,s.body->world,64);
    //DE730 finishes with218C30/218400, whose NPC virtual7C displacement is0.
    if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.0f;entity.transform[15]=1;}
    return 0;}
int NativeMissionCharacter::remove(){auto&s=*p_;if(!s.world||s.closed)return 1;
    if(!s.services.tags||s.services.tags(s.services.owner,entity.id,"INTERESTING_LOOK",false)||s.services.tags(s.services.owner,entity.id,"AUTOAIM",false))return 2;
    entity.flags=(entity.flags&~0x800u)|0x40000;s.release();return retire_fields(*s.world,entity,&s.services);}
int NativeMissionCharacter::close_detached(){auto&s=*p_;if(s.closed)return 0;
    //World close has already removed this actor's records. Its children may
    //have been closed earlier by source-index order; never dereference them.
    s.foreground=nullptr;s.root=nullptr;s.ragdoll.reset();s.corpse_local.reset();s.corpse_pose.reset();s.navigator.reset();s.movement.reset();s.body.reset();s.collision.reset();s.model.reset();entity.model=nullptr;
    if(entity.helper){if(!s.services.release_helper||s.services.release_helper(s.services.owner,entity.helper))return 1;entity.helper=nullptr;}
    s.dialogue.reset();s.contacts.reset();
    if(s.controller_registered){if(s.services.controller_registration(s.services.owner,entity.id,false))return 1;s.controller_registered=false;}
    if(s.faction_joined){if(!s.services.faction||s.services.faction(s.services.owner,entity.id,s.faction,false))return 1;s.faction_joined=false;}
    if(close_fields(entity))return 1;s.closed=true;return 0;}
int NativeMissionCharacter::enter_behaviour(uint32_t source){if(!p_->started)return 1;p_->transition((int32_t)source);return 0;}
bool NativeMissionCharacter::ready()const{return p_->started&&!p_->closed&&!(entity.flags&0x80001);}
void NativeMissionCharacter::append_collision(NativeCharacterWorld&w){
    auto&s=*p_;if(!ready()||(s.actor_status&1))return;require(s.collision&&s.model,"NPC collision lifetime");
    s.collision->update(s.model->source,s.model->prepared,entity.transform);s.collision->append(w);
}
uint32_t NativeMissionCharacter::status()const{return p_->actor_status;}
bool NativeMissionCharacter::navigation_state(NativeCharacterNavigationState&out)const{
    if(!ready()||!p_->navigator)return false;NativeCharacterNavigationState v;
    v.status=p_->navigator->status();v.blocker=p_->navigator->blocker();
    if(auto*b=p_->foreground){v.behaviour=b->entity.source;v.phase=b->move.phase;v.node=b->move.index;}
    std::copy_n(p_->navigator->goal(entity.transform+12),3,v.goal);std::copy_n(p_->navigator->waypoint(entity.transform+12),3,v.waypoint);
    v.movement_busy=p_->movement&&p_->movement->graph.movement_busy();out=v;return true;
}
bool NativeMissionCharacter::order_bounds(UCModelBounds&out)const{
    if(!ready()||!p_->model||!p_->model->prepared.ready)return false;const auto&m=*p_->model;
    require(uc_model_bounds(&m.source,m.prepared.world,m.source.bone_count,m.prepared.world_valid,&out)==UC_MODEL_OK,"NPC order source model bounds");return true;
}
int NativeMissionCharacter::property(const char*name,int32_t*out)const{
    if(!name||!out)return -1;auto&s=*p_;
    if(!std::strcmp(name,"incapacitated"))*out=(s.actor_status&0x20007)!=0;
    else if(!std::strcmp(name,"dead"))*out=(s.actor_status&1)!=0;
    else if(!std::strcmp(name,"invulnerable")||!std::strcmp(name,"shield blocks damage")||!std::strcmp(name,"is sniping")||
            !std::strcmp(name,"no damage")||!std::strcmp(name,"hard to shoot")||!std::strcmp(name,"do damage indicator"))return 0;
    else if(!std::strcmp(name,"burnable")){if(!s.root)return 0;if(s.root->cop||s.root->medic)*out=0;else return 0;}
    else if(!std::strcmp(name,"is character"))*out=1; //E1C24 Boolean literal.
    //E1A50 asks the foreground, then the root. None of the admitted
    //foregrounds publish this;169323 publishes4 only for a stocked TeamMedic.
    else if(!std::strcmp(name,"object type")){if(!s.root||!s.root->medic||s.root->plain_medic||s.root->heals()<=0)return 0;*out=4;}
    else if(!std::strcmp(name,"should duck"))return 0; //No admitted foreground/root publishes this property.
    else if(!std::strcmp(name,"should sidestep")){if(!s.root||!s.root->medic||s.root->plain_medic)return 0;*out=1;
        //16927E resolves the player before querying heals, then only asks
        //the held-order property if a heal remains and player health<100%.
        int32_t player=s.services.player_id(s.services.owner);if(s.root->heals()>0){double health=100.;require(s.services.target_scalar_property,"Medic sidestep player health owner");
            int found=s.services.target_scalar_property(s.services.owner,player,"percent health remaining",&health);require(found>=0,"Medic sidestep health query");if(!found)health=100.;
            if(health<100.){int32_t held=0;found=s.services.target_property(s.services.owner,player,"player holding order button",&held);require(found>=0,"Medic sidestep order query");if(found&&held)*out=0;}}}
    else if(!std::strcmp(name,"avoid targetting"))*out=(s.actor_status&0x8000)!=0;
    else if(!std::strcmp(name,"attacking"))*out=s.targets.current;
    else if(!std::strcmp(name,"number of attackers"))*out=(int32_t)s.attackers.size();
    else if(!std::strcmp(name,"weapon kind"))*out=s.root&&s.root->gun?s.root->gun->type:s.root&&s.root->melee?s.root->melee->type:s.root&&s.root->thrown?s.root->thrown->weapon:-1;
    else if(!std::strcmp(name,"last weapon target")){if(s.root&&s.root->gun){require(!uc_npc_gun_controller_status(s.root->gun.get(),0x18,out),"NPC last gun target");}
        else *out=s.root&&s.root->melee?s.root->melee->last_target:s.root&&s.root->thrown?s.root->thrown->next:-1;} //E1B9D ->2EF70 retained handler14.
    else if(!std::strcmp(name,"target ID")){if(!s.root)return 0;*out=s.current_target();} //36590/36620.
    else if(!std::strcmp(name,"cop dont shoot"))return 0; //Only surrender/death owner subclasses override; none admitted here.
    else if(!std::strcmp(name,"shield active"))return 0; //Only PlayerC3CBF publishes this property; admittedNPC roots do not.
    else if(!std::strcmp(name,"enemy of player")){if(!s.root)return 0;*out=s.root->enemy;}
    else if(!std::strcmp(name,"order id")){if(!s.root)return 0;*out=s.root->order_id;}
    //1C174C returns the dialogue character type (not the body class10).
    //1D3770/36590 do not publish this property for the Blazer root.
    else if(!std::strcmp(name,"character type")){if(!s.root||(!s.root->cop&&!s.root->medic))return 0;*out=s.root->medic?4:2;}
    else if(!std::strcmp(name,"dda enemy type")){if(!s.root||s.root->cop||s.root->medic)return 0;*out=s.root->brute?0:2;} //1D0D93 Brute=0;1D37B3 Blazer=2.
    else if(!std::strcmp(name,"heals left")&&s.root&&s.root->medic)*out=s.root->heals();
    else if(!std::strcmp(name,"allow heal")&&s.root&&s.root->medic)*out=s.root->medic_state.can_heal;
    else if(!std::strcmp(name,"move to heal player")&&s.root&&s.root->plain_medic)*out=s.root->move_to_heal;
    else if(!std::strcmp(name,"is dispatch"))return 0;
    else if(s.foreground&&s.foreground->kind==Behaviour::BruteAttack&&!std::strcmp(name,"team player"))*out=1;
    else if(s.foreground&&s.foreground->kind==Behaviour::BruteAttack&&!std::strcmp(name,"melee"))*out=1;
    else if(s.foreground&&s.foreground->kind==Behaviour::BruteAttack&&!std::strcmp(name,"throwing cleaver"))*out=s.foreground->brute_stage==4;
    else if(s.foreground&&s.foreground->kind==Behaviour::BruteAttack&&!std::strcmp(name,"taunting")){auto t=s.foreground->brute.tasks[3];*out=t&&!reinterpret_cast<CharacterCompletion*>(t)->status;}
    else if(s.foreground&&s.foreground->kind==Behaviour::PlayAnim&&!std::strcmp(name,"straight to ragdoll on tazer"))
        *out=s.foreground->ragdoll_when_tazered&&(!s.root||s.root->enemy); //144AC0/144B10.
    else if(s.foreground&&s.foreground->kind==Behaviour::Loop&&!std::strcmp(name,"can heal"))*out=s.foreground->can_heal;
    else if(s.foreground&&s.foreground->kind==Behaviour::Loop&&!std::strcmp(name,"ok medic drop box"))*out=0;
    else if(s.foreground&&s.foreground->kind==Behaviour::Follow&&!std::strcmp(name,"check friendly bulletby"))*out=s.foreground->friendly_bulletby;
    else return -1;return 1;
}
int NativeMissionCharacter::scalar_property(const char*name,double*out)const{
    if(!name||!out)return -1;auto&s=*p_;if(!std::strcmp(name,"target range")){
        if(!s.foreground||s.foreground->kind!=Behaviour::BruteAttack)return 0;
        if(!s.live_world(s.foreground->threat.current))return 0;*out=s.foreground->threat.distance;return 1;
    }return -1;
}
const char *NativeMissionCharacter::heartbeat_resource()const{auto*r=p_->root;if(!r)return nullptr;
    return r->medic?r->medic_policy.cues[0]:r->cop?r->death_cues[0]:nullptr;}
NativeCharacterDialogue *NativeMissionCharacter::dialogue()const{return p_->dialogue.get();}
int NativeMissionCharacter::bullet(const UCCharacterBullet&b,const UCCharacterBulletImpact&p,float damage,const UCCharacterBulletServices&external){
    auto&s=*p_;if(!ready()||!s.root||!std::isfinite(damage))return 1;
    auto h=external;h.world=entity.transform;h.id=entity.id;h.root_id=s.root->entity.id;h.group=entity.actor.group94;
    h.faction=s.faction_joined;h.controller=s.controller_registered;h.damage=damage;
    return uc_character_bullet(&s.hit_response,&b,&p,&h);
}
int NativeMissionCharacter::blast(const UCExplosionBlast&b,int32_t sender){
    auto&s=*p_;if(!ready()||!s.root||(s.actor_status&1))return 0;
    if(!s.services.player_id||!s.services.blast_obstruction||!std::isfinite(b.damage)||b.damage<0||!std::isfinite(b.radius)||b.radius<0)return 1;
    UCCharacterBlastServices h{};h.owner=&s;h.position=entity.transform+12;h.id=entity.id;h.controller=s.controller_registered;
    h.eligible=[](void*p,const UCExplosionBlast*b,uint8_t*out){auto&s=*static_cast<CharacterState*>(p);
        return uc_explosion_damage_eligible(b->point,b->radius,b->type,b->ignore_collision,s.entity().transform+12,80,140,&s.entity().identity,-1,
            [](void*p,const float*a,const float*b,const void*target,int32_t group){auto&s=*static_cast<CharacterState*>(p);return s.services.blast_obstruction(s.services.owner,a,b,target,group);},&s,out);};
    h.player=[](void*p,int32_t*out){auto&s=*static_cast<CharacterState*>(p);*out=s.services.player_id(s.services.owner);return 0;};
    h.root=[](void*p,int32_t*out,uint32_t*flags,uint8_t*found){auto&s=*static_cast<CharacterState*>(p);
        *found=s.root&&uc_entity_event_lookup(&s.world->registry(),s.root->entity.id);*out=*found?s.root->entity.id:-1;*flags=*found?s.root->protection:0;return 0;};
    h.order=[](void*p,int32_t*out){auto&s=*static_cast<CharacterState*>(p);*out=s.root?s.root->order_id:-1;return 0;};
    h.property=[](void*p,const char*name,uint8_t*out){auto&s=*static_cast<CharacterState*>(p);int32_t value=0;
        if(!std::strcmp(name,"hostage taker")){*out=0;return 0;} //Admitted roots publish no hostage-taker override.
        int found=s.owner->property(name,&value);if(found<0)return 1;*out=found&&value;return 0;};
    h.notify=[](void*p,int32_t id,uint8_t floor,uint8_t player){auto&s=*static_cast<CharacterState*>(p);
        if(id!=s.entity().id)return 1;return s.restrain(floor,player);};
    h.injure=[](void*p,uint32_t flags,float damage,const float*point,int32_t kind,int32_t owner,uint8_t check){
        uint8_t accepted=0;return static_cast<CharacterState*>(p)->owner->injure(flags,damage,point,kind,owner,check,&accepted);};
    h.root_blast=[](void*p,const UCExplosionBlast*b,int32_t sender){auto&s=*static_cast<CharacterState*>(p);auto&r=*s.root;
        //36BA0: admitted Cop/Medic roots leave AC clear. Blazer/Brute roots
        //publish no 'react to explosions' override, selecting36EE0->36450.
        if(!r.damage_enabled||!r.enemy||b->type==5||(s.actor_status&1))return 0;
        return b->radius>0?uc_character_blast_notice(&r.blast_notice,s.entity().transform+12,b->point,b->radius,sender,.2f):0;};
    return uc_character_blast(&s.hit_response,&b,sender,&h);
}
int NativeMissionCharacter::injure(uint32_t flags,float damage,const float point[3],int32_t kind,int32_t owner,uint8_t check,uint8_t*accepted){
    auto&s=*p_;if(!s.started||s.closed||!s.root)return 1;
    UCCharacterInjuryServices h{};h.owner=&s;
    h.root_id=[](void*p,int32_t*out){auto&s=*static_cast<CharacterState*>(p);*out=s.root?s.root->entity.id:-1;return 0;};
    h.enemy=[](void*p,int32_t id,uint8_t*out){auto&s=*static_cast<CharacterState*>(p);if(!s.root||s.root->entity.id!=id)return 1;*out=s.root->enemy;return 0;};
    h.shield=[](void*p,const float*,uint8_t*out){auto&s=*static_cast<CharacterState*>(p);
        //DF9D0 has no blocking owner for the admitted Cop/Medic/Brute/Blazer
        //weapon profiles; shield-carrying enemy classes are not admitted.
        if(!s.root||(!s.root->cop&&!s.root->medic&&!s.root->brute&&!s.root->thrown))return 1;*out=0;return 0;};
    h.resolve=[](void*p,int32_t id)->UCCharacterInjury*{auto&s=*static_cast<CharacterState*>(p);return s.root&&s.root->entity.id==id?&s.root->injury:nullptr;};
    return uc_character_injure(&h,flags,damage,point,kind,owner,check,accepted);
}
int NativeMissionCharacter::set_health(float value,int32_t kind){
    if(!std::isfinite(value))return 1;entity.actor.health9C=value;if(value>entity.actor.maximumA0)entity.actor.maximumA0=value;
    if(kind>=0)p_->death_kind=kind;return 0;
}
int NativeMissionCharacter::held_molotov_burn(){auto&s=*p_;
    if(!s.started||s.closed||!s.root)return 1;
    //DF690's held-Molotov packet: delay0, cause-death1, forever0, type2,
    //force0, immediate1, play-animation1. Dead/already burning return early.
    if(s.actor_status&0x10001)return 0;int32_t burnable=1;int result=property("burnable",&burnable);
    if(result<0)return 1;if(result&&!burnable)return 0;
    if(!s.services.held_burn||s.services.held_burn(s.services.owner,*this,false))return 1;
    s.burning=true;s.actor_status|=0x10000;s.navigator->stop();
    if(s.foreground)s.foreground->exit();s.controller_tasks.clear();s.movement->stop();
    s.movement->graph.stop_aim();s.movement->graph.stop_look();
    if(s.root->thrown)s.root->thrown->drop(); //1D06E0 first drops held weapons.
    s.pending_events={};s.dialogue->say(int32_t(0x80000628u),50);
    int32_t alternate=0;require(!uc_game_random_boolean(s.random,&alternate),"NPC burning animation shared RNG");
    s.burn_completion={&s,[](void*p,bool cancelled){static_cast<CharacterState*>(p)->burn_done=cancelled?2:1;}};
    s.movement->graph.queue_action(alternate?"OnFire":"OnFire2",&s.burn_completion);
    return 0;
}
int NativeMissionCharacter::damage_target(int32_t target){p_->targets.preferred=target;return 0;} //32E30: controller44.
bool NativeMissionCharacter::damage_friendly(int32_t target)const{return p_->faction_joined&&p_->services.factions&&p_->services.factions->friendly((int32_t)p_->faction,target);}
int32_t NativeMissionCharacter::root_id()const{return p_->root?p_->root->entity.id:-1;}
int NativeMissionCharacter::health_body(UCCharacterHealthBody kind,uint8_t*out)const{
    if(!out||!p_->body)return 1;
    //2AB150 reads component+10, the gun stance. Real class1(2B0000) and
    //class2(2B0680) constructors leave it NULL, so371B0 stops here.
    if(kind==UC_HEALTH_OUTER94&&(p_->character_class==1||p_->character_class==2)){*out=0;return 0;}return 1;
}
int NativeMissionCharacter::bullet_relation(int32_t shooter,uint8_t *handled,uint8_t *result)const{
    if(!handled||!result||!p_->world||p_->closed)return 1;
    //E345E installs query61->DE250: a missing actor100 accepts the shot;
    //otherwise39960 asks real faction membership, not the target cache.
    if(p_->faction_joined&&!p_->services.factions)return 1;
    *handled=1;*result=uint8_t(!p_->faction_joined||p_->services.factions->opponent((int32_t)p_->faction,shooter));return 0;
}
const char *NativeMissionCharacter::audio_bank_path()const{return p_->character_audio_path.c_str();}
bool NativeMissionCharacter::head_position(float point[3])const{if(!point||!p_->model||p_->head_bone<0)return false;p_->head_position(point);return true;}
bool NativeMissionCharacter::dialogue_ready(bool death)const{require(p_->dialogue!=nullptr,"NPC dialogue owner");return p_->dialogue->ready(death);}
bool NativeMissionCharacter::dialogue_playing()const{require(p_->dialogue!=nullptr,"NPC dialogue owner");return p_->dialogue->playing();}
bool NativeMissionCharacter::dialogue_owner(int32_t&out)const{require(p_->dialogue!=nullptr,"NPC dialogue owner");return p_->dialogue->speaker_owner(out);}
void NativeMissionCharacter::dialogue_cancel(){require(p_->dialogue!=nullptr,"NPC dialogue owner");p_->dialogue->cancel();}
bool NativeMissionCharacter::dialogue_request(const char*text,int32_t speaker,bool look,bool force){auto&s=*p_;
    require(s.dialogue&&s.services.target_property&&s.services.player_id,"NPC dialogue event owners");
    if(!s.root)return false;int32_t dead=0;int found=s.services.target_property(s.services.owner,s.services.player_id(s.services.owner),"dead",&dead);
    require(found>=0,"NPC dialogue player death property");if(!s.root->enemy&&found&&dead)return false;
    if(force&&s.dialogue->playing())s.dialogue->cancel();
    if(s.actor_status&1)return false;bool result=s.dialogue->request(text,speaker);
    if(result&&s.body&&look)s.dialogue_look=true;return result;
}
bool NativeMissionCharacter::posed_bone(const char*name,float matrix[16])const{
    return posed_bone_local(bone_index(name),matrix);
}
int32_t NativeMissionCharacter::bone_index(const char*name)const{
    if(name)for(uint32_t i=0;i<p_->bank.skeleton.bone_count;i++)if(!_stricmp(name,p_->bank.skeleton.bone_names[i]))return (int32_t)i;
    return -1;
}
bool NativeMissionCharacter::posed_bone_local(int32_t index,float matrix[16])const{
    if(index<0||uint32_t(index)>=p_->bank.skeleton.bone_count||!matrix||!p_->model||!p_->model->prepared.ready||!p_->model->prepared.world_valid[index])return false;
    std::memcpy(matrix,p_->model->prepared.world[index],64);return true;
}
bool NativeMissionCharacter::target_position(float out[3])const{
    float bone[16];if(!out||!p_->body||!posed_bone("Bip01 Spine3",bone))return false;
    const float *m=entity.transform,*p=bone+12;
    for(unsigned i=0;i<3;i++)out[i]=(m[i]*p[0]+m[4+i]*p[1])+(m[8+i]*p[2]+m[12+i]);return true;
}
void NativeMissionCharacter::presentation_world(float fraction,float out[16])const{
    native_interpolate_transform(p_->previous,entity.transform,fraction,out);DirectX::XMFLOAT4X4 a;std::memcpy(&a,out,64);
    //ponytail: the Brute bank has no QuickHitOverlay clip. This brief rigid
    //recoil is a PC presentation adapter until its original reaction owner lands.
    if(p_->hit_time>0&&!(p_->actor_status&1)){
        const float x=a._41,y=a._42,z=a._43;a._41=a._42=a._43=0;
        auto axis=DirectX::XMVectorSet(p_->hit_response.direction[2],0,-p_->hit_response.direction[0],0);
        if(DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis))>.0001f)
            DirectX::XMStoreFloat4x4(&a,DirectX::XMLoadFloat4x4(&a)*DirectX::XMMatrixRotationAxis(axis,.05f*p_->hit_time/.25f));
        a._41=x;a._42=y;a._43=z;
    }
    std::memcpy(out,&a,64);
}
void NativeMissionCharacter::collect_shadows(NativeRenderer&r,NativeModelLighting&lighting,const NativeLightingFrame&frame,NativeModelShadowCasters&casters,float fraction){
    if(!ready()||!NativeModelShadowCasters::eligible(*p_->model,entity.secondary))return;float world[16];presentation_world(fraction,world);p_->model->present(r,fraction);
    casters.add(*p_->model,world,entity.secondary,lighting.prepare_membership(*p_->model,frame,world),frame);
}
void NativeMissionCharacter::draw(NativeRenderer&r,const float vp[16],float exposure,float fraction,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready())return;
    float transform[16];presentation_world(fraction,transform);DirectX::XMFLOAT4X4 a,b,result;std::memcpy(&a,transform,64);std::memcpy(&b,vp,64);
    DirectX::XMStoreFloat4x4(&result,DirectX::XMLoadFloat4x4(&a)*DirectX::XMLoadFloat4x4(&b));const float white[4]={1,1,1,1};NativeTextureContext texture{r.material_phase};
    p_->model->present(r,fraction);if(lighting&&frame)p_->model->draw_lit(r,*lighting,*frame,&a._11,&result._11,white,&texture,exposure);
    else p_->model->draw(r,&result._11,white,&texture,exposure);}
