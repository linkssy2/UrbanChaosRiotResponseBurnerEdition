#include "player_mission.h"
#include <cstring>
#include <cstdlib>
#include <cmath>
extern "C" {
#include "../hud_health.h"
}

int NativeMissionPlayer::construct(NativeMissionWorld&w,uint32_t source,NativeFirstperson&p){
    if(world_||!p.equipped||!p.mission_flags||p.mission_flags!=w.script_services().flags||source>=w.source_count())return 1;
    const auto&m=w.metadata(source);
    if(!p.mission_level||source>=p.mission_level->entity_count||m.definition!=p.mission_level->entities+source||
       std::strcmp(m.definition->class_name,"Mainplayer")||*w.active_descriptor()!=&m)return 1;
    UCMainplayerProperties properties{};
    if(p.mission_flag_count!=p.mission_level->field||uc_mainplayer_properties(p.mission_level,source,&properties))return 1;
    for(size_t at=0;at<properties.flags[0].size;at+=4){uint32_t index;std::memcpy(&index,properties.flags[0].data+at,4);
        if(index>=p.mission_flag_count)return 1;} //No original out-of-bounds flag read.
    entity.view.transform=p.actor.body;
    if(w.bind(source,{&entity.view,this,
        [](void*q){return static_cast<NativeMissionPlayer*>(q)->step();},
        [](void*){return 0;}, // Main's physics step owns the actual body update.
        [](void*q){return static_cast<NativeMissionPlayer*>(q)->close_detached();}}))return 1;
    disable_orders_=properties.flags[0];
    world_=&w;player_=&p;target_={-1,0,300.f,0,{}};uc_player_damage_fresh(&damage_state_);concussion_={};concussion_feedback_=0;
    // Base construction initializes the shared matrix. Restore the already
    // constructed PC player's live pose before publishing its PLAYER name.
    float pose[16];std::memcpy(pose,p.actor.body,sizeof pose);
    int result=uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor());
    std::memcpy(p.actor.body,pose,sizeof pose);
    if(result)return 2;
    std::memcpy(previous_position_,p.actor.body+12,sizeof previous_position_);
    rows_[0]={34,[](void*q,const void*,uint32_t*)->int{auto&p=*static_cast<NativeMissionPlayer*>(q)->player_;
        return uc_firstperson_look_reset(p.actor.angles,p.motion.pitch_limit.current);}};
    rows_[1]={113,[](void*q,const void*,uint32_t*){return static_cast<NativeMissionPlayer*>(q)->take_control();}};
    rows_[2]={166,[](void*q,const void*,uint32_t*){return static_cast<NativeMissionPlayer*>(q)->return_control();}};
    rows_[3]={0x35,[](void*,const void*,uint32_t*){return 0;}}; //D4B6F binds BD660: original RET8, no effects.
    rows_[4]={19,[](void*q,const void*payload,uint32_t*){auto&p=*static_cast<NativeMissionPlayer*>(q)->player_;
        //BD6E0 forwards the exact nine-float packet into actual player+1C4.
        return uc_camera_push_add(&p.camera_push,static_cast<const UCCameraPushInput*>(payload))==UC_CAMERA_PUSH_OK?0:2;}};
    rows_[5]={9,[](void*q,const void*payload,uint32_t*sender){
        return static_cast<NativeMissionPlayer*>(q)->blast(static_cast<const UCExplosionBlast*>(payload),sender);}};
    listener_={rows_,6,this};int inserted=0;
    if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,&listener_,w.listeners(),&inserted)||!inserted||
       uc_entity_names_bind(w.names(),"PLAYER",entity.id))return 2;
    return 0;
}
int NativeMissionPlayer::set_damage_services(const UCPlayerDamageServices&h,const uint32_t*flags,
    uint32_t level,int(*elapsed)(void*),int(*pain_step)(void*)){
    if(!flags||!elapsed||!pain_step||!h.difficulty||!h.multiplayer||!h.sound||!h.pain||!h.feedback||!h.enemy_type||!h.adaptive_hit||!h.delta)return 1;
    damage_services_=h;damage_world_flags_=flags;selected_level74_=level;damage_elapsed_gate_=elapsed;damage_pain_step_=pain_step;return 0;
}
int NativeMissionPlayer::step(){
    if(uc_entity_fresh_aux_clear(entity.aux))return 1;
    if(!damage_world_flags_)return 0; // Existing non-damage preview adapter scope.
    int enabled=damage_elapsed_gate_(damage_services_.owner);if(enabled<0)return 2;
    return uc_player_damage_age(&damage_state_,damage_services_.delta(damage_services_.owner),enabled?1:0)?2:0;
}
int NativeMissionPlayer::after_camera_response(){
    if(!player_||!world_)return 1;
    //D2FBA C0C80, D2FC1 C0D70, D2FC8 C4980, then common D353A691B0.
    //The PC player still owns movement outside this entity adapter.
    const auto&a=player_->action;
    if(blast_services_.shared_attenuation&&a.state!=12&&a.state!=13&&!(a.state==11&&a.stage!=8)){
        int enabled=blast_services_.enabled(blast_services_.owner);if(enabled<0||enabled>1)return 2;
        if(enabled){
        UCPlayerConcussionServices h{};h.owner=this;h.shared_attenuation=blast_services_.shared_attenuation;h.feedback=&concussion_feedback_;
        h.group_pitch=[](void*q,const char*name,float ratio){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.group_pitch(h.owner,name,ratio);};
        h.unscaled_delta=[](void*q)->double{return static_cast<NativeMissionPlayer*>(q)->world_->unscaled_delta();};
        h.clock=[](void*q)->double{return static_cast<NativeMissionPlayer*>(q)->world_->time();};
        if(uc_player_concussion_step(&concussion_,&h))return 2;
        }
    }
    return damage_world_flags_?damage_pain_step_(damage_services_.owner):0;
}
int NativeMissionPlayer::set_impact_services(const NativePlayerImpactServices&h){
    if(!h.statistic||!h.shield_sound||!h.shield_spark)return 1;impact_services_=h;return 0;
}
int NativeMissionPlayer::set_blast_services(const NativePlayerBlastServices&h){
    if(!h.shared_attenuation||!h.obstruction||!h.secondary_pain||!h.sound||!h.parameter||!h.bus_filter||!h.group_pitch||!h.enabled)return 1;
    blast_services_=h;return 0;
}
int NativeMissionPlayer::blast(const UCExplosionBlast*b,const uint32_t*sender){
    if(uc_player_blast_quiet(b)==1)return 0;
    if(!b||!sender||!world_||!player_||!damage_world_flags_||!blast_services_.shared_attenuation)return 1;
    int multiplayer=damage_services_.multiplayer(damage_services_.owner);if(multiplayer<0||multiplayer>1)return 1;
    UCPlayerBlastServices h{};h.owner=this;h.target=&entity.identity;
    h.obstruction=[](void*q,const float*from,const float*to,const void*target,int32_t group){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.obstruction(h.owner,from,to,target,group);};
    h.pain=[](void*q){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.secondary_pain(h.owner);};
    h.damage=[](void*q,float amount,const UCPlayerFireDamage*d){return static_cast<NativeMissionPlayer*>(q)->damage(amount,*d);};
    h.sound=[](void*q,const char*name){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.sound(h.owner,name);};
    h.parameter=[](void*q,const char*event,const char*name,float value){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.parameter(h.owner,event,name,value);};
    h.bus_effect=[](void*q,const char*name,float cutoff,float attack,float hold,float release){auto&h=static_cast<NativeMissionPlayer*>(q)->blast_services_;return h.bus_filter(h.owner,name,cutoff,attack,hold,release);};
    h.delta=[](void*q)->double{return *static_cast<NativeMissionPlayer*>(q)->world_->script_services().delta;};
    return uc_player_blast_receive(&concussion_,entity.view.transform+12,(int32_t)*sender,b,nullptr,(uint8_t)multiplayer,&h);
}
int NativeMissionPlayer::damage(float amount,const UCPlayerFireDamage&context){
    if(!player_||!world_||!damage_world_flags_)return 1;
    auto&p=*player_;UCPlayerDamageInput i{};
    i.health=&p.inventory.health;i.maximum=&p.health_maximum;i.frame=p.camera_input.player_frame;
    i.shield=p.equip_control;i.action=p.action.state;i.item=p.inventory.active_item;
    i.shield_impact=p.shield_impact;i.scale=1.f; // Actual D3910 fresh actorD4.
    i.world_flags=*damage_world_flags_;i.selected_level74=selected_level74_;
    i.locked=p.switching.blocked; // Shared actual actor202; fresh3A8 remains0.
    if(context.kind==5){
        if(!p.random||!impact_services_.statistic||!impact_services_.shield_sound||!impact_services_.shield_spark)return 1;
        UCPlayerImpactServices h{};h.owner=this;h.position=entity.view.transform+12;h.has_point_push=1;
        h.random=[](void*q,float a,float z)->double{auto&p=*static_cast<NativeMissionPlayer*>(q)->player_;int32_t n;
            if(uc_game_random_next(p.random,&n))return NAN;return((double)n*((double)z-a))*0x1p-31+a;};
        h.statistic=[](void*q,unsigned offset){auto&h=static_cast<NativeMissionPlayer*>(q)->impact_services_;return h.statistic(h.owner,offset);};
        h.shield_sound=[](void*q,const char*name){auto&h=static_cast<NativeMissionPlayer*>(q)->impact_services_;return h.shield_sound(h.owner,name);};
        h.shield_spark=[](void*q,const float*point,const float*rotation){auto&h=static_cast<NativeMissionPlayer*>(q)->impact_services_;return h.shield_spark(h.owner,point,rotation);};
        h.shield_recoil=[](void*q,unsigned type,float,const float*direction){auto&p=*static_cast<NativeMissionPlayer*>(q)->player_;
            return uc_player_shield_recoil(&p.shield_recoil,&p.impact,p.camera_input.player_frame,type,direction,p.random);};
        h.shield_point=[](void*q,float*point){return static_cast<NativeMissionPlayer*>(q)->player_->shield_surface(point);};
        h.point_push=[](void*q,unsigned slot,const float*point,float force,float min,float max){auto&p=*static_cast<NativeMissionPlayer*>(q)->player_;
            return uc_camera_push_point(&p.camera_push,slot,point,force,min,max)==UC_CAMERA_PUSH_OK?0:1;};
        return uc_player_damage_impact(&damage_state_,&p.impact,&i,amount,&context,&damage_services_,&h);
    }
    return uc_player_damage_environment(&damage_state_,&i,amount,&context,&damage_services_);
}
static bool quiet(const NativeFirstperson&p){
    return p.equipped&&!p.action.animation&&!p.action.stage&&!p.action.input_flags&&
        !p.preview_firing&&!p.preview_shot_pending&&!p.reload_requested&&!p.equipped->state.pending_reload&&
        !p.shield_temporary&&!p.shield_bashing&&!p.shield_bash_pending&&!p.taser_phase&&!p.throw_stage&&
        !p.throw_capture_pending&&!p.switching.lowered&&!p.switching.raising&&!p.switching.restore_lowered&&
        !p.equip_control.select_empty&&!p.equip_control.select_previous&&
        p.inventory.active_item==p.inventory.requested_item;
}
int NativeMissionPlayer::publish_target(const UCFirstpersonAimCollision&hit){
    return !player_||uc_firstperson_aim_hit_begin(&hit,&target_)!=UC_MOTION_OK;
}
int NativeMissionPlayer::target_scalar_property(const char*name,double&out,uint32_t mode,uint32_t tier)const{
    if(!player_||!name)return -1;
    if(!std::strcmp(name,"health")){out=player_->inventory.health;return 1;} //218120
    if(!std::strcmp(name,"target distance")){out=target_.distance;return 1;} //C4242
    if(!std::strcmp(name,"percent health remaining")){ //C3BC8: float property publication, BDAB0 cap.
        out=(float)((double)player_->inventory.health/uc_hud_health_maximum(mode,tier)*100.);return 1;}
    return -1;
}
int NativeMissionPlayer::target_property(const char*name,int32_t&out)const{
    if(!player_||!name)return -1;
    if(!std::strcmp(name,"has control")){ //C39C7: actual action170 and actor202.
        out=player_->action.state!=12&&player_->action.state!=13&&!player_->switching.blocked;return 1;}
    if(!std::strcmp(name,"orders valid")){ //C3C0D ->210790 ANY disable flag, then negate.
        for(size_t at=0;at<disable_orders_.size;at+=4){uint32_t index;std::memcpy(&index,disable_orders_.data+at,4);
            if(index>=player_->mission_flag_count||!player_->mission_flags)return -1;
            if(player_->mission_flags[index]){out=0;return 1;}}
        out=1;return 1;}
    if(!std::strcmp(name,"target id")){out=target_.entity;return 1;} //C3DFA
    if(!std::strcmp(name,"target surface")){out=(int32_t)target_.detail;return 1;} //C4275
    if(!std::strcmp(name,"dead")){out=player_->action.state==12;return 1;} //C3A3C
    if(!std::strcmp(name,"shield active")){ //C3CBF ->41F50 and BBD60
        out=(player_->inventory.active_item==0||player_->equip_control.select_empty)&&
            !player_->equip_control.select_previous&&!(player_->shield_impact>=100.f);return 1;}
    if(!std::strcmp(name,"incapacitated")){
        // BDB80 with the actual fresh3A8/202/world10 values, followed by
        // C3811's separate !3A8 gate. State170 is the live action owner.
        out=player_->action.state==12||player_->action.state==13;return 1;
    }
    if(!std::strcmp(name,"weapon kind")){
        // BBD80: lowered2AC or absent active4FC; BE210 resolves that item's
        // descriptor. The PC equipped pointer already owns that descriptor.
        if(player_->switching.lowered||player_->inventory.active_item==-1){out=-1;return 1;}
        if(!player_->equipped)return -1;
        auto category=player_->equipped->spec.category;
        out=category==0?0:category==1?3:category==2?2:-1;return 1;
    }
    if(!std::strcmp(name,"avoid targetting")||!std::strcmp(name,"attacking")||
       !std::strcmp(name,"number of attackers"))return 0; // C4362, no base delegation.
    return -1;
}
int NativeMissionPlayer::snapshot_motion(){
    if(!player_)return 1;
    // D00D8 saves the pre-integration position; D05C7 publishes it at138
    // immediately before D05FF publishes the new position at40.
    std::memcpy(previous_position_,player_->actor.body+12,sizeof previous_position_);return 0;
}
int NativeMissionPlayer::target_vector_property(const char*name,float out[3])const{
    if(!player_||!name||!out)return -1;
    //C379D -> C0EC0 -> C01C0(selector0): the published6B0 matrix,
    //including its source head/sway placement, with no additional eye offset.
    if(!std::strcmp(name,"head pos")){std::memcpy(out,player_->camera_input.player_frame+12,12);return 1;}
    if(!std::strcmp(name,"head direction")){std::memcpy(out,player_->camera_input.player_frame+8,12);return 1;} //C3725,C01C0 selector0
    if(std::strcmp(name,"movement"))return -1;
    for(unsigned i=0;i<3;i++)out[i]=player_->actor.body[12+i]-previous_position_[i];return 1;
}
int NativeMissionPlayer::take_control(){
    if(!player_||player_->action.state||!quiet(*player_))return 1;
    auto&p=*player_;
    if(uc_player_equipment_idle_transition(&p.action,13,0)!=UC_MAINPLAYER_OK)return 1;
    p.set_fire(false);p.set_shield(false);p.switching.lowering_mode=0;return 0;
}
int NativeMissionPlayer::target_matrix_property(const char*name,float out[16])const{
    if(!player_||!name||!out||std::strcmp(name,"Look point"))return -1;
    std::memcpy(out,player_->camera_input.player_frame,64);return 1; //C36D3 direct6B0 publication
}
int NativeMissionPlayer::return_control(){
    if(!player_||player_->action.state!=13||!quiet(*player_))return 1;
    auto&p=*player_;
    try{
        // Recreate real weapon/arms controllers and invalidate interpolation
        // history before allowing PC input again. Inventory/ammo are retained.
        p.commit_equipment();p.apply_action(0,-1,1);
        p.switching.dirty0=p.switching.dirty1=1;
        if(uc_player_equipment_idle_transition(&p.action,0,0)!=UC_MAINPLAYER_OK)return 2;
    }catch(...){return 2;}
    return 0;
}
int NativeMissionPlayer::close_detached(){
    if(world_&&uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
    if(world_)uc_entity_names_remove(world_->names(),"PLAYER",entity.id);
    for(auto&l:entity.identity.listeners)if(l==&listener_)l=nullptr;
    if(uc_entity_fresh_aux_destroy(entity.aux))return 1;
    entity.aux=nullptr;world_=nullptr;player_=nullptr;disable_orders_={};return 0;
}
int NativePlayerCamera::construct(NativeMissionWorld&w,NativeMissionPlayer&p,unsigned width,unsigned height){
    if(world_||!p.player()||!width||!height||uc_entity_event_lookup(&w.registry(),p.entity.id)!=&p.entity.identity)return 1;
    if(resize(width,height))return 1;
    stack.entities=&w.context();stack.owner=this;
    stack.current_viewport=[](void*q){return &static_cast<NativePlayerCamera*>(q)->viewport;};
    stack.memory={nullptr,[](void*){return static_cast<UCSceneViewport*>(std::malloc(sizeof(UCSceneViewport)));},
        [](void*,UCSceneViewport*q){std::free(q);}};
    entity.view.transform=p.player()->camera_world;
    if(w.bind_dynamic({&entity.view,this,
        [](void*q){return uc_entity_fresh_aux_clear(static_cast<NativePlayerCamera*>(q)->entity.aux);},
        [](void*){return 0;}, // Firstperson.after_physics owns the camera update.
        [](void*q){return static_cast<NativePlayerCamera*>(q)->close_detached();}}))return 1;
    world_=&w;auto context=w.context();const void*definition="PCGameplayCamera";context.definition=&definition;
    float pose[16];std::memcpy(pose,entity.view.transform,sizeof pose);
    int result=uc_entity_fresh_construct_unplaced(&context,&entity.view);
    std::memcpy(entity.view.transform,pose,sizeof pose);
    if(result||uc_entity_names_bind(w.names(),"CAMERA",entity.id)||
       uc_scene_camera_stack_activate(&stack,&token,p.entity.id))return 2;
    return 0;
}
bool NativePlayerCamera::selected()const{
    UCSceneCamera*current=nullptr;
    return world_&&!uc_scene_camera_stack_current(&stack,0,&current)&&current==&token;
}
int NativePlayerCamera::close_detached(){
    if(world_){
        if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
        uc_entity_names_remove(world_->names(),"CAMERA",entity.id);
        if(uc_scene_camera_stack_remove(&stack,&token))return 1;
    }
    if(uc_entity_fresh_aux_destroy(entity.aux))return 1;
    entity.aux=nullptr;world_=nullptr;return 0;
}
