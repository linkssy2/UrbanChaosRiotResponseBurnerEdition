#include "mission_objective.h"
#include <cmath>
#include <cstring>

bool NativeObjectiveSavedStatus::ensure(int32_t source){return !records_.try_emplace(source,false,false).second;}
bool NativeObjectiveSavedStatus::completed(int32_t source)const{auto p=records_.find(source);return p!=records_.end()&&p->second.first;}
int NativeObjectiveSavedStatus::complete(int32_t source){auto p=records_.find(source);if(p==records_.end())return 1;p->second.first=true;return 0;}
void NativeObjectiveSavedStatus::capture(){for(auto&v:records_)v.second.second=v.second.first;}
void NativeObjectiveSavedStatus::restore(){for(auto&v:records_)v.second.first=v.second.second;}
std::vector<int32_t> NativeObjectiveSavedStatus::saved_completed()const{
    std::vector<int32_t> out;for(const auto&v:records_)if(v.second.second)out.push_back(v.first);return out;
}
void NativeObjectiveSavedStatus::load_saved(const std::vector<int32_t>&sources){
    records_.clear();for(int32_t source:sources)records_.emplace(source,std::make_pair(true,true));
}
int NativeMissionObjective::construct(NativeMissionWorld&w,uint32_t source,NativeObjectiveSavedStatus&saved,const NativeObjectiveServices&s){
    if(world_||source>=w.source_count()||!s.player_ready||!s.message)return 1;
    const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"Objective")||m.asset_present||*w.active_descriptor()!=&m)return 1;
    const uint8_t*cursor=*w.property_cursor(source);int found=0;uint32_t primary=0;
    const char *name="",*secondary="";float timer=0;std::vector<int32_t> activate,fail;
    auto get=[&](uint32_t type,void*out){return !uc_entity_stream_get(d,&cursor,type,out,&found)&&found;};
    auto flags=[&](std::vector<int32_t>&out){int32_t count=0;if(!get(1,&count)||count<0||count>4096)return false;
        out.resize(size_t(count));for(auto&v:out)if(!get(1,&v)||v<0||size_t(v)>=w.script_services().flag_count)return false;return true;};
    if(!get(11,&name)||!get(11,&secondary)||!get(2,&timer)||!flags(activate)||!flags(fail)||!get(3,&primary)||
       !std::isfinite(timer)||cursor!=d->properties.data+d->properties.size||!w.script_services().delta)return 1;
    world_=&w;saved_=&saved;services_=s;
    if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    if(uc_entity_names_bind(w.names(),"OBJECTIVES",entity.id))return failure_=2;named_=true;
    name_=name;secondary_=secondary;timer_=secondary&&*secondary?timer:0;primary_=(primary&255)!=0;
    activate_=std::move(activate);fail_=std::move(fail);*w.property_cursor(source)=cursor;
    if(entity.script.attach_actor(entity.view,m,w.script_services())||coroutine_.create(run,this))return failure_=2;
    return 0;
}
bool NativeMissionObjective::message(int32_t type,const char*title,const char*text){
    if(services_.message(services_.owner,type,title,text)){failure_=2;return false;}return true;
}
void NativeMissionObjective::run(void*p,NativeCoroutine&c){
    auto&a=*(NativeMissionObjective*)p;
    if(a.saved_->ensure(a.entity.source)&&a.saved_->completed(a.entity.source)){
        a.done_=true;a.entity.flags=0x10000000;a.status_=1;
    }
    if(!a.done_){
        if(!a.primary_&&!a.message(0,"Objectives-HUD-3",a.name_))return;
        a.entity.flags&=~0x10000000u;
        while(!a.done_){
            if(a.timer_>0){
                const double remaining=double(a.timer_)-*a.world_->script_services().delta;
                a.timer_=float(remaining);
                if(remaining<=0){a.secondary_shown_=true;if(!a.message(1,"Objectives-HUD-4",a.secondary_))return;}
            }
            size_t active=0;for(int32_t f:a.activate_)if(a.world_->script_services().flags[f])++active;
            if((a.entity.flags&0x10000000u)||(!a.activate_.empty()&&active==a.activate_.size())){
                if(!a.message(3,"Objectives-HUD-1",a.secondary_shown_?a.secondary_:a.name_))return;
                a.done_=true;a.status_=1;if(a.saved_->complete(a.entity.source)){a.failure_=2;return;}
            }else{
                size_t failed=0;for(int32_t f:a.fail_)if(a.world_->script_services().flags[f])++failed;
                if(!a.fail_.empty()&&failed==a.fail_.size()){
                    if(!a.message(4,"Objectives-HUD-2",a.secondary_shown_?a.secondary_:a.name_))return;
                    a.done_=true;a.status_=2;
                }
            }
            if(c.yield()){a.failure_=2;return;}
        }
    }
    for(;;)if(c.yield()){a.failure_=2;return;}
}
int NativeMissionObjective::update(){
    if(!world_||failure_)return failure_?failure_:1;
    bool ready=false;if(services_.player_ready(services_.owner,&ready))return failure_=2;
    if(ready&&(coroutine_.resume()!=0||failure_))return failure_=2;
    if(entity.script.update())return failure_=2;return 0;
}
int NativeMissionObjective::late(){
    if(!world_||failure_)return failure_?failure_:1;
    if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.f;entity.transform[15]=1.f;}return 0;
}
int NativeMissionObjective::close_detached(){
    if(!world_)return 0;
    if(named_&&uc_entity_names_remove(world_->names(),"OBJECTIVES",entity.id))return 1;named_=false;
    activate_.clear();fail_.clear();if(coroutine_.close()||entity.script.close_detached())return 1;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;
    entity.aux=nullptr;world_=nullptr;return 0;
}
int native_objectives_block_checkpoint(NativeMissionWorld&w,uint8_t*out){
    const int32_t*ids=nullptr;uint32_t count=0;
    if(!out||uc_entity_names_view(w.names(),"OBJECTIVES",&ids,&count))return 1;
    uint8_t blocked=0;
    for(uint32_t i=0;i<count;i++){
        auto*t=uc_entity_event_lookup(&w.registry(),ids[i]);if(!t)return 1;
        auto*v=w.context().resolve(w.context().owner,t);
        if(!v||!v->source_index||*v->source_index<0||uint32_t(*v->source_index)>=w.source_count())return 1;
        auto*b=w.binding(uint32_t(*v->source_index));const auto*d=w.metadata(uint32_t(*v->source_index)).definition;
        if(!b||b->view!=v||!b->owner||!d||std::strcmp(d->class_name,"Objective"))return 1;
        auto*objective=static_cast<NativeMissionObjective*>(b->owner);
        if(&objective->entity.view!=v)return 1;
        if(objective->status()==2)blocked=1;
    }
    *out=blocked;return 0;
}
