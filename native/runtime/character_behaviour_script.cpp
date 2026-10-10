#include "character_behaviour_script.h"
#include "character_script_tasks.h"
#include <cmath>
#include <cstring>
static uint32_t word(const uint8_t *p){uint32_t x;std::memcpy(&x,p,4);return x;}
int NativeCharacterBehaviourScript::bind(NativeMissionEntity &e,const UCEntityAuthored &m,
    const NativeMissionServices &s,NativeCharacterScriptTasks &tasks){
    if(entity_||!m.definition||!s.crt_seed)return 1;
    entity_=&e;metadata_=&m;services_=s;tasks_=&tasks;return 0;
}
int NativeCharacterBehaviourScript::request(int32_t type,int32_t index,bool grouped){
    if(!entity_)return 1;if(index==-1)return 0;
    auto &a=entity_->actor;const auto *d=metadata_->definition;
    if(grouped){UCMissionGroup group{};auto status=uc_mission_group(d,(uint32_t)index,&group);
        if(status==UC_MISSION_NOT_FOUND)return 0;
        if(status!=UC_MISSION_READY||group.count>INT32_MAX)return 1;
        if(a.idB0>group.priority)return 0;a.idB0=group.priority;
        uint32_t selected=0;
        if(group.count>1){auto &seed=*services_.crt_seed;seed=seed*214013u+2531011u;
            int32_t draw=(int32_t)((seed>>16)&32767u)%100;
            for(uint32_t i=0;i<group.count;i++){int32_t low=(int32_t)word(group.rows+i*12+4),high=(int32_t)word(group.rows+i*12+8);
                if(low<draw&&high>draw){selected=i;break;}}
        }
        index=(int32_t)word(group.rows+selected*12);
    }else{if(a.idB0>type)return 0;a.idB0=type;}
    tasks_->request_script(index,false);return 0;
}
int NativeCharacterBehaviourScript::run(int32_t state,int32_t index,int32_t *out){
    if(!entity_||!out)return 1;
    auto &a=entity_->actor;const auto *d=metadata_->definition;
    if(state==1){a.wordC4=index>=0&&(uint32_t)index<d->script_count&&d->scripts?d->scripts[index].code.data:nullptr;
        if(!a.wordC4){*out=3;return 0;}a.script_indexB8=(uint32_t)index;state=2;}
    if(a.audioC0||a.handleBC)return 1;
    const auto *p=a.wordC4;uint32_t op=UINT32_MAX;size_t available=0;
    if(p){if(d->script_count&&!d->scripts)return 1;
        for(uint32_t i=0;i<d->script_count;i++){auto b=d->scripts[i].code;auto x=(uintptr_t)p,y=(uintptr_t)b.data;
            if(b.data&&x>=y&&x-y<=b.size){available=b.size-(x-y);if(available>=4)break;}}
        if(available<4)return 1;op=word(p);}
    if(op==21){if(available<8)return 1;float wait;std::memcpy(&wait,p+4,4);if(!std::isfinite(wait))return 1;
        a.wordC4=p+8;tasks_->wait(wait);*out=state;return 0;}
    if(op!=17&&op!=22&&op!=52&&op!=53&&op!=UINT32_MAX)return 1;
    UCMissionEvent event{};if(uc_mission_event_bind(&entity_->view,&event))return 1;
    UCMissionCommandContext c{&event,&a.handleBC,nullptr,0,services_.flags,services_.flag_count,
        services_.delta,services_.owner,nullptr,services_.ensure};
    //327D0 falls back to217BB0 for immediate removal; reuse the real world's
    //live-target/deferred-target action owner, with the same command lifetime.
    c.entity_action=services_.entity_action;
    c.logic=services_.logic;
    int32_t decoded=0;int result=uc_mission_command_decode_descriptor(&c,d,services_.flag_limit,&decoded);
    if(result)return result;if(decoded==-1){a.idB0=-1;state=3;}*out=state;return 0;
}
