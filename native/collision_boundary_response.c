#include "collision_boundary_response.h"
UCCollisionStatus uc_collision_boundary_response(const UCCollisionBoundary *b,int added,
    uint32_t type,const void *body,const UCCollisionBoundaryResponseHooks *h,void *context) {
    if(!b||(added!=0&&added!=1)||type>255||!h||!h->motion||!h->remove)return UC_COLLISION_ARGUMENT;
    if(!added||type!=1||!body)return UC_COLLISION_OK;
    if(b->mode<2)h->motion(context,body,7,1,0);
    else if(b->mode==2){
        if(!b->world)return UC_COLLISION_ARGUMENT;
        h->remove(context,b->world,body);
    }
    return UC_COLLISION_OK;
}
