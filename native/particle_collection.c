#include "particle_collection.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int finite_array(const float *p,unsigned n){
    for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;
}
static int bounds(const UCParticleLifecycleView *v,UCModelBounds *b){
    if(!v||!v->asset||!v->system||!v->source_matrix)return 0;
    const UCParticleDefinition *d=&v->asset->simulation;
    float radius=(d->end_size>d->start_size?d->end_size:d->start_size)*.5f;
    if(!isfinite(d->end_size)||!isfinite(d->start_size)||!isfinite(radius))return 0;
    for(unsigned k=0;k<3;k++){
        b->minimum[k]=v->system->bounds.minimum[k]-radius;
        b->maximum[k]=v->system->bounds.maximum[k]+radius;
        b->center[k]=(b->maximum[k]+b->minimum[k])*.5f;
        b->extent[k]=(b->maximum[k]-b->minimum[k])*.5f;
    }
    return finite_array(b->minimum,3)&&finite_array(b->maximum,3)&&
        finite_array(b->center,3)&&finite_array(b->extent,3);
}
int uc_particle_depth_key(const UCParticleLifecycleView *v,const float frame[16],float *out){
    if(!v||!v->asset||!v->source_matrix||!frame||!out||!finite_array(frame,16))return 0;
    if(!finite_array(v->source_matrix+12,4))return 0;
    __m128 a=_mm_sub_ps(_mm_loadu_ps(v->source_matrix+12),_mm_loadu_ps(frame+12));
    a=_mm_mul_ps(a,a);a=_mm_add_ps(a,_mm_movehl_ps(a,a));
    a=_mm_add_ss(a,_mm_shuffle_ps(a,a,1));
    float square=_mm_cvtss_f32(a);if(!isfinite(square)||square<=0)return 0;
    __m128 r=_mm_rsqrt_ss(a),product=_mm_mul_ss(r,a);
    product=_mm_mul_ss(product,r);product=_mm_sub_ss(_mm_set_ss(3),product);
    r=_mm_mul_ss(_mm_set_ss(.5f),r);r=_mm_mul_ss(r,product);a=_mm_mul_ss(a,r);
    float distance=_mm_cvtss_f32(a);
    float key=(float)(-(((double)distance-(double)v->asset->depth_bias)-900.0));
    if(!isfinite(distance)||!isfinite(key))return 0;
    *out=key;return 1;
}
int uc_particle_collect(const UCParticleRegistry *r,const float clip[16],const float frame[16],
    uint32_t mask,UCParticleCollection *out){
    if(!r||!out||!clip||!frame||uc_particle_registry_count(r)>32||
       !finite_array(clip,16)||!finite_array(frame,16))return 1;
    UCParticleCollection next={0};
    for(uintptr_t token=uc_particle_registry_first(r);token;token=uc_particle_registry_next(r,token)){
        const UCParticleLifecycleView *v=uc_particle_registry_view(r,token);UCModelBounds b;uint32_t visible;float key=899;
        const UCGunFlareSpriteSource *s=uc_particle_registry_sprite(r,token);
        if(s){
            for(unsigned k=0;k<3;k++){b.minimum[k]=-99999;b.maximum[k]=99999;b.center[k]=0;b.extent[k]=99999;}
        }else if(!bounds(v,&b))return 1;
        if(uc_model_visibility(clip,&b,&visible)!=UC_MODEL_OK)return 1;
        if(!visible)continue;
        if(!s&&!uc_particle_depth_key(v,frame,&key))return 1;
        if(mask&0x80){UCParticleRenderRecord record={token,v,5,0x110,0,key,s};next.records[next.count++]=record;}
    }
    /* Same stable insertion order as original98030; the actual caller selects
     * this branch for <=32 records. Equal floating keys never move. */
    for(uint32_t i=1;i<next.count;i++){
        UCParticleRenderRecord x=next.records[i];uint32_t j=i;
        while(j&&next.records[j-1].depth>x.depth){next.records[j]=next.records[j-1];j--;}
        next.records[j]=x;
    }
    *out=next;return 0;
}
int uc_particle_collection_draw(const UCParticleCollection *c,UCParticleRenderPass *p,
    int present,float exposure,uint8_t disabled0,uint8_t disabled1,const UCParticlePassHooks *h,void *ctx){
    if(!c||c->count>32)return 1;if(!c->count)return 0;
    if(!p||!h||!h->render_state||!h->constant119||!h->world_identity||!h->draw)return 1;
    float rgba[4]={1,1,1,1};
    if(present){if(!isfinite(exposure)||exposure==0)return 1;rgba[0]=rgba[1]=rgba[2]=(float)(1.0/(double)exposure);if(!isfinite(rgba[0]))return 1;}
    for(uint32_t i=0;i<c->count;i++){
        const UCParticleRenderRecord *r=c->records+i;
        if(!r->token||!!r->view==!!r->sprite||r->group!=5||r->flags!=0x110||r->partition||!isfinite(r->depth))return 1;
    }
    h->render_state(ctx,0x4035C,0);h->constant119(ctx,rgba);
    if(!disabled0&&!disabled1){
        p->mode=2;h->render_state(ctx,0x40344,0x306);h->render_state(ctx,0x40348,0);
        for(uint32_t i=0;i<c->count;i++){
            const UCParticleRenderRecord *r=c->records+i;
            h->world_identity(ctx,r->token);p->partition=0;h->draw(ctx,r,p);
        }
    }
    h->render_state(ctx,0x40304,0);h->render_state(ctx,0x40300,0);return 0;
}
