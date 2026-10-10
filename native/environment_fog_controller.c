#include "environment_fog_controller.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int valid(const UCFogProperties*p){
    if(!p||p->distance_to_full!=0||!isfinite(p->start)||!isfinite(p->end)||
       fabsf(p->start)>1e8f||fabsf(p->end)>1e8f||!isfinite(p->maximum)||
       p->maximum<0||p->maximum>1||fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xe040))return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(p->color[i])||p->color[i]<0||p->color[i]>1)return 0;
    return 1;
}
static uint32_t pack(const float*c){uint32_t a=0xff000000;for(unsigned i=0;i<3;i++)a|=(uint32_t)(c[i]*255.f)<<((2-i)*8);return a;}
int uc_environment_fog_controller_init(UCEnvironmentFogController*s,const UCFogProperties*p){
    if(!s||!valid(p))return 1;s->properties=*p;s->properties.enabled=(uint8_t)p->enabled!=0;s->first=1;s->transition=0;return 0;
}
static void install(UCEnvironmentFogController*s,const UCEnvironmentFogHooks*h,void*ctx){
    UCEnvironmentFogState*f=h->scene(ctx)->fog;f->enabled=(uint8_t)s->properties.enabled;
    f->start=s->properties.start;f->end=s->properties.end;f->color=pack(s->properties.color);f->maximum=s->properties.maximum;
}
int uc_environment_fog_controller_install(UCEnvironmentFogController*s,const UCEnvironmentFogHooks*h,void*ctx){
    if(!s||!valid(&s->properties)||!h||!h->scene)return 1;install(s,h,ctx);return 0;
}
static void blend(UCEnvironmentFogController*s,const UCEnvironmentFogHooks*h,void*ctx){
    UCEnvironmentFogState*f=h->scene(ctx)->fog;f->enabled=(uint8_t)s->properties.enabled;
    float old[3];for(unsigned i=0;i<3;i++)old[i]=(float)((f->color>>((2-i)*8))&255)*(1.f/255.f);
    float start=f->start,end=f->end,max=f->maximum;
    /* Targets precede the first time getter, exactly as195663..1956A7. */
    UCFogProperties p=s->properties;
    float e=(float)pow(.10000000149011612,(double)h->delta(ctx));
    float b=(float)pow(.10000000149011612,(double)h->delta(ctx));
    float x=(1.f-e)*p.end,y=e*end;f->end=x+y;
    x=(1.f-b)*p.start;y=b*start;f->start=x+y;
    float blue=(float)pow(.10000000149011612,(double)h->delta(ctx));
    float green=(float)pow(.10000000149011612,(double)h->delta(ctx));
    double red=pow(.10000000149011612,(double)h->delta(ctx));
    float color[3]={(float)((1.-red)*p.color[0]+red*old[0]),
        (float)((1.-(double)green)*p.color[1]+(double)green*old[1]),
        (float)((1.-(double)blue)*p.color[2]+(double)blue*old[2])};
    f->color=pack(color);
    double k=pow(.10000000149011612,(double)h->delta(ctx));f->maximum=(float)((1.-k)*p.maximum+k*max);
}
static int member(const UCScenePortalStart*r,const float*p){
    for(uint32_t i=0;i<r->count;i++){const UCScenePortalRoom*q=r->rooms[i];
        if(p[0]>=q->minimum[0]&&q->maximum[0]>=p[0]&&p[1]>=q->minimum[1]&&q->maximum[1]>=p[1]&&p[2]>=q->minimum[2]&&q->maximum[2]>=p[2])return 1;
    }return 0;
}
int uc_environment_fog_controller_update(UCEnvironmentFogController*s,const float*p,const UCEnvironmentFogHooks*h,void*ctx){
    if(!s||!valid(&s->properties)||!p||!h||!h->scene||!h->delta||!h->world_mode)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i]))return 1;
    if((uint8_t)h->world_mode(ctx))return 0;
    if(!h->scene(ctx))return 0;
    if(!h->scene(ctx)->rooms)return 0;
    int inside=member(h->scene(ctx)->rooms,p);
    if(s->first){UCEnvironmentFogState*f=h->scene(ctx)->fog;
        if(inside||!f->enabled){install(s,h,ctx);s->transition=1;}s->first=0;
    }else if(inside){blend(s,h,ctx);s->transition=1;}else s->transition=0;
    return 0;
}
