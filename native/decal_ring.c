#include "decal_ring.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int valid(const UCDecalRing *r){return r&&r->entries&&r->capacity>=2&&r->capacity<=65536&&r->first<r->capacity&&r->next<r->capacity;}
static int arithmetic(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static uint32_t successor(const UCDecalRing *r,uint32_t i){return i+1==r->capacity?0:i+1;}
int uc_decal_ring_init(UCDecalRing *r,UCDecalEntry *entries,uint32_t capacity){
    if(!r||!entries||capacity<2||capacity>65536)return 1;
    *r=(UCDecalRing){entries,0,0,capacity};return 0;
}
int uc_decal_ring_insert(UCDecalRing *r,const char *shader,const float q[4][3],uint32_t flags,float fade,const UCDecalRingServices *h){
    if(!valid(r)||!q||!shader||!h||!h->preflight||!h->material||!h->light||!arithmetic()||!isfinite(fade)||fabsf(fade)>1e4f)return 1;
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<3;j++)if(!isfinite(q[i][j])||fabsf(q[i][j])>1e6f)return 1;
    float a[3],b[3],normal[3],point[3];
    for(unsigned j=0;j<3;j++){a[j]=q[2][j]-q[0][j];b[j]=q[1][j]-q[0][j];}
    for(unsigned j=0;j<3;j++){unsigned k=(j+1)%3,l=(j+2)%3;normal[j]=a[k]*b[l]-a[l]*b[k];}
    double length=sqrt(((double)normal[2]*normal[2]+(double)normal[1]*normal[1])+(double)normal[0]*normal[0]);
    if(!(length>0)||!isfinite(length))return 1;
    double inverse=1.0/length;
    for(unsigned j=0;j<3;j++){
        float center=0;for(unsigned i=0;i<4;i++)center=center+q[i][j];
        point[j]=center/4.f+(float)((double)normal[j]*inverse);
    }
    double dx=(double)q[0][0]-q[2][0],dy=(double)q[0][1]-q[2][1],dz=(double)q[0][2]-q[2][2];
    float diagonal=(float)sqrt((dz*dz+dy*dy)+dx*dx);
    float rate=diagonal>9.99999974737875163555e-6f?fade/diagonal:0.f;
    if(!isfinite(rate)||!h->preflight(h->context,shader,q))return 1;
    uint32_t first=r->first,next=r->next,steps=0,index=next;
    for(;;){
        next=successor(r,next);if(next!=first)break;
        first=successor(r,first);if(!(r->entries[next].flags&1))break;
        if(++steps>=r->capacity)return 1;
    }
    r->first=first;r->next=next;UCDecalEntry *entry=r->entries+index;
    void *material;if(h->material(h->context,shader,&material))return 2;
    entry->material=material;entry->flags=flags;entry->rate=rate;entry->progress=0;
    memcpy(entry->corners,q,sizeof entry->corners);
    uint32_t color;if(h->light(h->context,point,&color))return 2;
    entry->color[0]=(float)((color>>16)&255)*(1.f/255.f);
    entry->color[1]=(float)((color>>8)&255)*(1.f/255.f);
    entry->color[2]=(float)(color&255)*(1.f/255.f);return 0;
}
int uc_decal_ring_tick(UCDecalRing *r,float delta){
    if(!valid(r)||!isfinite(delta)||!arithmetic())return 1;if(delta<=0)return 0;
    for(uint32_t i=r->first;i!=r->next;i=successor(r,i)){
        const UCDecalEntry *e=r->entries+i;
        if(!isfinite(e->rate)||!isfinite(e->progress)||(e->rate>0&&e->progress<1&&!isfinite(e->rate*delta+e->progress)))return 1;
    }
    for(uint32_t i=r->first;i!=r->next;i=successor(r,i)){
        UCDecalEntry *e=r->entries+i;if(e->rate<=0)continue;
        if(e->progress<1)e->progress=e->rate*delta+e->progress;
        if(e->progress>=.7f&&e->rate>.025f)e->rate=e->rate-e->rate*.025f;
        if(e->progress>=1){e->progress=1;e->rate=0;}
    }return 0;
}
int uc_decal_ring_clear(UCDecalRing *r){if(!valid(r))return 1;r->first=r->next;return 0;}
