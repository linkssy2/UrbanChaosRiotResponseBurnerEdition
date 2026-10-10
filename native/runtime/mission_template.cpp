#include "mission_template.h"
#include <cstring>
int NativeMolotovTemplate::construct(NativeMissionWorld&w,uint32_t source,void*owner,
 int(*receive)(void*,const void*,uint32_t*)){
 if(source>=w.source_count()||!receive||object_.view)return 1;
 const auto&d=w.metadata(source);
 if(!d.definition||!d.definition->class_name||
    std::strcmp(d.definition->class_name,"ThrownWeaponTemplateMolotov")||
    d.asset_present||d.definition->properties.size||d.condition_count)return 1;
 if(w.bind(source,{&view_,this,idle,idle,close,nullptr}))return 1;
 static const uint32_t code=210; //actual initialized4FA014
 return uc_throwable_template_construct(&object_,&w.context(),&view_,
     w.active_descriptor(),w.listeners(),w.names(),&code,owner,receive);
}
int NativeMolotovTemplate::construct_dynamic(NativeMissionWorld&w,int32_t parent,void*owner,
 int(*receive)(void*,const void*,uint32_t*)){
 if(!receive||object_.view||!uc_entity_event_lookup(&w.registry(),parent))return 1;
 if(w.bind_dynamic({&view_,this,idle,idle,close,nullptr}))return 1;
 //207CD0 publishes creation context; the nested factory uses its own empty
 //descriptor, retaining the enclosing authored descriptor for its caller.
 auto context=w.context();const void*definition="ThrownWeaponTemplateMolotov";
 context.definition=&definition;*context.registration->creation={0,1,parent};
 static const uint32_t code=210;
 return uc_throwable_template_construct(&object_,&context,&view_,nullptr,
     w.listeners(),w.names(),&code,owner,receive);
}
