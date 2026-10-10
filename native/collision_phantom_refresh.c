#include "collision_phantom_refresh.h"
static UCCollisionStatus valid(const UCCollisionPhantomPairBody *b) {
    if(!b||!b->identity||!b->owner||b->shape_present>1||
       (b->property_count&&!b->properties))return UC_COLLISION_ARGUMENT;
    if(b->type!=1&&b->type!=2)return UC_COLLISION_UNSUPPORTED;
    return b->property_count>4096?UC_COLLISION_LIMIT:UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_refresh(UCCollisionPhantomPairFilter filter,
    const UCCollisionPhantomPairBody *selected,const UCCollisionPhantomPairBody *other,
    void *context,UCCollisionPhantomContains contains,UCCollisionPhantomOverlap overlap) {
    UCCollisionStatus s;int present,accepted;
    if(!contains||!overlap)return UC_COLLISION_ARGUMENT;
    if(filter!=UC_COLLISION_PHANTOM_PAIRS_DEFAULT_FILTER&&
       filter!=UC_COLLISION_PHANTOM_PAIRS_GAME_FILTER)return UC_COLLISION_UNSUPPORTED;
    if((s=valid(selected))!=UC_COLLISION_OK||(s=valid(other))!=UC_COLLISION_OK)return s;
    if(selected->type!=2)return UC_COLLISION_UNSUPPORTED;
    present=contains(context,selected->owner,other->identity)!=0;
    s=uc_collision_phantom_pairs_filter(filter,selected,other,&accepted);
    if(s!=UC_COLLISION_OK)return s;
    if(present!=accepted) {
        overlap(context,selected->owner,other->identity,accepted);
        if(other->type==2)overlap(context,other->owner,selected->identity,accepted);
    }
    return UC_COLLISION_OK;
}
