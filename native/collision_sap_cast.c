#include "collision_sap_cast.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
static UCCollisionStatus candidate(const UCCollisionSAPView *v,uint32_t node,
    UCCollisionSAPCandidate call,void *context,float *limit) {
    float reported=*limit;UCCollisionStatus status=call(context,v->nodes[node].handle,&reported);
    if(status)return status;
    if(!isfinite(reported)||reported<0||reported>1)return UC_COLLISION_ARGUMENT;
    /* MSVC may lower the conditional assignment to MINSS with reversed zero
     * tie operands. Preserve the original callback's equal signed-zero bits. */
    if(reported<=*limit){volatile float exact=reported;*limit=exact;}
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_sap_cast(const UCCollisionSAPView *v,const float from[4],
    const float to[4],UCCollisionSAPCandidate call,void *context,float *out) {
    if(!v||!from||!to||!call||!out||!v->nodes||!v->node_count||
        v->node_count>UC_COLLISION_SAP_MAX_BODIES+1||v->endpoint_count!=v->node_count*2)return UC_COLLISION_ARGUMENT;
    if((_mm_getcsr()&0xe040u)||fegetround()!=FE_TONEAREST)return UC_COLLISION_UNSUPPORTED;
    if(!isfinite(v->bias))return UC_COLLISION_ARGUMENT;
    for(unsigned i=0;i<4;i++)if(!isfinite(from[i])||!isfinite(to[i])||
        !isfinite(v->offset_low[i])||!isfinite(v->scale[i]))return UC_COLLISION_ARGUMENT;
    for(unsigned axis=0;axis<3;axis++){
        const UCCollisionSAPEndpoint *e=v->axis[axis];
        if(!e||v->scale[axis]<=0||e[0].value||e[0].node||
            e[v->endpoint_count-1].value!=65533||e[v->endpoint_count-1].node)return UC_COLLISION_FORMAT;
        for(uint32_t i=1;i<v->endpoint_count-1;i++){
            uint32_t node=e[i].node;if(!node||node>=v->node_count||e[i].value<e[i-1].value||e[i].value>65533)return UC_COLLISION_FORMAT;
            uint16_t index=(e[i].value&1)?v->nodes[node].max[axis]:v->nodes[node].min[axis];
            if(index!=i)return UC_COLLISION_FORMAT;
        }
    }
    for(uint32_t node=1;node<v->node_count;node++){
        if(!v->nodes[node].handle||!v->nodes[node].handle->identity||v->nodes[node].handle->index!=node)return UC_COLLISION_FORMAT;
        for(unsigned a=0;a<3;a++)if(v->nodes[node].min[a]>=v->nodes[node].max[a])return UC_COLLISION_FORMAT;
    }
    uint8_t *flags=(uint8_t*)calloc(v->node_count,1);if(!flags)return UC_COLLISION_MEMORY;
    UCCollisionStatus status=UC_COLLISION_OK;int32_t at[3],step[3]={1,1,1};float inverse[3],base[3],time[3],delta[4];uint32_t quantized[4];
    __m128 start=_mm_loadu_ps(from),scaled=_mm_mul_ps(_mm_add_ps(start,_mm_loadu_ps(v->offset_low)),_mm_loadu_ps(v->scale));
    __m128 q=_mm_add_ps(_mm_max_ps(_mm_min_ps(scaled,_mm_set1_ps(65532)),_mm_setzero_ps()),_mm_set1_ps(65536));
    float quant[4];_mm_storeu_ps(quant,q);_mm_storeu_ps(delta,_mm_sub_ps(_mm_loadu_ps(to),start));
    for(unsigned a=0;a<4;a++){uint32_t bits;memcpy(&bits,&quant[a],4);quantized[a]=(bits>>7)&65535u;}
    /* Initial parity and first endpoint strictly beyond the quantized start.
     * Prefix XOR equals the original forward/backward optimized scans. */
    for(unsigned a=0;a<3;a++){
        const UCCollisionSAPEndpoint *e=v->axis[a];uint32_t i=1;
        while(i<v->endpoint_count&&e[i].value<=quantized[a]){flags[e[i].node]^=(uint8_t)(0x11u<<a);++i;}
        if(i==v->endpoint_count){status=UC_COLLISION_FORMAT;goto done;}at[a]=(int32_t)i;
        double d=(double)delta[a]*v->scale[a];float stored_d=(float)d;
        double s=((double)v->offset_low[a]+from[a])*v->scale[a];float stored_s=(float)s;
        double end=((double)to[a]+v->offset_low[a])*v->scale[a];
        if(!isfinite(delta[a])||!isfinite(stored_d)||!isfinite(stored_s)||!isfinite(end)){status=UC_COLLISION_UNSUPPORTED;goto done;}
        if(fabs(d)<s*0x1p-23||fabs(d)<end*0x1p-23){inverse[a]=0;base[a]=-2;}
        else{
            double reciprocal=1.0/(double)stored_d;inverse[a]=(float)reciprocal;
            base[a]=(float)(((double)stored_s-v->bias)*reciprocal);
            if(stored_d<0){step[a]=-1;--at[a];}
            if(!isfinite(inverse[a])||!isfinite(base[a])){status=UC_COLLISION_UNSUPPORTED;goto done;}
        }
        time[a]=(float)((double)e[at[a]].value*inverse[a]-base[a]);
        if(!isfinite(time[a])){status=UC_COLLISION_UNSUPPORTED;goto done;}
    }
    float limit=1;
    for(uint32_t node=1;node<v->node_count;node++)if(flags[node]==0x77){status=candidate(v,node,call,context,&limit);if(status)goto done;}
    flags[0]=0x88;
    uint32_t work=0,max_work=v->endpoint_count*3+3;
    for(;;){
        unsigned a=time[0]<time[1]?(time[0]<time[2]?0:2):(time[1]<time[2]?1:2);
        if(time[a]>limit)break;
        const UCCollisionSAPEndpoint *e=v->axis[a];
        for(;;){
            if(++work>max_work||at[a]<0||(uint32_t)at[a]>=v->endpoint_count){status=UC_COLLISION_LIMIT;goto done;}
            uint32_t node=e[at[a]].node;flags[node]^=(uint8_t)(0x10u<<a);
            if(flags[node]>=0x70){
                if(!node){time[a]=2;break;}
                status=candidate(v,node,call,context,&limit);if(status)goto done;
            }
            uint16_t value=e[at[a]].value;at[a]+=step[a];
            if(at[a]<0||(uint32_t)at[a]>=v->endpoint_count){status=UC_COLLISION_FORMAT;goto done;}
            if(e[at[a]].value==value)continue;
            time[a]=(float)((double)e[at[a]].value*inverse[a]-base[a]);
            if(!isfinite(time[a])){status=UC_COLLISION_UNSUPPORTED;goto done;}break;
        }
    }
    *out=limit;
done:free(flags);return status;
}
