#include "glass_ground.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int random_valid(const UCVectorRandom *r){if(!r||r->first>=28||r->second>=28)return 0;for(unsigned i=0;i<112;i++)if(!isfinite(r->values[i])||r->values[i]<0||r->values[i]>4000000)return 0;return 1;}
void uc_glass_ground_pool_init(UCGlassGroundPool*p){if(p)memset(p->used,0,sizeof p->used);}
int uc_glass_ground_valid(const UCGlassGroundStore*s){
    if(!s||!s->pool||!s->random||!s->sentinel||s->blocks>16)return 0;
    unsigned n=0,mask=0;const UCGlassBlock*last=s->sentinel;
    for(const UCGlassBlock*b=s->sentinel->next;b!=s->sentinel;b=b->next){if(!b||++n>16||b->previous!=last||b->index>=16||b->cursor>=4||!s->pool->used[b->index]||(mask&(1u<<b->index)))return 0;mask|=1u<<b->index;last=b;}
    return n==s->blocks&&s->sentinel->previous==last;
}
static int submit_service(void*c,const float packet[8]){uint32_t n;return uc_glass_ground_submit(c,(const float(*)[8])packet,1,&n);}
static void destroy_service(void*c){(void)uc_glass_ground_destroy(c);}
UCGlassGroundStore *uc_glass_ground_create(UCGlassGroundPool*p,UCVectorRandom*r,float floor){
    if(!p||!random_valid(r)||!isfinite(floor))return NULL;
    UCGlassGroundStore*s=malloc(sizeof *s);if(!s)return NULL;UCGlassBlock*b=malloc(sizeof *b);if(!b){free(s);return NULL;}
    b->next=b->previous=b;s->pool=p;s->random=r;s->sentinel=b;s->blocks=0;s->floor=floor;s->service=(UCGlassGround){s,submit_service,destroy_service};return s;
}
static UCGlassGroundRecord*slot(UCGlassGroundStore*s,UCGlassBlock*b){UCGlassGroundRecord*r=&s->pool->records[b->index][b->cursor];if(r->active)return NULL;b->cursor=(b->cursor+1)%4;r->active=1;return r;}
static UCGlassGroundRecord*acquire(UCGlassGroundStore*s,int*failure){
    for(UCGlassBlock*b=s->sentinel->next;b!=s->sentinel;b=b->next){UCGlassGroundRecord*r=slot(s,b);if(r)return r;}
    unsigned i=0;while(i<16&&s->pool->used[i])i++;if(i==16)return NULL;s->pool->used[i]=1;for(unsigned j=0;j<4;j++)s->pool->records[i][j].active=0;
    UCGlassBlock*b=malloc(sizeof *b);if(!b){s->pool->used[i]=0;*failure=1;return NULL;}b->index=i;b->cursor=0;UCGlassGroundRecord*r=slot(s,b);b->next=s->sentinel->next;b->previous=s->sentinel;b->next->previous=b;s->sentinel->next=b;s->blocks++;return r;
}
static float uniform(UCVectorRandom*r){unsigned a=r->first?r->first-1:27,b=r->second?r->second-1:27;for(unsigned i=0;i<4;i++){float v=r->values[4*a+i]-r->values[4*b+i];r->values[4*a+i]=v+(v<0?4000000.f:0.f);}r->first=a;r->second=b;return r->values[4*a]*(1.f/4000000.f);}
int uc_glass_ground_submit(UCGlassGroundStore*s,const float(*p)[8],int32_t count,uint32_t*accepted){
    if(!accepted||!uc_glass_ground_valid(s)||!arithmetic()||!random_valid(s->random)||(count>0&&!p))return 1;*accepted=0;
    for(int32_t n=0;n<count;n++){int failure=0;UCGlassGroundRecord*r=acquire(s,&failure);if(!r)return failure?2:0;r->velocity[0]=p[n][6];r->velocity[1]=p[n][7];
        for(unsigned j=0;j<3;j++){memcpy(r->triangle[j],p[n]+2*j,8);r->fade=(uniform(s->random)+.1f)*.3f;}r->alpha=1;(*accepted)++;}
    return 0;
}
int uc_glass_ground_update(UCGlassGroundStore*s,float delta){
    if(!uc_glass_ground_valid(s)||!arithmetic()||!isfinite(delta)||delta<0||delta>1)return 1;
    UCGlassBlock*b=s->sentinel->next;
    while(b!=s->sentinel){int active=0;for(unsigned i=0;i<4;i++){UCGlassGroundRecord*r=&s->pool->records[b->index][i];if(!r->active)continue;active=1;r->alpha-=r->fade*delta;if(0.f>r->alpha){r->active=0;continue;}
        float x=r->velocity[0]*delta,y=r->velocity[1]*delta;for(unsigned j=0;j<3;j++){r->triangle[j][0]+=x;r->triangle[j][1]+=y;}float damping=delta*4.f;if(damping>.99f)damping=.99f;for(unsigned j=0;j<2;j++)r->velocity[j]-=r->velocity[j]*damping;
        }UCGlassBlock*next=b->next;if(!active){s->pool->used[b->index]=0;b->previous->next=next;next->previous=b->previous;free(b);s->blocks--;}b=next;
    }return 0;
}
int uc_glass_ground_triangles(const UCGlassGroundStore*s,UCGlassTriangle out[256],uint32_t*count){
    if(!uc_glass_ground_valid(s)||!out||!count)return 1;uint32_t n=0;
    for(const UCGlassBlock*b=s->sentinel->next;b!=s->sentinel&&n<256;b=b->next)for(unsigned i=0;i<4&&n<256;i++){
        const UCGlassGroundRecord*r=&s->pool->records[b->index][i];if(!r->active)continue;UCGlassTriangle*t=&out[n++];
        for(unsigned j=0;j<3;j++){t->vertex[j].position[0]=r->triangle[j][0];t->vertex[j].position[1]=s->floor;t->vertex[j].position[2]=r->triangle[j][1];}
        t->vertex[0].uv[0]=t->vertex[0].uv[1]=t->vertex[1].uv[1]=t->vertex[2].uv[0]=0;t->vertex[1].uv[0]=t->vertex[2].uv[1]=1;t->alpha=r->alpha;
    }*count=n;return 0;
}
int uc_glass_ground_destroy(UCGlassGroundStore*s){
    if(!uc_glass_ground_valid(s))return 1;UCGlassBlock*b=s->sentinel->next;for(;b!=s->sentinel;b=b->next)s->pool->used[b->index]=0;
    b=s->sentinel->next;s->sentinel->next=s->sentinel->previous=s->sentinel;s->blocks=0;while(b!=s->sentinel){UCGlassBlock*next=b->next;free(b);b=next;}free(s->sentinel);s->sentinel=NULL;free(s);return 0;
}
