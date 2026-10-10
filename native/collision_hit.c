#include "collision_hit.h"
#include <math.h>
static int fraction(float v){return isfinite(v)&&v>=0&&v<=1;}
static int normal(const UCCollisionTriangleHit *h){for(unsigned i=0;i<4;++i)if(!isfinite(h->normal[i]))return 0;return 1;}
int uc_collision_terminal_filter(uint32_t type,uint32_t ray,uint32_t material){
    return (type!=24&&type!=6&&type!=16)||(((ray>>20)&material&63)!=0);
}
int uc_collision_hit_route(const UCCollisionClosest *current,const UCCollisionTriangleHit *hit,
    const UCCollisionBodyMetadata *body,uint32_t count,UCCollisionHitRoute *out){
    UCCollisionHitRoute r={0};uint32_t owner=0;
    if(!current||!hit||!out||!fraction(current->hit.fraction)||!fraction(hit->fraction))return -1;
    if(!(hit->fraction<current->hit.fraction))return 0;
    if(!body||!count||count>64||!normal(hit))return -1;
    for(uint32_t i=0;i<count;++i){
        if(!body[i].identity)return -1;
        for(uint32_t j=0;j<i;++j)if(body[j].identity==body[i].identity)return -1;
    }
    r.hit=*hit;r.root_body=body[count-1].identity;r.material_key=body[0].shape_key;
    r.filter_info=body[count-1].filter_info;
    if(body[0].shape_type==6){
        if(count<2)return -1;
        for(owner=1;owner<count;++owner)
            if(body[owner].shape_type==2||body[owner].shape_type==16||body[owner].shape_type==24)break;
        if(owner==count){r.filter_info=0;*out=r;return 1;}
    }else if(body[0].shape_type!=24){r.material=body[count-1].shape_user_data;*out=r;return 1;}
    r.material_body=owner;
    r.mode=body[owner].shape_type==24?UC_COLLISION_MATERIAL_CHILD:UC_COLLISION_MATERIAL_SHAPE;
    *out=r;return 1;
}
int uc_collision_hit_commit(UCCollisionClosest *current,const UCCollisionHitRoute *route,uint32_t material){
    UCCollisionClosest next;
    if(!current||!route||!fraction(current->hit.fraction)||!fraction(route->hit.fraction))return -1;
    if(!(route->hit.fraction<current->hit.fraction))return 0;
    if(!route->root_body||!normal(&route->hit)||route->mode<UC_COLLISION_MATERIAL_DIRECT||route->mode>UC_COLLISION_MATERIAL_CHILD)return -1;
    next=*current;next.hit=route->hit;next.limit=route->hit.fraction;next.root_body=route->root_body;
    next.material=route->mode==UC_COLLISION_MATERIAL_DIRECT?route->material:material;
    next.filter_info=route->filter_info;*current=next;return 1;
}
