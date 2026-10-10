#include "decal_render.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
struct UCDecalBatch {const UCDecalRing *ring;uint32_t *seen,count,first;};
static const UCDecalEntry *entry(const UCDecalBatch*b,uint32_t i){
    i+=b->ring->first;if(i>=b->ring->capacity)i-=b->ring->capacity;return b->ring->entries+i;
}
int uc_decal_render(const UCDecalRing*r,uint32_t mode,const UCDecalRenderHooks*h,void*ctx){
    if(mode!=2)return 0;
    if(!r||!r->entries||r->capacity<2||r->capacity>65536||r->first>=r->capacity||r->next>=r->capacity||
       !h||!h->preflight||!h->special_enabled||!h->material||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    uint32_t seen[2048]={0};UCDecalBatch b={r,seen,(r->next+r->capacity-r->first)%r->capacity,0};
    for(uint32_t i=0;i<b.count;i++){
        const UCDecalEntry*e=entry(&b,i);if(!e->material||!isfinite(e->rate)||!isfinite(e->progress)||fabsf(e->progress)>1e4f)return 1;
        for(unsigned k=0;k<3;k++)if(!isfinite(e->color[k])||fabsf(e->color[k])>1e6f)return 1;
        for(unsigned j=0;j<4;j++)for(unsigned k=0;k<3;k++)if(!isfinite(e->corners[j][k])||fabsf(e->corners[j][k])>1e6f)return 1;
    }
    if(!h->preflight(ctx,r))return 1;uint8_t enabled=h->special_enabled(ctx);
    for(b.first=0;b.first<b.count;b.first++){
        uint32_t mask=1u<<(b.first&31),*word=seen+(b.first>>5);const UCDecalEntry*e=entry(&b,b.first);
        if(!enabled&&(e->flags&2))*word|=mask;
        if(!(*word&mask)&&h->material(ctx,e->material,&b))return 2;
    }return 0;
}
int uc_decal_draw_batch(const UCDecalBatch*b,const UCDecalDrawHooks*h,void*ctx){
    if(!b||!h||!h->device||!h->begin||!h->exposure||!h->quad||!h->end||b->first>=b->count)return 1;
    h->device(ctx);h->begin(ctx);void*material=entry(b,b->first)->material;
    for(uint32_t i=b->first;i<b->count;i++){
        const UCDecalEntry*e=entry(b,i);if(e->material!=material)continue;b->seen[i>>5]|=1u<<(i&31);
        float exposure=h->exposure(ctx);if(!isfinite(exposure)||exposure==0)return 2;
        double inverse=1.0/(double)exposure;float color[4],corners[4][3];color[3]=1;
        for(unsigned k=0;k<3;k++){color[k]=(float)((double)e->color[k]*inverse);if(!isfinite(color[k]))return 2;}
        float scale=(1.f-e->progress)*.5f;
        for(unsigned j=0;j<4;j++)for(unsigned k=0;k<3;k++){
            float p=e->corners[j][k];if(e->rate>0)p=p-(p-e->corners[(j+2)&3][k])*scale;corners[j][k]=p;
        }
        h->quad(ctx,e,corners,color);
    }h->end(ctx);return 0;
}
