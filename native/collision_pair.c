#include "collision_pair.h"

static UCCollisionStatus valid(const UCCollisionPairBody *b) {
    if(!b||!b->identity||!b->rigid_owner||b->impulse_disabled>255||b->shape_present>1||
       (b->property_count&&!b->properties)||(b->agent_count&&!b->agents))return UC_COLLISION_ARGUMENT;
    if(b->collidable_type!=1)return UC_COLLISION_UNSUPPORTED;
    if(b->property_count>4096||b->agent_count>16383)return UC_COLLISION_LIMIT;
    return UC_COLLISION_OK;
}
static const UCCollisionPairProperty *property(const UCCollisionPairBody *b,uint32_t key) {
    for(uint32_t i=0;i<b->property_count;i++)if(b->properties[i].key==key)return b->properties+i;
    return NULL;
}
static uint32_t value(const UCCollisionPairBody *b,uint32_t key) {
    const UCCollisionPairProperty *p=property(b,key);return p?(uint32_t)p->value:0;
}
static int restriction(const UCCollisionPairBody *a,const UCCollisionPairBody *b) {
    const UCCollisionPairProperty *p=property(a,0x18);
    return p&&((uint16_t)p->value==0xffff||(uint16_t)p->value==(b->filter_info&0xfffff));
}
/*2A8AF0 returns0/1, or -1 at its unresolved script lookup. */
static int special(const UCCollisionPairBody *a,const UCCollisionPairBody *b) {
    if(value(a,1)!=0x15)return 1;
    uint32_t type=value(b,1);
    if(property(b,0x19)||(type==1&&!property(b,0x1b)))return 0;
    return type==4?-1:1;
}
UCCollisionStatus uc_collision_pair_filter(UCCollisionPairFilter filter,
    const UCCollisionPairBody *a,const UCCollisionPairBody *b,int *accepted) {
    UCCollisionStatus s;int result=0;
    if(!accepted)return UC_COLLISION_ARGUMENT;
    if(filter!=UC_COLLISION_PAIR_GAME_FILTER)return UC_COLLISION_UNSUPPORTED;
    if((s=valid(a))!=UC_COLLISION_OK||(s=valid(b))!=UC_COLLISION_OK)return s;
    if(restriction(a,b)||restriction(b,a))goto done;
    int v=special(a,b);if(v<0)return UC_COLLISION_UNSUPPORTED;if(!v)goto done;
    v=special(b,a);if(v<0)return UC_COLLISION_UNSUPPORTED;if(!v)goto done;
    uint32_t x=value(a,1),y=value(b,1);
    if(x==4&&y==4&&value(a,0x2d)==value(b,0x2d)) {
        uint32_t count=value(a,4),key=value(b,3);
        if(count<0x80000000u) {
            if(count>4096)return UC_COLLISION_UNSUPPORTED;
            for(uint32_t i=0;i<count;i++)if(value(a,i+5)==key)goto done;
        }
    }
    if(a->impulse_disabled&&b->impulse_disabled&&!x&&!y)goto done;
    if(!a->shape_present||!b->shape_present)goto done;
    if(a->shape_type==24||b->shape_type==24){result=1;goto done;}
    x=a->filter_info;y=b->filter_info;
    if(!x||!y||!((x^y)&0xfffff))goto done;
    if(!((x>>26)&(y>>20))&&!((y>>26)&(x>>20))&&(x>>26)&&(y>>26))goto done;
    x=a->shape_user_data;y=b->shape_user_data;
    result=!x||!y||(((y>>6)&x)&63)||(((x>>6)&y)&63);
done:
    *accepted=result;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_pair_agent(const UCCollisionPairBody *a,
    const UCCollisionPairBody *b,const void **out) {
    UCCollisionStatus s;if(!out)return UC_COLLISION_ARGUMENT;
    if((s=valid(a))!=UC_COLLISION_OK||(s=valid(b))!=UC_COLLISION_OK)return s;
    const UCCollisionPairBody *scan=a->agent_count<=b->agent_count?a:b,*other=scan==a?b:a;
    const void *found=NULL;
    for(uint32_t i=0;i<scan->agent_count;i++)if(scan->agents[i].other_collidable==other->identity){found=scan->agents[i].agent;break;}
    *out=found;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_pair_admit(UCCollisionPairFilter filter,
    const UCCollisionPair *pairs,uint32_t count,int removing) {
    if((count&&!pairs)||(removing!=0&&removing!=1))return UC_COLLISION_ARGUMENT;
    if(filter!=UC_COLLISION_PAIR_GAME_FILTER)return UC_COLLISION_UNSUPPORTED;
    if(count>16383)return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<count;i++) {
        UCCollisionStatus s;
        if(removing) {
            const void *agent;
            s=uc_collision_pair_agent(pairs[i].first,pairs[i].second,&agent);
            if(s!=UC_COLLISION_OK)return s;
            if(agent)return UC_COLLISION_UNSUPPORTED;
        } else {
            int accepted;
            s=uc_collision_pair_filter(filter,pairs[i].first,pairs[i].second,&accepted);
            if(s!=UC_COLLISION_OK)return s;
            if(accepted)return UC_COLLISION_UNSUPPORTED;
        }
    }
    return UC_COLLISION_OK;
}
