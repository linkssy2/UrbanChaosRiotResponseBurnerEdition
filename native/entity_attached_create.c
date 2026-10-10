#include "entity_attached_create.h"
int uc_entity_attached_create(UCEntityCreation *creation,const int32_t *parent,
    UCEntityProperties *properties,uint32_t mode,const char *name,void *owner,
    int (*factory)(void *,const char *,UCEntityProperties *,int32_t *),int32_t *out){
    if(!creation||!parent||!properties||!name||!factory||!out)return 1;
    properties->placement_mode=mode;
    int32_t id=*parent;
    creation->relative=1;creation->priority=1;creation->parent_id=id;
    if(factory(owner,name,properties,&id))return 2;
    *out=id;return 0;
}
