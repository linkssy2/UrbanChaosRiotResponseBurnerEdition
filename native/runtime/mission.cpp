#include "mission.h"
#include <cstring>
int NativeMissionController::attach_actor(const UCEntityFreshView &v,
    const UCEntityAuthored &m,const NativeMissionServices &s){
    if(view_||!m.definition||!v.actor||v.actor->scriptC8||v.actor->handleBC||
       v.actor->audioC0||!v.aux84||!uc_entity_fresh_aux_empty(*v.aux84)||
       !s.crt_seed||!s.delta||(m.condition_count&&!m.conditions))return 1;
    if(uc_mission_event_bind(&v,&event_))return 1;
    view_=&v;metadata_=&m;services_=s;
    schedule_={&event_,&v.actor->flagCD,&v.actor->flagCE,m.definition,m.conditions,
        m.condition_count,s.flags,s.flag_count,s.crt_seed,this,
        uc_mission_command_destroy,decode,yield};
    return 0;
}
int NativeMissionController::attach_controller(const UCEntityFreshView &v,
    const UCEntityAuthored &m,const uint8_t **cursor,const NativeMissionServices &s){
    if(!m.definition||!m.definition->class_name||
       std::strcmp(m.definition->class_name,"ScriptController")||m.asset_present||
       !v.model74||*v.model74||!cursor)return 1;
    int r=attach_actor(v,m,s);if(r)return r;
    uint32_t pair[2]={0,UINT32_MAX};int found=0;
    if(uc_entity_stream_get(m.definition,cursor,5,pair,&found))return failure_=2;
    property_type_=found?pair[0]:0;group_=found?pair[1]:UINT32_MAX;
    controller_=true;
    if(primary_.create([](void*p,NativeCoroutine&){auto &m=*static_cast<NativeMissionController*>(p);
        UCMissionController c{&m.event_,nullptr,0,m.services_.crt_seed,&m.group_,
            m.view_->model74,&m,nullptr,nullptr,uc_mission_command_destroy};
        if(uc_mission_controller_update_descriptor(&c,m.metadata_->definition))m.failure_=2;
    },this))return failure_=2;
    return 0;
}
int NativeMissionController::yield(void *p){return static_cast<NativeMissionController*>(p)->secondary_.yield();}
int NativeMissionController::attach_wave(const UCEntityFreshView &v,
    const UCEntityAuthored &m,const uint8_t **cursor,const NativeMissionServices &s){
    if(!m.definition||!m.definition->class_name||
       (std::strcmp(m.definition->class_name,"WaveController")&&
        std::strcmp(m.definition->class_name,"FriendlyWaveController"))||
       m.asset_present||!cursor||!s.slots||!s.character_query||!s.character_status)return 1;
    const uint8_t *p=*cursor;uint32_t start[2]={0,UINT32_MAX},end[2]={0,UINT32_MAX},excluded=0;int found=0;
    if(uc_entity_stream_get(m.definition,&p,5,start,&found)||!found||
       uc_entity_stream_get(m.definition,&p,5,end,&found)||!found||
       uc_entity_stream_get(m.definition,&p,1,&excluded,&found)||!found||excluded)return 1;
    int r=attach_actor(v,m,s);if(r)return r;
    *cursor=p;wave_=controller_=true;property_type_=start[0];group_=start[1];end_group_=end[1];
    *v.target.flags|=0x100;
    if(primary_.create([](void*p,NativeCoroutine&fiber){auto&m=*static_cast<NativeMissionController*>(p);
        *m.view_->target.flags|=0x100;
        // Both retail wave vtables use2C3360: readiness is unconditionally true.
        uint8_t accepted=0;
        if(m.event(m.group_,1,&accepted)){m.failure_=2;return;}
        while(!m.spawn_finished_)if(fiber.yield()){m.failure_=2;return;}
        for(;;){uint8_t done=0;
            if(uc_wave_members_complete_slots(&m.members_,m.services_.slots,
                m.services_.owner,m.services_.character_status,&done)){m.failure_=2;return;}
            if(done)break;
            if(fiber.yield()){m.failure_=2;return;}
        }
        if(m.event(m.end_group_,1,&accepted))m.failure_=2;
    },this))return failure_=2;
    return 0;
}
int NativeMissionController::ensure(void *p,uint32_t index){auto &m=*static_cast<NativeMissionController*>(p);return m.services_.ensure?m.services_.ensure(m.services_.owner,index):1;}
int NativeMissionController::decode(void *p,int32_t *result){
    auto &m=*static_cast<NativeMissionController*>(p);
    UCMissionCommandContext c{&m.event_,&m.view_->actor->handleBC,nullptr,0,
        m.services_.flags,m.services_.flag_count,m.services_.delta,&m,yield,
        m.services_.ensure?ensure:nullptr};
    if(m.services_.slots)c.ensure_id=[](void*p,uint32_t i,int32_t*id){auto&s=static_cast<NativeMissionController*>(p)->services_;return uc_level_entity_ensure(s.slots,i,id);};
    if(m.services_.registry)c.alive=[](void*p,int32_t id,uint8_t*live){auto&s=static_cast<NativeMissionController*>(p)->services_;*live=uc_entity_event_lookup(s.registry,id)!=nullptr;return 0;};
    c.logic=m.services_.logic;
    if(m.services_.operate&&m.view_->source_index&&m.view_->id&&m.view_->transform)c.operate=[](void*p,uint32_t target){auto&m=*static_cast<NativeMissionController*>(p);return m.services_.operate(m.services_.owner,(uint32_t)*m.view_->source_index,*m.view_->id,m.view_->transform+12,target);};
    if(m.services_.entity_action)c.entity_action=[](void*p,uint32_t i,uint32_t mode){auto&s=static_cast<NativeMissionController*>(p)->services_;return s.entity_action(s.owner,i,mode);};
    /* Route by opcode only after the concrete decoder validates the cursor.
     * Rejection may be malformed supported input; never fall through to an
     * unrelated handler in that case. */
    const uint8_t *cursor=*m.event_.cursor;
    uint32_t op=UINT32_MAX;size_t available=0;
    if(cursor){bool inside=false;
        for(uint32_t i=0;i<m.metadata_->definition->script_count;i++){
            auto b=m.metadata_->definition->scripts[i].code;
            uintptr_t a=(uintptr_t)cursor,z=(uintptr_t)b.data;
            if(b.data&&a>=z&&a-z<=b.size&&b.size-(a-z)>=4){inside=true;available=b.size-(a-z);break;}
        }
        if(!inside)return 1;
        std::memcpy(&op,cursor,4);
    }
    if(m.wave_&&!m.spawn_finished_){
        if(m.view_->actor->handleBC||*m.event_.command)return 1;
        if(op==UINT32_MAX){*m.event_.secondary_flags|=0x8000;m.spawn_finished_=1;*result=-1;return 0;}
        if(op==22||op==61){
            if(available<8)return 1;
            int32_t source;std::memcpy(&source,cursor+4,4);
            if(source!=-1&&(m.members_.count<0||m.members_.count>=32))return 1;
            *m.event_.cursor=cursor+8;
            if(source!=-1){int32_t id=-1;
                if(uc_level_entity_ensure(m.services_.slots,(uint32_t)source,&id))return 2;
                int32_t i=m.members_.count++;m.members_.source[i]=source;m.members_.active[i]=1;
                if(m.services_.character_query(m.services_.owner,id))return 2;
            }
            *result=-2;return 0;
        }
        // No base fallback during spawning: in particular opcode5 is not the
        // base switch-script operation here. Other wave handlers need decoding.
        if(op!=6&&op!=21&&op!=10&&op!=11)return 1;
    }
    if(op==UINT32_MAX||op==5||op==6||op==10||op==11||op==16||op==17||op==21||op==22||op==23||op==43||op==52||op==53||op==57||op==61)
        return uc_mission_command_decode_descriptor(&c,m.metadata_->definition,m.services_.flag_limit,result);
    if(!m.services_.decode_extra||*m.event_.command)return 1;
    int status=m.services_.decode_extra(m.services_.owner,&m.event_,m.metadata_->definition,result);
    return *m.event_.command?2:status;
}
int NativeMissionController::update(){
    if(!view_||failure_)return failure_?failure_:1;
    if(controller_&&(primary_.resume()<0||failure_))return failure_=2;
    auto &a=*view_->actor;
    if(a.flagCC||a.flagCD||a.flagCE){
        if(!a.scriptC8){
            if(secondary_.create([](void*p,NativeCoroutine&){auto&m=*static_cast<NativeMissionController*>(p);
                if(uc_mission_schedule_run(&m.schedule_))m.failure_=2;},this))return failure_=2;
            a.scriptC8=&secondary_;
        }
        if(a.scriptC8!=&secondary_||secondary_.resume()<0||failure_)return failure_=2;
    }
    return uc_entity_fresh_aux_clear(*view_->aux84)?(failure_=2):0;
}
int NativeMissionController::event(uint32_t request,uint8_t grouped,uint8_t *accepted){
    if(!view_||failure_)return failure_?failure_:1;
    return uc_mission_event_start_descriptor(&event_,metadata_->definition,request,
        grouped,services_.crt_seed,uc_mission_command_destroy,this,accepted);
}
int NativeMissionController::close_detached(){
    if(!view_)return 0;
    if(view_->actor->handleBC)return 1;
    if(*event_.command){uc_mission_command_destroy(nullptr,*event_.command);*event_.command=nullptr;}
    if(primary_.close()||secondary_.close())return 1;
    view_->actor->scriptC8=nullptr;view_=nullptr;metadata_=nullptr;return 0;
}
int native_mission_create(const UCEntityFreshContext &c,
    const UCEntityAuthored **active,const UCEntityAuthored &m,
    const uint8_t **cursor,const UCLevelEntitySlot &slot,const NativeMissionServices &s,
    void *owner,NativeMissionEntity *(*allocate)(void *),NativeMissionEntity **out){
    if(!active||!cursor||!slot.flags||!slot.id||!out||!allocate||
       !c.registration||!c.registration->creation||!m.definition||
       !m.definition->class_name||(std::strcmp(m.definition->class_name,"ScriptController")&&
       std::strcmp(m.definition->class_name,"WaveController")&&std::strcmp(m.definition->class_name,"FriendlyWaveController"))||
       m.asset_present)return 1;
    if(!(*slot.flags&2)||(*slot.flags&0x18))return 0;
    auto &creation=*c.registration->creation;
    creation.relative=0;creation.priority=1;creation.parent_id=-1;
    *active=&m;
    NativeMissionEntity *entity=allocate(owner);*out=entity;
    int result=0;
    if(entity){
        result=uc_entity_fresh_construct_authored(&c,&entity->view,active);
        if(!result)result=!std::strcmp(m.definition->class_name,"ScriptController")?
            entity->script.attach_controller(entity->view,m,cursor,s):entity->script.attach_wave(entity->view,m,cursor,s);
    }
    *active=nullptr;
    if(result)return 2;
    if(entity){
        if(!(*slot.flags&0x20))entity->flags|=0x10000000;
        *slot.id=entity->id;*slot.flags=(*slot.flags&0xffffffe8u)|8;
    }
    *cursor=m.definition->properties.data;
    return 0;
}
