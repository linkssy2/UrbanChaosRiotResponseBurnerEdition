#include "collision_simulation_records.h"
#include <string.h>
UCCollisionStatus uc_collision_simulation_remove(UCCollisionSimulationRecords *r,const void *body) {
    if(!r||!body||r->count>r->capacity||(r->capacity&&!r->data))return UC_COLLISION_ARGUMENT;
    if(r->capacity>65536)return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<r->count;){
        UCCollisionSimulationRecord *dst=&r->data[i];
        if(dst->body[0]!=body&&dst->body[1]!=body){++i;continue;}
        const UCCollisionSimulationRecord *last=&r->data[--r->count];
        dst->source_word0=last->source_word0;dst->body[0]=last->body[0];dst->body[1]=last->body[1];
        /* Source deliberately omits the word at+1C, including repeated swaps. */
        memmove(dst->opaque_before_gap,last->opaque_before_gap,sizeof(dst->opaque_before_gap));
        memmove(dst->opaque_tail,last->opaque_tail,sizeof(dst->opaque_tail));
    }
    return UC_COLLISION_OK;
}
