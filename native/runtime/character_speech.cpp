#include "character_speech.h"
#include <cstring>

static bool valid(const NativeCharacterSpeechServices &s){
    return s.ready&&s.playing&&s.request&&s.status&&s.stop&&s.speaker_owner&&s.cancel;
}
int native_character_speech_update(NativeCharacterSpeechState &a,int32_t sender,
    const UCLevelEntitySlots &slots,const UCEntityEventRegistry &registry,
    const NativeCharacterSpeechServices &s,bool *remove){
    if(!remove||!valid(s)||!a.text||a.targets.size()>4096)return 1;
    *remove=false;
    if(!a.sent){
        bool absent=true;
        for(uint32_t source:a.targets){
            int32_t id=-1;if(uc_level_entity_lookup(&slots,source,&id))return 2;
            if(!uc_entity_event_lookup(&registry,id))continue;
            absent=false;a.selected=id;
            uint8_t ready=0;
            if(!a.force&&s.ready(s.owner,id,&ready))return 2;
            if(a.force||ready){
                if(s.request(s.owner,id,sender,a.text,a.look,a.force))return 2;
                a.sent=1;return 0;
            }
        }
        *remove=absent;return 0;
    }
    if(a.selected==-1||!uc_entity_event_lookup(&registry,a.selected)){*remove=true;return 0;}
    uint8_t playing=0;if(s.playing(s.owner,a.selected,&playing))return 2;
    if(!playing){*remove=true;return 0;}
    uint32_t status=0;if(s.status(s.owner,a.selected,&status))return 2;
    if(status&1){if(s.stop(s.owner,a.selected))return 2;*remove=true;}
    return 0;
}
int native_character_speech_cancel(const NativeCharacterSpeechState &a,int32_t sender,
    const UCEntityEventRegistry &registry,const NativeCharacterSpeechServices &s){
    if(!valid(s))return 1;
    if(a.selected!=-1&&uc_entity_event_lookup(&registry,a.selected)){
        int32_t owner=-1;if(s.speaker_owner(s.owner,a.selected,&owner))return 2;
        if(owner==sender&&s.cancel(s.owner,a.selected))return 2;
    }
    return 0;
}
int NativeCharacterSpeech::construct(NativeMissionWorld &w,uint32_t source,const NativeCharacterSpeechServices &s){
    if(world_||source>=w.source_count()||!valid(s))return 1;
    const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"CharacterSpeech")||m.asset_present||m.condition_count||d->script_count||
       d->script_records.size!=8||std::memcmp(d->script_records.data,"\0\0\0\0\0\0\0\0",8)||*w.active_descriptor()!=&m)return 1;
    const uint8_t *cursor=*w.property_cursor(source);const char *text="";uint32_t look=0,force=0;int32_t count=0;int found=0;
    if(uc_entity_stream_get(d,&cursor,11,&text,&found)||!found||
       uc_entity_stream_get(d,&cursor,3,&look,&found)||!found||
       uc_entity_stream_get(d,&cursor,1,&count,&found)||!found||count>4096)return 1;
    std::vector<uint32_t> targets(count>0?size_t(count):0);
    for(auto&t:targets)if(uc_entity_stream_get(d,&cursor,1,&t,&found)||!found)return 1;
    if(uc_entity_stream_get(d,&cursor,3,&force,&found)||!found||cursor!=d->properties.data+d->properties.size)return 1;
    world_=&w;services_=s;
    if(uc_entity_base_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    state_.targets=std::move(targets);state_.text=text;state_.look=uint8_t((look&255)!=0);state_.force=uint8_t((force&255)!=0);
    entity.flags|=0x100;*w.property_cursor(source)=cursor;return 0;
}
int NativeCharacterSpeech::update(){
    if(!world_||failure_)return failure_?failure_:1;
    bool retire=false;
    if(native_character_speech_update(state_,entity.id,world_->slots(),world_->registry(),services_,&retire))return failure_=2;
    if(retire&&remove())return failure_=2;return 0;
}
int NativeCharacterSpeech::remove(){
    if(!world_||entity.model||entity.helper||entity.aux)return 1;
    for(auto*l:entity.identity.listeners)if(l)return 1;
    entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return 2;
    if(entity.flags&0x1000){if(entity.source<0||uint32_t(entity.source)>=world_->source_count())return 2;
        auto*f=world_->slot(uint32_t(entity.source)).flags;*f=(*f&~15u)|16u;}
    return 0;
}
int NativeCharacterSpeech::close_detached(){
    if(!world_)return 0;
    if(native_character_speech_cancel(state_,entity.id,world_->registry(),services_))return 1;
    state_.targets.clear();world_=nullptr;return 0;
}
