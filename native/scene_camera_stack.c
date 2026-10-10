#include "scene_camera_stack.h"
#include <stdlib.h>
#include <math.h>
static int valid(const UCSceneCameraStack*s){
    return s&&s->entities&&s->entities->registration&&s->entities->registration->registry&&s->entities->resolve&&s->current_viewport&&s->memory.allocate&&s->memory.release&&s->count<=s->capacity&&s->capacity<=4&&(!s->capacity||s->layers);
}
/* Actual214630 tables; authored640x480 coordinates, not scaled PC dimensions. */
static void layout(UCSceneCameraLayer*v,unsigned index,unsigned count,int horizontal){
    static const uint32_t colors[]={0xFF00FF00u,0xFFFF0000u,0xFF0000FFu,0xFF80FF80u};
    v->top=v->left=0;v->bottom=480;v->right=640;v->clear=colors[0];
    if(count==2){v->clear=index?colors[2]:colors[0];if(horizontal){v->top=index?240.f:0;v->bottom=index?480.f:240.f;}else{v->left=index?320.f:0;v->right=index?640.f:320.f;}}
    else if(count==3||count==4){v->clear=colors[index];v->top=index>=2?240.f:0;v->bottom=index>=2?480.f:240.f;v->left=index&1?320.f:0;v->right=index&1?640.f:320.f;}
}
/* WholeA5CD0 arithmetic, same concrete viewport storage and lifetime owner. */
static int rectangle(UCSceneViewport*p,const UCSceneCameraLayer*v,const UCSceneViewportMemory*m,UCSceneViewport**out){
    double values[5];if(uc_scene_viewport_values(p,values))return 1;
    float aspect=(float)values[4],ph=p->bottom-p->top,pw=p->right-p->left;
    UCSceneViewport*q=m->allocate(m->owner);*out=q;if(!q)return 0;
    float h=v->bottom-v->top,w=v->right-v->left,ratio=((w*ph)*aspect)/(h*pw);ratio=(ratio*h)/w;
    if(!isfinite(ratio)||ratio<=0)return 2;
    *q=(UCSceneViewport){p,v->top,v->left,v->bottom,v->right,ratio,1};return 0;
}
int uc_scene_camera_stack_add(UCSceneCameraStack*s,int32_t id,UCSceneCameraLayer**out){
    if(!valid(s)||!out||s->count==4)return 1;
    UCSceneCameraLayer*v=malloc(sizeof*v);if(!v)return 1;
    v->word0=0;v->entity=uc_entity_event_lookup(s->entities->registration->registry,id);v->count=0;v->head.next=v->head.previous=&v->head;
    v->viewport=NULL;v->tag=-1;v->value=.8f;
    if(v->entity){const UCEntityFreshView*e=s->entities->resolve(s->entities->owner,v->entity);if(!e||!e->flags60){free(v);return 1;}*e->flags60|=0x2000;}
    if(s->count==s->capacity){unsigned capacity=s->capacity+s->capacity/2;if(capacity<s->count+1)capacity=s->count+1;UCSceneCameraLayer**next=realloc(s->layers,capacity*sizeof*next);if(!next){free(v);return 2;}s->layers=next;s->capacity=capacity;}
    s->layers[s->count++]=v;*out=v;
    for(unsigned i=0;i<s->count;i++){
        UCSceneCameraLayer fields;layout(&fields,i,s->count,s->horizontal);UCSceneViewport*next=NULL;
        if(rectangle(s->current_viewport(s->owner),&fields,&s->memory,&next)){if(next)s->memory.release(s->memory.owner,next);return 2;}
        UCSceneCameraLayer*live=s->layers[i];if(live->viewport&&uc_scene_viewport_end(live->viewport,1,&s->memory))return 2;
        live->clear=fields.clear;live->top=fields.top;live->left=fields.left;live->bottom=fields.bottom;live->right=fields.right;live->viewport=next;
    }return 0;
}
int uc_scene_camera_stack_activate(UCSceneCameraStack*s,UCSceneCamera*c,int32_t id){
    if(!valid(s)||!c)return 1;UCSceneCameraLayer*v=NULL;UCEntityEventTarget*e=uc_entity_event_lookup(s->entities->registration->registry,id);
    if(e)for(unsigned i=0;i<s->count;i++)if(s->layers[i]->entity==e){v=s->layers[i];break;}
    if(!v){if(s->count)v=s->layers[0];else{int error=uc_scene_camera_stack_add(s,id,&v);if(error)return error;}}
    if(v->count>=65536)return 2;UCSceneCameraNode*n=malloc(sizeof*n);if(!n)return 2;
    n->next=&v->head;n->previous=v->head.previous;n->camera=c;++v->count;v->head.previous=n;n->previous->next=n;return 0;
}
int uc_scene_camera_stack_remove(UCSceneCameraStack*s,UCSceneCamera*c){
    if(!valid(s)||!c)return 1;for(unsigned i=0;i<s->count;i++){UCSceneCameraLayer*v=s->layers[i];UCSceneCameraNode*n=v->head.next;
        while(n!=&v->head){UCSceneCameraNode*next=n->next;if(n->camera==c){n->previous->next=next;next->previous=n->previous;free(n);--v->count;}n=next;}}
    return 0;
}
int uc_scene_camera_stack_current(const UCSceneCameraStack*s,uint32_t i,UCSceneCamera**out){
    if(!valid(s)||!out||(s->count&&i>=s->count))return 1;*out=s->count&&s->layers[i]->count?s->layers[i]->head.previous->camera:NULL;return 0;
}
void uc_scene_camera_stack_close(UCSceneCameraStack*s){
    if(!s)return;for(unsigned i=0;i<s->count;i++)if(s->layers[i]->viewport)uc_scene_viewport_end(s->layers[i]->viewport,1,&s->memory);
    for(unsigned i=0;i<s->count;i++){UCSceneCameraLayer*v=s->layers[i];UCSceneCameraNode*n=v->head.next;while(n!=&v->head){UCSceneCameraNode*next=n->next;free(n);n=next;}free(v);}free(s->layers);s->layers=NULL;s->count=s->capacity=0;
}
