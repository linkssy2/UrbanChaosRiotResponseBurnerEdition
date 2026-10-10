#include "restart_point.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
const NativeRestartPoint *NativeCheckpointManager::find(NativeMissionWorld&w,int32_t number)const{
    if(number==-1)return nullptr;auto it=points_.find(number);if(it==points_.end())return nullptr;
    auto*target=uc_entity_event_lookup(&w.registry(),it->second);const auto&context=w.context();
    auto*view=target?context.resolve(context.owner,target):nullptr;
    if(!view||!view->source_index||*view->source_index<0||!view->definition70||!*view->definition70||
       std::strcmp((const char*)*view->definition70,"RestartPoint"))throw std::runtime_error("Checkpoint registry lost its RestartPoint owner");
    auto*binding=w.binding((uint32_t)*view->source_index);
    if(!binding||binding->view!=view||!binding->owner)throw std::runtime_error("Checkpoint binding changed");
    return (const NativeRestartPoint*)binding->owner;
}
bool NativeCheckpointManager::ready(NativeMissionWorld&w,const uint32_t*mode)const{auto*p=find(w,current(mode));return !p||p->state().restart_ready!=0;}
bool NativeCheckpointManager::auto_fade(NativeMissionWorld&w,const uint32_t*mode)const{auto*p=find(w,current(mode));return !p||p->state().restart.auto_fade!=0;}
bool NativeCheckpointManager::transform(NativeMissionWorld&w,const uint32_t*mode,float out[16])const{
    if(!out)throw std::invalid_argument("Missing checkpoint transform output");auto*p=find(w,current(mode));if(!p)return false;
    std::memcpy(out,p->entity.transform,64);return true;
}
int NativeRestartPoint::construct(NativeMissionWorld&w,uint32_t source,const NativeRestartPointServices&s){
    const auto&o=s.operations;
    if(world_||source>=w.source_count()||!s.register_point||!o.save_pending||!o.player||!o.session||!o.dead||
       !o.objectives_block||!o.current||!o.select||!o.capture_flags||!o.capture_player||!o.sound||!o.message||!o.save||!o.event)return 1;
    const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||!d->class_name||std::strcmp(d->class_name,"RestartPoint")||m.asset_present||*w.active_descriptor()!=&m)return 1;
    auto&p=checkpoint_;p.restart.entity_index=source;p.restart.number=0;p.restart.auto_fade=1;
    const uint8_t*cursor=*w.property_cursor(source);int found=0;uint32_t fade=1;
    p.restart.on_restart[1]=p.restart.on_operated[1]=p.restart.on_post_restart[1]=UINT32_MAX;
    if(uc_entity_stream_get(d,&cursor,1,&p.restart.number,&found)||
       uc_entity_stream_get(d,&cursor,5,p.restart.on_restart,&found)||
       uc_entity_stream_get(d,&cursor,5,p.restart.on_operated,&found)||
       uc_entity_stream_get(d,&cursor,5,p.restart.on_post_restart,&found)||
       uc_entity_stream_get(d,&cursor,3,&fade,&found)||cursor!=d->properties.data+d->properties.size)return 1;
    p.restart.auto_fade=(fade&255)!=0;
    if(w.bind(source,{&entity.view,this,[](void*q){return ((NativeRestartPoint*)q)->update();},
       [](void*q){return ((NativeRestartPoint*)q)->late();},[](void*q){return ((NativeRestartPoint*)q)->close_detached();},&entity.script}))return 1;
    world_=&w;services_=s;
    if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    *w.property_cursor(source)=cursor;
    std::memcpy(p.restart.transform,entity.transform,sizeof entity.transform);
    if(entity.script.attach_actor(entity.view,m,w.script_services())||primary_.create(run,this)||
       s.register_point(o.owner,p.restart.number,entity.id))return failure_=2;
    p.remaining_delay=.25f;p.initialized=1;
    if(uc_entity_subscriptions_create(this,&subscriptions_))return failure_=2;
    int inserted=0;
    if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions_),w.listeners(),&inserted)||!inserted)return failure_=2;
    UCEntityEventSubscription subscription{0xBF,[](void*q,const void*,uint32_t*){((NativeRestartPoint*)q)->pending_=1;return 0;}};
    return uc_entity_subscriptions_append(subscriptions_,&subscription)?failure_=2:0;
}
int NativeRestartPoint::event(void*q,uint32_t source,uint32_t group){
    auto&p=*(NativeRestartPoint*)q;if(source!=p.checkpoint_.restart.entity_index)return -2;
    uint8_t accepted=0;return p.entity.script.event(group,1,&accepted)?-2:accepted;
}
int NativeRestartPoint::poll(void*q,uint32_t source){auto&p=*(NativeRestartPoint*)q;return source!=p.checkpoint_.restart.entity_index?-2:int((p.entity.flags&0x40000000)!=0);}
void NativeRestartPoint::run(void*q,NativeCoroutine&fiber){
    auto&p=*(NativeRestartPoint*)q;auto&o=p.services_.operations;p.entity.flags|=0x100;
    int32_t current;if(o.current(o.owner,&current)){p.failure_=2;return;}
    if(current==p.checkpoint_.restart.number){
        for(;;){
            auto status=uc_checkpoint_resume(&p.checkpoint_,*p.world_->script_services().delta,event,poll,&p);
            if(status!=UC_CHECKPOINT_YIELD&&status!=UC_CHECKPOINT_POST_STARTED&&status!=UC_CHECKPOINT_PARKED){p.failure_=2;return;}
            if(fiber.yield()){p.failure_=2;return;}
        }
    }
    p.checkpoint_.reset_complete=1;
    for(;;){
        auto operation=o;operation.pending=&p.pending_;operation.parked=&p.parked_;
        operation.number=&p.checkpoint_.restart.number;operation.group=&p.checkpoint_.restart.on_operated[1];
        operation.entity=p.checkpoint_.restart.entity_index;
        if(uc_checkpoint_operate(&operation)||fiber.yield()){p.failure_=2;return;}
    }
}
int NativeRestartPoint::update(){
    if(!world_||failure_)return failure_?failure_:1;
    UCCheckpointPump pump{&checkpoint_.restart_ready,&checkpoint_.reset_complete,&checkpoint_.restart.number,this,
        [](void*q){auto&p=*(NativeRestartPoint*)q;return p.primary_.resume()<0||p.failure_?1:0;},
        [](void*q){return ((NativeRestartPoint*)q)->entity.script.update();},
        [](void*q,int32_t*out){auto&p=*(NativeRestartPoint*)q;return p.services_.operations.current(p.services_.operations.owner,out);}};
    return uc_checkpoint_pump(&pump)?failure_=2:0;
}
int NativeRestartPoint::late(){
    if(!world_||failure_)return failure_?failure_:1;
    if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(entity.transform[i]))return failure_=2;entity.transform[i]+=0.f;}entity.transform[15]=1;}
    return 0;
}
int NativeRestartPoint::close_detached(){
    if(primary_.close()||entity.script.close_detached())return 1;
    if(entity.helper){if(!services_.close_helper||services_.close_helper(services_.operations.owner,entity.helper))return 1;entity.helper=nullptr;}
    uc_entity_subscriptions_close(subscriptions_);subscriptions_=nullptr;
    for(auto&l:entity.identity.listeners)l=nullptr;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;world_=nullptr;return 0;
}
