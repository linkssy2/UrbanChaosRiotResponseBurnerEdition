#include "collision_scenery_trace.h"
#include <math.h>
int uc_collision_scenery_trace(const UCCollisionSceneryTrace *owner,const float start[3],
    const float end[3],uint32_t type,uint32_t group,uint32_t mask,float impulse,
    const UCCollisionResult **out){
    if(!owner||!owner->queries||!owner->hits||!out||!isfinite(impulse))return -1;
    UCCollisionRay ray;
    int status=uc_collision_ray_prepare(start,end,type,group,mask,&ray);
    if(status<0)return -1;
    ++*owner->queries;
    if(status&&owner->query_result&&owner->world){
        status=uc_collision_scenery_index_cast(owner->world,&ray,impulse,owner->query_result);
        if(status<0)return -2;
        if(status){++*owner->hits;*out=owner->query_result;return 1;}
    }
    *out=NULL;return 0;
}
