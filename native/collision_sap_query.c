#include "collision_sap_query.h"
static int valid(const UCCollisionSAPView *v,const UCCollisionSAPHandle *selected) {
    if(!v||!selected||!v->nodes||v->node_count<1||v->node_count>UC_COLLISION_SAP_MAX_BODIES+1||
       v->endpoint_count!=v->node_count*2||!selected->index||selected->index>=v->node_count||
       v->nodes[selected->index].handle!=selected||v->nodes[0].handle)return 0;
    for(uint32_t i=1;i<v->node_count;i++) {
        const UCCollisionSAPNode *n=v->nodes+i;
        if(!n->handle||!n->handle->identity||n->handle->index!=i)return 0;
        for(unsigned a=0;a<3;a++)if(!n->min[a]||n->min[a]>=n->max[a]||n->max[a]>=v->endpoint_count-1)return 0;
    }
    for(unsigned a=0;a<3;a++) {
        const UCCollisionSAPEndpoint *e=v->axis[a];uint32_t end=v->endpoint_count-1;
        if(!e||e[0].value||e[0].node||e[end].value!=0xFFFD||e[end].node)return 0;
        for(uint32_t i=0;i<=end;i++) {
            if(e[i].node>=v->node_count||(i&&e[i-1].value>e[i].value)||
               (i&&i<end&&!e[i].node))return 0;
            const UCCollisionSAPNode *n=v->nodes+e[i].node;
            if((uint32_t)(e[i].value&1?n->max[a]:n->min[a])!=i)return 0;
        }
    }
    return 1;
}
static int overlaps(const UCCollisionSAPNode *a,const UCCollisionSAPNode *b) {
    for(unsigned axis=0;axis<3;axis++)if(a->max[axis]<b->min[axis]||b->max[axis]<a->min[axis])return 0;
    return 1;
}
UCCollisionStatus uc_collision_sap_query(const UCCollisionSAPView *view,
    const UCCollisionSAPHandle *selected,UCCollisionSAPPairs *pairs) {
    if(!pairs||pairs->count>pairs->capacity||(pairs->capacity&&!pairs->data)||!valid(view,selected))return UC_COLLISION_ARGUMENT;
    const UCCollisionSAPNode *node=view->nodes+selected->index;uint32_t count=0;
    for(uint32_t i=1;i<view->node_count;i++)if(i!=selected->index&&overlaps(node,view->nodes+i))++count;
    if(count>pairs->capacity-pairs->count)return UC_COLLISION_LIMIT;
    for(uint32_t i=1;i<view->node_count;i++)if(i!=selected->index&&overlaps(node,view->nodes+i))
        pairs->data[pairs->count++]=(UCCollisionSAPPair){selected,view->nodes[i].handle};
    return UC_COLLISION_OK;
}
