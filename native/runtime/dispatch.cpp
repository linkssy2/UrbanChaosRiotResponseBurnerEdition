#include "dispatch.h"
#include "subtitles.h"
#include "player_mission.h"
#include <algorithm>
#include <cstring>
#include <string>
int NativeDispatch::construct(NativeMissionWorld&w,uint32_t source,NativeCharacterDialogueAudio&a,NativeDialogueText&t,UCGameRandom&r,NativeSubtitles&s,NativeMissionPlayer&p,int32_t&active,const uint32_t&world_flags){
    if(world_||source>=w.source_count())return 1;
    const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"Dispatch")||m.asset_present||m.condition_count||d->script_count||*w.active_descriptor()!=&m)return 1;
    const uint8_t*cursor=*w.property_cursor(source);int32_t count=0;int found=0;
    if(uc_entity_stream_get(d,&cursor,1,&count,&found)||!found||count<0||count>4096)return 1;
    std::vector<uint32_t> flags(count);
    for(auto&f:flags)if(uc_entity_stream_get(d,&cursor,1,&f,&found)||!found||f>=w.script_services().flag_count)return 1;
    const char*line="";if(uc_entity_stream_get(d,&cursor,11,&line,&found)||!found||cursor!=d->properties.data+d->properties.size)return 1;
    world_=&w;audio_=&a;text_=&t;random_=&r;subtitles_=&s;player_=&p;active_=&active;world_flags_=&world_flags;
    if(uc_entity_base_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return 2;
    flags_=std::move(flags);line_=line;entity.flags|=0x100;*w.property_cursor(source)=cursor;
    return uc_entity_names_bind(w.names(),"DISPATCH",entity.id);
}
int NativeDispatch::update(){
    if(!world_||removed_)return 1;
    const float delta=*world_->script_services().delta;
    if(dialogue_)dialogue_->step(delta);
    if(player_->player()->inventory.health<=0||(*world_flags_&0x400))return remove();
    if(!uc_entity_event_lookup(&world_->registry(),player_->entity.id))return 0;
    //D50F0 places a radio-only entity at the current camera, before request.
    std::memcpy(entity.transform+12,player_->player()->camera_world+12,12);
    if(!sent_){
        if(*active_!=-1)return 0;
        const int32_t*ids=nullptr;uint32_t count=0;
        if(uc_entity_names_view(world_->names(),"DISPATCH",&ids,&count))return 2;
        if(!count||ids[0]!=entity.id)return 0;
        NativeCharacterDialogueServices services{};services.owner=this;
        services.position=[](void*q,bool,float*out){auto&s=*(NativeDispatch*)q;std::memcpy(out,s.entity.transform+12,12);return 0;};
        services.subtitle=[](void*q,const NativeDialogueLine&line,int32_t id,int32_t*out){auto&s=*(NativeDispatch*)q;
            std::string name=s.line_;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(c>='A'&&c<='Z'?c+32:c);});
            int kind=name.find("-cop")!=std::string::npos?2:name.find("-medic")!=std::string::npos?4:name.find("-fireman")!=std::string::npos?3:0;
            return s.subtitles_->add_dispatch(line,*s.dialogue_,id,s.entity.transform+12,kind,*s.world_,*s.player_,out);};
        //21A600 sends facial data to the actor event listeners. Dispatch has no
        //listeners/model (20BDC0): the original event is successfully unhandled.
        services.face=[](void*q,const NativeDialogueVariant&){auto&s=*(NativeDispatch*)q;for(auto*l:s.entity.identity.listeners)if(l)return 1;return 0;};
        dialogue_=std::make_unique<NativeCharacterDialogue>(*audio_,*text_,*random_,entity.id,services);
        if(!dialogue_->request(line_,-1))return 2;
        sent_=true;*active_=entity.id;return 0;
    }
    if(!dialogue_->playing()){
        for(uint32_t flag:flags_)world_->script_services().flags[flag]=1;
        completed_=true;return remove();
    }
    return 0;
}
int NativeDispatch::remove(){
    if(!world_)return 1;if(removed_)return 0;
    if(*active_==entity.id)*active_=-1;
    subtitles_->remove_actor(entity.id);dialogue_.reset();flags_.clear();
    if(uc_entity_names_remove(world_->names(),"DISPATCH",entity.id))return 2;
    entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return 2;
    if(entity.flags&0x1000){if(entity.source<0||uint32_t(entity.source)>=world_->source_count())return 2;
        auto*f=world_->slot(uint32_t(entity.source)).flags;*f=(*f&~15u)|16u;}
    removed_=true;return 0;
}
int NativeDispatch::close_detached(){
    if(!world_)return 0;
    if(*active_==entity.id)*active_=-1;
    subtitles_->remove_actor(entity.id);dialogue_.reset();flags_.clear();
    if(uc_entity_names_remove(world_->names(),"DISPATCH",entity.id))return 2;
    world_=nullptr;return 0;
}
