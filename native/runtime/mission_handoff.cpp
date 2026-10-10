#include "mission_handoff.h"
#include "mission_objective.h"
#include "character_dialogue.h"
#include <cstring>
#include <stdexcept>

NativeMissionHandoff::NativeMissionHandoff(NativeMissionWorld&w,NativeMissionPlayer&p,NativePlayerCamera&c,
    NativeRenderer&r,NativePlayerAudio&a,const UCArchive*ar,const uint8_t hash[32],const std::filesystem::path&assets)
    :world_(w),player_(p),camera_(c),renderer_(r),audio_(a),archive_(ar),catalog_(assets/"1-01.ent.ucmat"){
    if(!p.player()||!c.selected()||!ar||!hash)throw std::invalid_argument("Mission handoff requires the live player and camera");
    std::memcpy(hash_.data(),hash,32);auto bytes=read_file(assets/"animation-coefficients.bin",16384);
    if(uc_animation_blend_math_load(bytes.data(),bytes.size(),&math_))throw std::runtime_error("Mission animation coefficients");
    // Same fresh, unupgraded PC loadout profile used by NativeFirstperson::load.
    const uint8_t upgrades[36]{};for(unsigned i=0;i<27;i++)if(uc_mainplayer_item_spec(i,0,upgrades,specs+i))throw std::runtime_error("Mission item definition");
    fade_services_={&world_.registry(),this,[](void*p){++((NativeMissionHandoff*)p)->white_depth_;return 0;},
        [](void*p){auto&s=*(NativeMissionHandoff*)p;if(!s.white_depth_)return 1;--s.white_depth_;return 0;}};
}
bool NativeMissionHandoff::supports(const char*n)const{return n&&(!std::strcmp(n,"RestartPoint")||!std::strcmp(n,"SimpleCutscene"));}
NativeSimpleCutscene *NativeMissionHandoff::scene()const{for(auto&p:scenes_)if(p)return p.get();return nullptr;}
NativeRestartPoint *NativeMissionHandoff::restart(int32_t n)const{for(auto&p:restarts_)if(p->state().restart.number==n)return p.get();return nullptr;}
void NativeMissionHandoff::load_feedback(const std::filesystem::path&game,NativeDialogueText&text){
    if(feedback_loaded_)throw std::runtime_error("Handoff feedback already loaded");
    const auto*line=text.find("NewFrontEnd-Checkpoint-1");
    messages_.load(game,renderer_,line&&!line->variants.empty()?line->variants[0].text:"NewFrontEnd-Checkpoint-1");
    stream_=game/"5.saf";feedback_text_=&text;feedback_loaded_=true;
}
int NativeMissionHandoff::hud_message(const char*resource){
    if(!resource||!feedback_text_)return 1;
    const auto*line=feedback_text_->find(resource);
    return messages_.add(line&&!line->variants.empty()?line->variants[0].text.c_str():resource);
}
int NativeMissionHandoff::capture_player(){
    if(!player_.player())return 1;int32_t n=checkpoints.current(&session_mode);
    UCCheckpointSnapshotInput in{&player_.player()->inventory,specs,item_extra,&flashlight,&n,mission,statistics};
    int result=uc_checkpoint_snapshot(&in,&snapshot);if(!result)++snapshots;return result;
}
int NativeMissionHandoff::request_restart(int32_t number){
    //1154D9: select the restart, save CURRENT player state, then world+10|=2.
    //The top-level world owner must consume bit2 and recreate OnRestart actors.
    checkpoints.selected=number;if(capture_player())return 1;world_flags|=2;return 0;
}
int NativeMissionHandoff::checkpoint_sound(){
    if(!feedback_loaded_)return 1;auto*p=restart(checkpoints.current(&session_mode));if(!p)return 1;
    auto i=checkpoint_audio_.find(p);
    if(i==checkpoint_audio_.end()){
        if(p->entity.helper)return 1;auto source=std::make_unique<NativePlayerAudio::MissionSource>(audio_,archive_,find_resource(archive_,"banks\\panel.amp"),stream_,true);
        auto*helper=source.get();i=checkpoint_audio_.emplace(p,std::move(source)).first;p->entity.helper=helper;
    }
    if(p->entity.helper!=i->second.get())return 1;
    //136E30 ignores216BD0's accepted byte; real arbitration may reject a voice.
    i->second->play("Checkpoint");return 0;
}
int NativeMissionHandoff::control_event(int32_t id,uint32_t event,uint32_t sender){
    if(id!=player_.entity.id||!player_.player())return 1;
    uint32_t payload=0;
    uint8_t handled=0;int result=uc_entity_event_send(&world_.registry(),id,event,&payload,4,&sender,&handled);
    if(result||!handled)return 1;if(event==113)++takeovers;else if(event==166)++returns;return 0;
}
NativeSimpleCutsceneServices NativeMissionHandoff::scene_services(){
    NativeSimpleCutsceneServices s{};s.camera.owner=this;
    s.camera.preflight=[](void*p){auto&s=*(NativeMissionHandoff*)p;return !s.player_.player()?1:0;};
    s.camera.current_provider=[](void*p)->void*{auto&s=*(NativeMissionHandoff*)p;return s.player_.player()?p:nullptr;};
    s.camera.player=[](void*p,void*v,int32_t i,int32_t*out){auto&s=*(NativeMissionHandoff*)p;if(v!=p||i||!out||!s.player_.player())return 1;*out=s.player_.entity.id;return 0;};
    s.camera.control=[](void*p,int32_t id){auto&s=*(NativeMissionHandoff*)p;auto*c=s.scene();return c&&c->camera()?s.control_event(id,113,(uint32_t)c->camera()->entity.id):1;};
    s.camera.return_control=[](void*p,int32_t id,uint32_t sender){return ((NativeMissionHandoff*)p)->control_event(id,166,sender);};
    s.camera.mode=[](void*p,void*v,int32_t mode,int32_t argument){auto&s=*(NativeMissionHandoff*)p;if(v!=p||argument||(mode!=6&&mode!=-1))return 1;s.camera_mode=mode;return 0;};
    s.provider_ready=[](void*p,void*v,uint8_t*out){auto&s=*(NativeMissionHandoff*)p;if(v!=p||!out)return 1;*out=s.checkpoints.ready(s.world_,&s.session_mode);return 0;};
    s.skip_input=[](void*p,double*out){if(!out)return 1;*out=((NativeMissionHandoff*)p)->skip?1.:0.;return 0;};
    s.restart=[](void*p,int32_t number){return ((NativeMissionHandoff*)p)->request_restart(number);};s.history=&history;return s;
}
NativeRestartPointServices NativeMissionHandoff::restart_services(){
    NativeRestartPointServices s{};auto&o=s.operations;o.owner=this;o.save_pending=&save_pending;
    o.player=[](void*p,void**out){if(!out)return 1;*out=((NativeMissionHandoff*)p)->player_.player();return 0;};
    o.session=[](void*p,uint32_t*out){if(!out)return 1;*out=((NativeMissionHandoff*)p)->session_mode;return 0;};
    o.dead=[](void*p,void*player,uint8_t*out){auto&s=*(NativeMissionHandoff*)p;if(!out||player!=s.player_.player()||!player)return 1;*out=s.player_.player()->inventory.health<=0;return 0;};
    o.objectives_block=[](void*p,uint8_t*out){return native_objectives_block_checkpoint(((NativeMissionHandoff*)p)->world_,out);};
    o.current=[](void*p,int32_t*out){auto&s=*(NativeMissionHandoff*)p;if(!out)return 1;*out=s.checkpoints.current(&s.session_mode);return 0;};
    o.select=[](void*p,int32_t n){((NativeMissionHandoff*)p)->checkpoints.selected=n;return 0;};
    o.capture_flags=[](void*p){auto&s=*(NativeMissionHandoff*)p;if(!s.objective_saved_)return 1;s.objective_saved_->capture();return 0;};
    o.capture_player=[](void*p){return ((NativeMissionHandoff*)p)->capture_player();};
    o.sound=[](void*p){return ((NativeMissionHandoff*)p)->checkpoint_sound();};
    o.message=[](void*p){auto&s=*(NativeMissionHandoff*)p;return s.feedback_loaded_?s.messages_.add():1;};
    //136F98 calls16A60: adaptive difficulty reward, not a disk save.
    o.save=[](void*p){auto&s=*(NativeMissionHandoff*)p;return uc_game_difficulty_checkpoint(s.difficulty_,s.adaptive_disabled);};
    o.event=[](void*p,uint32_t i,uint32_t group){auto&s=*(NativeMissionHandoff*)p;uint8_t accepted=0;return s.world_.event(i,group,&accepted)?-2:accepted;};
    s.register_point=[](void*p,int32_t n,int32_t id){((NativeMissionHandoff*)p)->checkpoints.register_point(n,id);return 0;};
    s.close_helper=[](void*p,void*helper){auto&s=*(NativeMissionHandoff*)p;for(auto i=s.checkpoint_audio_.begin();i!=s.checkpoint_audio_.end();++i)if(i->second.get()==helper){s.checkpoint_audio_.erase(i);return 0;}return 1;};return s;
}
int NativeMissionHandoff::construct(uint32_t i){
    if(i>=world_.source_count())return 1;const auto*d=world_.metadata(i).definition;if(!d||!supports(d->class_name))return 1;
    if(!std::strcmp(d->class_name,"RestartPoint")){auto p=std::make_unique<NativeRestartPoint>();auto*raw=p.get();restarts_.push_back(std::move(p));return raw->construct(world_,i,restart_services());}
    if(scene())return 1;auto p=std::make_unique<NativeSimpleCutscene>();auto*raw=p.get();scenes_.push_back(std::move(p));
    if(world_.bind(i,{&raw->entity.view,this,[](void*p){return ((NativeMissionHandoff*)p)->scene()->update();},[](void*p){return ((NativeMissionHandoff*)p)->scene()->late();},
        [](void*p){return ((NativeMissionHandoff*)p)->scene()->close_detached();},&raw->entity.script,[](void*p){return ((NativeMissionHandoff*)p)->scene()->remove();},
        [](void*p){auto&s=*(NativeMissionHandoff*)p;if(s.scene()->close_detached())return 1;s.scenes_[0].reset();return 0;}}))return 1;
    return raw->construct(renderer_,world_,i,archive_,hash_.data(),catalog_,math_,audio_,camera_.stack,&events_[0],&events_[1],fade,fade_services_,scene_services());
}
int NativeMissionHandoff::helper_tick(void*helper,float delta){
    for(auto&p:checkpoint_audio_)if(p.second.get()==helper){p.second->step(delta);return 0;}
    auto*s=scene();if(!s||helper!=s->entity.helper)return 1;
    audio_.tick_empty_source((NativePlayerAudio::EmptySource*)helper,delta);return 0;
}
