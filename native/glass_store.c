#include "glass_store.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
void uc_glass_pool_init(UCGlassPool *p){if(p)memset(p->used,0,sizeof p->used);}
int uc_glass_store_valid(const UCGlassStore *s){
    if(!s||!s->pool||!s->sentinel||s->blocks>32)return 0;
    uint32_t count=0,mask=0;const UCGlassBlock *last=s->sentinel;
    for(const UCGlassBlock *b=s->sentinel->next;b!=s->sentinel;b=b->next){
        if(!b||++count>32||b->previous!=last||b->index>=32||b->cursor>=8||!s->pool->used[b->index]||(mask&(1u<<b->index)))return 0;
        mask|=1u<<b->index;last=b;
    }
    return count==s->blocks&&s->sentinel->previous==last;
}
int uc_glass_store_init(UCGlassStore *s,UCGlassPool *p,const float m[16],float floor,UCGlassGround *g){
    if(!s||!p||!m||!isfinite(floor)||(g&&(!g->submit||!g->destroy)))return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 1;
    UCGlassBlock *b=malloc(sizeof *b);if(!b)return 1;
    b->next=b->previous=b;s->sentinel=b;s->blocks=0;s->pool=p;s->ground=g;s->floor=floor;memcpy(s->matrix,m,64);return 0;
}
static UCGlassRecord *slot(UCGlassStore *s,UCGlassBlock *b){
    UCGlassRecord *r=&s->pool->records[b->index][b->cursor];
    if(r->active)return NULL;b->cursor=(b->cursor+1)%8;r->active=1;return r;
}
static UCGlassRecord *acquire(UCGlassStore *s,int *failure){
    for(UCGlassBlock *b=s->sentinel->next;b!=s->sentinel;b=b->next){UCGlassRecord *r=slot(s,b);if(r)return r;}
    unsigned i=0;while(i<32&&s->pool->used[i])i++;if(i==32)return NULL;
    s->pool->used[i]=1;for(unsigned j=0;j<8;j++)s->pool->records[i][j].active=0;
    UCGlassBlock *b=malloc(sizeof *b);if(!b){s->pool->used[i]=0;*failure=1;return NULL;}
    b->index=i;b->cursor=0;UCGlassRecord *r=slot(s,b);
    b->next=s->sentinel->next;b->previous=s->sentinel;b->next->previous=b;s->sentinel->next=b;s->blocks++;return r;
}
static void transform(float out[3],const float m[16],const float in[3],int point){
    for(unsigned i=0;i<3;i++){float z=m[8+i]*in[2]+(point?m[12+i]:0.f);out[i]=(m[i]*in[0]+m[4+i]*in[1])+z;}
}
static void rotation(float q[4],const float axis[3],float angle){
    double half=(double)angle*.5,cosine=cos(half);float sine=(float)sin(half);
    q[0]=(float)((double)sine*axis[0]);q[1]=(float)((double)sine*axis[1]);q[2]=axis[2]*sine;
    double length=sqrt((((double)q[0]*q[0]+(double)q[1]*q[1])+(double)q[2]*q[2])+cosine*cosine),inverse=1./length;
    for(unsigned i=0;i<3;i++)q[i]=(float)(inverse*q[i]);q[3]=(float)(cosine*inverse);
}
int uc_glass_store_submit(UCGlassStore *s,const UCGlassShard *p,int32_t count,uint32_t *accepted){
    if(!accepted||!uc_glass_store_valid(s)||!arithmetic()||(count>0&&!p))return 1;*accepted=0;
    for(int32_t n=0;n<count;n++){
        int failure=0;UCGlassRecord *r=acquire(s,&failure);if(!r)return failure?2:0;
        float center[3]={p[n].center[0],p[n].center[1],0},axis[3];
        transform(r->position,s->matrix,center,1);transform(r->velocity,s->matrix,p[n].velocity,0);
        for(unsigned i=0;i<3;i++){
            float v[3]={p[n].triangle[i][0],p[n].triangle[i][1],0};
            r->floor_triangle[i][0]=v[0]*4.f;r->floor_triangle[i][1]=v[1]*4.f;
            transform(r->triangle[i],s->matrix,v,0);for(unsigned j=0;j<3;j++)r->triangle[i][j]*=1.6f;
        }
        transform(axis,s->matrix,p[n].axis,0);rotation(r->rotation,axis,p[n].spin*(1.f/30.f));r->active=1;(*accepted)++;
    }return 0;
}
static void rotate(float v[3],const float q[4]){
    float x=v[0]*2.f,y=v[1]*2.f,z=v[2]*2.f;
    float a=q[2]*y-q[1]*z,b=q[0]*z-q[2]*x,c=q[1]*x-q[0]*y;
    float d=q[2]*b-q[1]*c,e=q[0]*c-q[2]*a,f=q[1]*a-q[0]*b;
    v[0]=(v[0]+q[3]*a)+d;v[1]=(q[3]*b+v[1])+e;v[2]=(q[3]*c+v[2])+f;
}
static int random_valid(const UCVectorRandom *r){
    if(!r||r->first>=28||r->second>=28)return 0;
    for(unsigned i=0;i<112;i++)if(!isfinite(r->values[i])||r->values[i]<0||r->values[i]>4000000)return 0;return 1;
}
static float uniform(UCVectorRandom *r){
    unsigned a=r->first?r->first-1:27,b=r->second?r->second-1:27;
    for(unsigned i=0;i<4;i++){float v=r->values[4*a+i]-r->values[4*b+i];r->values[4*a+i]=v+(v<0?4000000.f:0.f);}
    r->first=a;r->second=b;return r->values[4*a]*(1.f/4000000.f);
}
int uc_glass_store_update(UCGlassStore *s,UCVectorRandom *random,float delta){
    if(!uc_glass_store_valid(s)||!arithmetic()||!isfinite(delta)||delta<0||delta>1||!random_valid(random)||!s->ground||!s->ground->submit||!s->ground->destroy)return 1;
    UCGlassBlock *b=s->sentinel->next;
    while(b!=s->sentinel){
        int active=0;
        for(unsigned i=0;i<8;i++){
            UCGlassRecord *r=&s->pool->records[b->index][i];if(!r->active)continue;active=1;
            r->velocity[1]-=delta*1200.f;for(unsigned j=0;j<3;j++)rotate(r->triangle[j],r->rotation);
            for(unsigned j=0;j<3;j++)r->position[j]+=r->velocity[j]*delta;
            if(s->floor>r->position[1]){
                if(uniform(random)*-100.f>r->velocity[1]){r->velocity[1]=(r->velocity[1]*uniform(random))*-.3f;r->position[1]=s->floor+1.f;}
                else{
                    float packet[8];r->active=0;
                    for(unsigned j=0;j<3;j++){packet[2*j]=r->floor_triangle[j][0]+r->position[0];packet[2*j+1]=r->floor_triangle[j][1]+r->position[2];}
                    packet[6]=r->velocity[0];packet[7]=r->velocity[2];
                    if(s->ground->submit(s->ground->context,packet)||!random_valid(random))return 2;
                }
            }
        }
        UCGlassBlock *next=b->next;
        if(!active){s->pool->used[b->index]=0;b->previous->next=next;next->previous=b->previous;free(b);s->blocks--;}
        b=next;
    }return 0;
}
int uc_glass_store_triangles(const UCGlassStore *s,UCGlassTriangle scratch[257],uint32_t *count){
    if(!uc_glass_store_valid(s)||!scratch||!count||!arithmetic())return 1;uint32_t n=0;
    for(const UCGlassBlock *b=s->sentinel->next;b!=s->sentinel&&n<=256;b=b->next)for(unsigned i=0;i<8&&n<=256;i++){
        const UCGlassRecord *r=&s->pool->records[b->index][i];if(!r->active)continue;
        UCGlassTriangle *t=&scratch[n++];for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++)t->vertex[j].position[k]=r->triangle[j][k]+r->position[k];
        t->vertex[0].uv[0]=t->vertex[0].uv[1]=t->vertex[1].uv[1]=t->vertex[2].uv[0]=0;
        t->vertex[1].uv[0]=t->vertex[2].uv[1]=t->alpha=1;
    }*count=n;return 0;
}
int uc_glass_store_destroy(UCGlassStore *s){
    if(!uc_glass_store_valid(s)||(s->ground&&!s->ground->destroy))return 1;
    if(s->ground)s->ground->destroy(s->ground->context);
    UCGlassBlock *b=s->sentinel->next;for(;b!=s->sentinel;b=b->next)s->pool->used[b->index]=0;
    b=s->sentinel->next;s->sentinel->next=s->sentinel->previous=s->sentinel;s->blocks=0;
    while(b!=s->sentinel){UCGlassBlock *next=b->next;free(b);b=next;}
    free(s->sentinel);s->sentinel=NULL;return 0;
}
