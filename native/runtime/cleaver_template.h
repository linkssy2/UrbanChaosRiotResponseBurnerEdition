#pragma once
#include "mission_world.h"
#include <cstring>

// F9000/F9060/F9090: bare template owner, not the Molotov blast listener.
class NativeCleaverTemplate {
    NativeMissionWorld *world_=nullptr;
    UCEntityEventTarget identity_{};
    uint32_t flags_=0,secondary_=0;
    int32_t parent_=-1,id_=-1,source_=-1;
    float transform_[16]{};
    const void *definition_=nullptr;
    void *model_=nullptr,*helper_=nullptr;
    UCEntityFreshView view_{{&identity_,&flags_,&parent_},transform_,&source_,&id_,
        &secondary_,&definition_,&model_,&helper_,nullptr,nullptr};
    bool named_=false;
    static int idle(void*p){auto&s=*(NativeCleaverTemplate*)p;return !s.world_||s.model_||s.helper_;}
    static int close(void*p){auto&s=*(NativeCleaverTemplate*)p;
        if(s.named_&&uc_entity_names_remove(s.world_->names(),"TEMPLATE_CLEAVER",s.id_))return 1;
        s.named_=false;s.world_=nullptr;return 0;}
public:
    int construct(NativeMissionWorld &world,int32_t parent){
        if(world_||!uc_entity_event_lookup(&world.registry(),parent))return 1;world_=&world;
        if(world.bind_dynamic({&view_,this,idle,idle,close,nullptr}))return 1;
        auto context=world.context();const void *definition="ThrownWeaponTemplateCleaver";
        context.definition=&definition;*context.registration->creation={0,1,parent};
        if(uc_entity_base_construct_unplaced(&context,&view_)||uc_entity_names_bind(world.names(),"TEMPLATE_CLEAVER",id_))return 1;
        named_=true;return 0;
    }
    const UCEntityFreshView *view()const{return &view_;}
    static const char *property(const char *name){return !std::strcmp(name,"model")?
        "Assets_Characters\\CHA_BurnerBrute\\CHA_BurnerBrute_Cleaver_Skin.AMF":nullptr;}
};
