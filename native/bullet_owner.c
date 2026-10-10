#include "bullet_owner.h"
int uc_bullet_owner_submit(const UCBulletOwner *o,const UCWeaponBulletRequest *r,
        uint32_t owner_id,uint8_t *submitted) {
    UCEntityEventTarget *target;
    int32_t id;
    if(!o||!r||!submitted||!o->registry||!o->cached_id||!o->creation||!o->preflight||
        !o->create||!o->initialize||!o->preflight(o->context,r,owner_id))return 1;
    target=uc_entity_event_lookup(o->registry,*o->cached_id);
    if(!target){
        o->creation->relative=0;o->creation->priority=1;o->creation->parent_id=-1;
        if(o->create(o->context,&id))return 2;
        *o->cached_id=id;
    }
    target=uc_entity_event_lookup(o->registry,*o->cached_id);
    if(target&&o->initialize(o->context,target,r,owner_id))return 2;
    *submitted=(uint8_t)(target!=0);return 0;
}
