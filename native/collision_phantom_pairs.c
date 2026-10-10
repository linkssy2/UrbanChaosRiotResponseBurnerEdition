#include "collision_phantom_pairs.h"
static UCCollisionStatus valid(const UCCollisionPhantomPairBody *b) {
    if(!b||!b->identity||!b->owner||b->shape_present>1||
       (b->property_count&&!b->properties))return UC_COLLISION_ARGUMENT;
    if(b->type!=1&&b->type!=2)return UC_COLLISION_UNSUPPORTED;
    return b->property_count>4096?UC_COLLISION_LIMIT:UC_COLLISION_OK;
}
static UCCollisionStatus pair(UCCollisionPhantomPairFilter f,
    const UCCollisionPhantomPairBody *a,const UCCollisionPhantomPairBody *b) {
    UCCollisionStatus s;
    if(f!=UC_COLLISION_PHANTOM_PAIRS_DEFAULT_FILTER&&f!=UC_COLLISION_PHANTOM_PAIRS_GAME_FILTER)return UC_COLLISION_UNSUPPORTED;
    if((s=valid(a))!=UC_COLLISION_OK||(s=valid(b))!=UC_COLLISION_OK)return s;
    return a->type==2||b->type==2?UC_COLLISION_OK:UC_COLLISION_UNSUPPORTED;
}
static const UCCollisionPairProperty *property(const UCCollisionPhantomPairBody *b,uint32_t key) {
    for(uint32_t i=0;i<b->property_count;i++)if(b->properties[i].key==key)return b->properties+i;
    return NULL;
}
static uint32_t value(const UCCollisionPhantomPairBody *b,uint32_t key) {
    const UCCollisionPairProperty *p=property(b,key);return p?(uint32_t)p->value:0;
}
static int restriction(const UCCollisionPhantomPairBody *a,const UCCollisionPhantomPairBody *b) {
    const UCCollisionPairProperty *p=property(a,0x18);
    return p&&((uint16_t)p->value==0xffff||(uint16_t)p->value==(b->filter_info&0xfffff));
}
static int phantom_gate(const UCCollisionPhantomPairBody *a,const UCCollisionPhantomPairBody *b) {
    if(a->type!=2||value(a,0x29)!=1)return 1;
    if(b->type==2)return value(b,0x29)==1;
    uint32_t kind=value(b,1);
    return !property(b,0x19)&&kind!=4&&kind!=21&&
        !(value(a,0x2a)==1&&property(b,0x1a))&&!(kind==1&&property(b,0x1b));
}
UCCollisionStatus uc_collision_phantom_pairs_filter(UCCollisionPhantomPairFilter f,
    const UCCollisionPhantomPairBody *a,const UCCollisionPhantomPairBody *b,int *accepted) {
    UCCollisionStatus s;int result=0;
    if(!accepted)return UC_COLLISION_ARGUMENT;
    if((s=pair(f,a,b))!=UC_COLLISION_OK)return s;
    if(f==UC_COLLISION_PHANTOM_PAIRS_DEFAULT_FILTER){result=1;goto done;}
    if(restriction(a,b)||restriction(b,a)||!phantom_gate(a,b)||!phantom_gate(b,a))goto done;
    if(!a->shape_present||!b->shape_present)goto done;
    if(a->shape_type==24||b->shape_type==24){result=1;goto done;}
    uint32_t x=a->filter_info,y=b->filter_info;
    if(!x||!y||!((x^y)&0xfffff))goto done;
    if(!((x>>26)&(y>>20))&&!((y>>26)&(x>>20))&&(x>>26)&&(y>>26))goto done;
    x=a->shape_user_data;y=b->shape_user_data;
    result=!x||!y||(((y>>6)&x)&63)||(((x>>6)&y)&63);
done:
    *accepted=result;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_pairs_dispatch(UCCollisionPhantomPairFilter f,
    const UCCollisionPhantomPairBody *a,const UCCollisionPhantomPairBody *b,int removing,
    void *context,UCCollisionPhantomOverlap overlap) {
    UCCollisionStatus s;int accepted;
    if(!overlap||(removing!=0&&removing!=1))return UC_COLLISION_ARGUMENT;
    if((s=pair(f,a,b))!=UC_COLLISION_OK)return s;
    if(!removing) {
        s=uc_collision_phantom_pairs_filter(f,a,b,&accepted);
        if(s!=UC_COLLISION_OK||!accepted)return s;
    }
    if(a->type==2)overlap(context,a->owner,b->identity,!removing);
    if(b->type==2)overlap(context,b->owner,a->identity,!removing);
    return UC_COLLISION_OK;
}
