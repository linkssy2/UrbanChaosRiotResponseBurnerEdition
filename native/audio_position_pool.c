#include "audio_position_pool.h"
#include "audio_material_route.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int point_valid(const float *p){
    return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2])&&
        fabsf(p[0])<=1e15f&&fabsf(p[1])<=1e15f&&fabsf(p[2])<=1e15f;
}
static const float *position(const UCAudioPositionSource *s){
    return s->attached_position?s->attached_position:s->position;
}
static int valid(const UCAudioPositionPool *p){
    if(!p||p->active||!p->sources||!p->capacity||p->capacity>4096||p->count>p->capacity)return 0;
    for(uint32_t i=0;i<p->count;i++)
        if(!p->sources[i]||(i&&(uintptr_t)p->sources[i-1]>=(uintptr_t)p->sources[i])||
           !point_valid(position(p->sources[i])))return 0;
    return 1;
}
int uc_audio_position_pool_begin(UCAudioPositionPool *p,UCAudioPositionSource **storage,uint32_t cap){
    if(!p||p->sources||p->count||p->capacity||p->active||!storage||!cap||cap>4096)return 1;
    p->sources=storage;p->capacity=cap;return 0;
}
int uc_audio_position_pool_remove(UCAudioPositionPool *p,UCAudioPositionSource *s){
    if(!valid(p)||!s)return 1;
    for(uint32_t i=0;i<p->count;i++)if(p->sources[i]==s){
        --p->count;memmove(p->sources+i,p->sources+i+1,(p->count-i)*sizeof(*p->sources));break;
    }
    return 0;
}
int uc_audio_position_pool_end(UCAudioPositionPool *p){
    if(!valid(p)||p->count)return 1;
    memset(p,0,sizeof(*p));return 0;
}
static int request(UCAudioPositionPool *p,const void *bank,const char *name,
    const float point[3],float radius,int material,const UCAudioPositionServices *h,
    UCAudioPositionSource **out,uint8_t *accepted){
    UCAudioPositionSource *s=NULL;size_t n=0;float radius2=radius*radius;
    if(!valid(p)||!point_valid(point)||!name||!out||!accepted||!h||!h->preflight||
       !h->create||!h->play||(_mm_getcsr()&0xE040u))return 1;
    while(n<256&&name[n])++n;
    if(n==256)return 1;
    for(uint32_t i=0;i<p->count;i++){
        const float *v=position(p->sources[i]);
        float dz=point[2]-v[2],dy=point[1]-v[1],dx=point[0]-v[0];
        float distance=(dz*dz+dy*dy)+dx*dx;
        if(radius2>distance){s=p->sources[i];break;}
    }
    if((!s&&p->count==p->capacity)||!h->preflight(h->context,p,bank))return 1;
    p->active=1;*accepted=0;
    if(!s){
        s=h->create(h->context,point);*out=s;
        if(s){
            uint32_t i=0;
            if(!point_valid(position(s))){p->active=0;return 2;}
            while(i<p->count&&(uintptr_t)p->sources[i]<(uintptr_t)s)++i;
            if(i==p->count||p->sources[i]!=s){
                memmove(p->sources+i+1,p->sources+i,(p->count-i)*sizeof(*p->sources));
                p->sources[i]=s;++p->count;
            }
        }
    }else *out=s;
    if(s){
        if(h->play(h->context,s,bank,name,accepted)){p->active=0;return 2;}
        if(material&&!*accepted&&h->play(h->context,s,bank,uc_audio_material_name(0),accepted)){
            p->active=0;return 2;
        }
    }
    p->active=0;return 0;
}
int uc_audio_position_named(UCAudioPositionPool *p,const void *bank,const char *name,
    const float point[3],const UCAudioPositionServices *h,UCAudioPositionSource **out,uint8_t *accepted){
    return request(p,bank,name,point,10.f,0,h,out,accepted);
}
int uc_audio_position_material(UCAudioPositionPool *p,const void *bank,uint32_t material,
    const float point[3],const UCAudioPositionServices *h,UCAudioPositionSource **out,uint8_t *accepted){
    return request(p,bank,uc_audio_material_name(material),point,1.f,1,h,out,accepted);
}
