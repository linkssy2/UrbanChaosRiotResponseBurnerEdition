#include "scene_camera_create.h"
#include <cstring>

int native_scene_camera_create(const NativeSceneCameraOwners &owners,
    UCEntityProperties **active,void *allocation_owner,
    NativeSceneCamera *(*allocate)(void *),const int32_t *parent,
    UCEntityProperties &properties,const char *class_name,
    NativeSceneCamera **object,int32_t *id) {
    if(!active||!allocate||!parent||!object||!id||!class_name||
       std::strcmp(class_name,"Camera")||!owners.entities||
       !owners.entities->registration||!owners.entities->registration->creation||
       owners.entities->definition!=&properties.definition)return 1;
    struct Call {
        const NativeSceneCameraOwners &owners;UCEntityProperties **active;
        void *owner;NativeSceneCamera *(*allocate)(void *);NativeSceneCamera **out;
    } call{owners,active,allocation_owner,allocate,object};
    return uc_entity_attached_create(owners.entities->registration->creation,
        parent,&properties,1,class_name,&call,
        [](void *p,const char *name,UCEntityProperties *descriptor,int32_t *result){
            auto &c=*static_cast<Call*>(p);
            descriptor->definition=name;*c.active=descriptor;
            auto *camera=c.allocate(c.owner);*c.out=camera;
            int status=camera?camera->construct(c.owners,*descriptor):2;
            *c.active=nullptr;
            if(status)return 2;
            *result=camera->entity.id;return 0;
        },id);
}
