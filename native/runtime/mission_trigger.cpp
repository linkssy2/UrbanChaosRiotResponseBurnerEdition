#include "mission_trigger.h"
int NativeMissionTrigger::base(void *p){auto&m=*static_cast<NativeMissionTrigger*>(p);
    return uc_entity_fresh_construct_authored(m.context_,m.view_,m.active_);
}
int NativeMissionTrigger::suffix(void *p){return static_cast<NativeMissionTrigger*>(p)->mission_.update();}
int NativeMissionTrigger::construct(const UCEntityFreshContext &context,const UCEntityAuthored **active,const UCEntityFreshView &view,
    UCLevelTrigger &trigger,int32_t source_index,UCLevelEntitySlot &slot,
    UCMissionCondition *conditions,uint32_t condition_count,const UCMissionTriggerWorld &world,
    const NativeMissionServices &services){
    if(view_||!active||!context.registration||!context.registration->creation||
       source_index<0||!slot.id||!slot.flags||!(*slot.flags&0x40)||
       (*slot.flags&0x18)||!(trigger.flags&32)||trigger.shape!=3||
       (trigger.kind!=17&&trigger.kind!=37)||!view.id||!services.crt_seed||!services.delta)return 1;
    context_=&context;view_=&view;trigger_=&trigger;world_=world;seed_=services.crt_seed;
    metadata_={&trigger.script,source_index,0,conditions,condition_count};active_=active;
    auto &creation=*context.registration->creation;
    creation.relative=0;creation.priority=1;creation.parent_id=-1;
    *active_=&metadata_;
    property_cursor_=trigger.script.properties.data;
    int r=uc_mission_trigger_construct(&actor_,&view,&trigger,source_index,
        trigger.script.properties.data?1u:0u,&property_cursor_,conditions,condition_count,
        &context.memory,this,base);
    *active_=nullptr;
    if(r)return failure_=2;
    /*20A1D0 sees the actual NULL asset on this custom metadata, then211820
     * publishes the acquired ID and activated slot flags before cursor reset. */
    *slot.id=*view.id;*slot.flags=(*slot.flags&~0x17u)|8;
    property_cursor_=trigger.script.properties.data;
    if(mission_.attach_actor(view,metadata_,services))return failure_=2;
    return 0;
}
int NativeMissionTrigger::update(){
    if(!view_||failure_)return failure_?failure_:1;
    int r=uc_mission_trigger_update(&actor_,trigger_,&world_,&mission_.fields(),seed_,
        uc_mission_command_destroy,this,suffix);
    if(r)failure_=2;return r;
}
int NativeMissionTrigger::close_detached(){
    if(!view_)return 0;
    if(mission_.close_detached())return 1;
    if(actor_.rows)context_->memory.release(context_->memory.owner,actor_.rows);
    actor_.rows=nullptr;actor_.count=0;view_=nullptr;context_=nullptr;trigger_=nullptr;return 0;
}
