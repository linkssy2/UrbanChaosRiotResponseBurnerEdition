#include "model_light_membership.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
struct UCModelLightMembership {
    uint32_t capacity,generation,retained_count,member_count;
    float center[3];
    UCModelLightScore *retained,*temporary;
    const UCModelMembershipLight **members;
};
static int finite_values(const float *p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
UCModelLightMembership *uc_model_light_membership_create(uint32_t capacity,uint32_t generation,const float center[3]){
    if(!capacity||capacity>65536||!finite_values(center,3))return NULL;
    size_t bytes=sizeof(UCModelLightMembership)+(size_t)capacity*(2*sizeof(UCModelLightScore)+2*sizeof(void*));
    UCModelLightMembership *s=calloc(1,bytes);if(!s)return NULL;
    s->capacity=capacity;s->generation=generation;memcpy(s->center,center,12);
    s->retained=(UCModelLightScore*)(s+1);s->temporary=s->retained+capacity;
    s->members=(const UCModelMembershipLight**)(s->temporary+capacity);return s;
}
void uc_model_light_membership_free(UCModelLightMembership *s){free(s);}
int uc_model_light_intersection(const UCPointLightGeometry *g,const UCModelBounds*b,int*out){
    if(!g||!b||!out||!arithmetic()||!finite_values(g->scale,3)||!finite_values(g->offset,3)||!finite_values(b->minimum,3)||!finite_values(b->maximum,3))return 0;
    float low[3],high[3];for(unsigned i=0;i<3;i++){low[i]=g->offset[i]+b->minimum[i]*g->scale[i];high[i]=g->offset[i]+b->maximum[i]*g->scale[i];}
    if(!finite_values(low,3)||!finite_values(high,3))return 0;
    float distance=0;const unsigned order[3]={0,2,1};
    for(unsigned j=0;j<3;j++){unsigned i=order[j];float d=low[i]>0?low[i]:(high[i]<0?high[i]:0);if(d!=0)distance=d*d+distance;if(distance>.25f){*out=0;return 1;}}
    *out=distance<.25f;return 1;
}
int uc_model_light_score(const UCPointLightGeometry*g,const float p[4],float*out){
    if(!g||!out||!arithmetic()||!finite_values(g->world_to_volume,16)||!finite_values(p,4))return 0;
    float q[4];for(unsigned i=0;i<4;i++)q[i]=(g->world_to_volume[i]*p[0]+g->world_to_volume[4+i]*p[1])+(g->world_to_volume[8+i]*p[2]+g->world_to_volume[12+i]*p[3]);
    float squared=(q[0]*q[0]+q[2]*q[2])+(q[1]*q[1]+q[3]*q[3]);
    if(!isfinite(squared)||squared<=0)return 0;
    float r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
    float product=(r*squared)*r,refinement=(.5f*r)*(3.f-product),score=squared*refinement;
    if(!isfinite(score))return 0;*out=score;return 1;
}
static void transform(const UCModelBounds*b,const float m[16],UCModelBounds*out){
    float c[3],e[3];for(unsigned i=0;i<3;i++){c[i]=(b->minimum[i]+b->maximum[i])*.5f;e[i]=(b->maximum[i]-b->minimum[i])*.5f;}
    for(unsigned i=0;i<3;i++){
        float middle=(m[i]*c[0]+m[12+i])+(m[4+i]*c[1]+m[8+i]*c[2]);
        float radius=fabsf(m[i]*e[0])+(fabsf(m[4+i]*e[1])+fabsf(m[8+i]*e[2]));
        float low=middle-radius,high=middle+radius;out->minimum[i]=high<low?high:low;out->maximum[i]=high>low?high:low;
        out->center[i]=(out->maximum[i]+out->minimum[i])*.5f;out->extent[i]=(out->maximum[i]-out->minimum[i])*.5f;
    }
}
static void insert_score(UCModelLightScore*v,uint32_t*n,const UCModelMembershipLight*l,float score){
    uint32_t at=0;while(at<*n&&score>v[at].score)at++;
    memmove(v+at+1,v+at,(*n-at)*sizeof(*v));v[at].light=l;v[at].score=score;(*n)++;
}
static uint32_t lower(const UCModelLightMembership*s,const UCModelMembershipLight*l){
    uint32_t low=0,high=s->member_count;uintptr_t key=(uintptr_t)l;
    while(low<high){uint32_t middle=low+(high-low)/2;if((uintptr_t)s->members[middle]<key)low=middle+1;else high=middle;}return low;
}
static void insert_member(UCModelLightMembership*s,const UCModelMembershipLight*l){
    uint32_t at=lower(s,l);if(at<s->member_count&&s->members[at]==l)return;
    memmove(s->members+at+1,s->members+at,(s->member_count-at)*sizeof(*s->members));s->members[at]=l;s->member_count++;
}
int uc_model_light_membership_update(UCModelLightMembership*s,const UCModelLightMembershipView*v){
    if(!s||!v)return 0;if(s->generation==v->generation)return 1;
    if(v->flags&4){s->generation=v->generation;return 1;}
    if(!arithmetic()||v->indexed>1||v->disabled>1||!finite_values(v->world,16)||!v->bounds||!finite_values((const float*)v->bounds,12))return 0;
    UCModelBounds b;transform(v->bounds,v->world,&b);if(!finite_values((const float*)&b,12))return 0;
    float d[3];for(unsigned i=0;i<3;i++)d[i]=b.center[i]-s->center[i];float distance=(d[2]*d[2]+d[1]*d[1])+d[0]*d[0];if(!isfinite(distance))return 0;
    int moved=distance>75.f;
    if(!v->disabled){
        if(v->light_count>s->capacity||(v->light_count&&!v->lights)||(v->indexed&&(!v->model||v->model->batch_count>100000||(v->model->batch_count&&!v->model->batches))))return 0;
        for(uint32_t i=0;i<v->light_count;i++){
            const UCModelMembershipLight*l=v->lights[i];int intersects;float score;
            if(!l||!l->state||!l->geometry||!finite_values(l->local_center,4)||!uc_model_light_intersection(l->geometry,&b,&intersects)||!uc_model_light_score(l->geometry,v->world+12,&score))return 0;
            if(!v->indexed&&moved&&l->state->enabled&&intersects&&l->index>=0){float sum=(l->local_center[2]+l->local_center[1])+l->local_center[0];if(!isfinite(sum)||(sum==0&&!v->world_query))return 0;}
        }
    }
    s->generation=v->generation;if(moved){memcpy(s->center,b.center,12);s->retained_count=0;}s->member_count=0;
    if(v->disabled)return 1;
    uint32_t temporary=0;
    for(uint32_t i=0;i<v->light_count;i++){
        const UCModelMembershipLight*l=v->lights[i];if(!l->state->enabled)continue;int intersects;float score;int dynamic=0,retain=0;
        if(v->indexed){
            if(l->index<0){uc_model_light_intersection(l->geometry,&b,&intersects);if(intersects)dynamic=1;}
            if(!dynamic&&moved)for(uint32_t j=0;j<v->model->batch_count;j++)if((int32_t)v->model->batches[j].light_index==l->index){retain=1;break;}
        }else{
            uc_model_light_intersection(l->geometry,&b,&intersects);if(!intersects)continue;
            if(l->index<0)dynamic=1;
            else if(moved){float sum=(l->local_center[2]+l->local_center[1])+l->local_center[0];retain=sum!=0||!v->world_query(v->context,l,b.center);}
        }
        if(!dynamic&&!retain)continue;
        uc_model_light_score(l->geometry,v->world+12,&score);if(!l->state->shadows)score+=1000000000.f;
        if(dynamic)insert_score(s->temporary,&temporary,l,score);else insert_score(s->retained,&s->retained_count,l,score);
    }
    for(uint32_t i=0;i<s->retained_count;i++){insert_member(s,s->retained[i].light);if(!v->indexed&&i>v->static_budget)break;}
    for(uint32_t i=0;i<temporary;i++)insert_member(s,s->temporary[i].light);return 1;
}
int uc_model_light_membership_contains(const UCModelLightMembership*s,const UCModelMembershipLight*l){if(!s||!l)return 0;uint32_t at=lower(s,l);return at<s->member_count&&s->members[at]==l;}
const UCModelMembershipLight *const *uc_model_light_membership_members(const UCModelLightMembership*s,uint32_t*n){if(!s||!n)return NULL;*n=s->member_count;return s->members;}
const UCModelLightScore *uc_model_light_membership_retained(const UCModelLightMembership*s,uint32_t*n){if(!s||!n)return NULL;*n=s->retained_count;return s->retained;}
uint32_t uc_model_light_membership_generation(const UCModelLightMembership*s){return s?s->generation:0;}
const float *uc_model_light_membership_center(const UCModelLightMembership*s){return s?s->center:NULL;}
