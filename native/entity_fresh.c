#include "entity_fresh.h"
#include "entity_authored.h"
#include "shot_lightflash.h"
#include "euler_rotation.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
#include <xmmintrin.h>

typedef struct UCEntityFreshSentinel {
    struct UCEntityFreshSentinel *next,*previous;
} UCEntityFreshSentinel;
struct UCEntityFreshAux {
    UCEntityFreshSentinel *sentinel;
    uint32_t count,wordC,word10;
    UCEntityFreshMemory memory;
};
UCEntityFreshAux *uc_entity_fresh_aux_create(const UCEntityFreshMemory *m) {
    UCEntityFreshAux *p;
    UCEntityFreshSentinel *s;
    if(!m||!m->allocate||!m->release)return NULL;
    p=m->allocate(m->owner,sizeof *p);if(!p)return NULL;
    s=m->allocate(m->owner,sizeof *s);
    if(!s){m->release(m->owner,p);return NULL;}
    s->next=s;s->previous=s;p->sentinel=s;
    p->count=p->wordC=p->word10=0;p->memory=*m;return p;
}
int uc_entity_fresh_aux_empty(const UCEntityFreshAux *p) {
    return p&&p->sentinel&&p->count==0&&p->sentinel->next==p->sentinel&&
        p->sentinel->previous==p->sentinel;
}
int uc_entity_fresh_aux_clear(UCEntityFreshAux *p) {
    if(!uc_entity_fresh_aux_empty(p))return 1;
    p->sentinel->next=p->sentinel;p->sentinel->previous=p->sentinel;p->count=0;
    return 0;
}
int uc_entity_fresh_aux_destroy(UCEntityFreshAux *p) {
    UCEntityFreshMemory m;
    if(!p)return 0;
    if(uc_entity_fresh_aux_clear(p))return 1;
    m=p->memory;m.release(m.owner,p->sentinel);p->sentinel=NULL;
    m.release(m.owner,p);return 0;
}
static const UCEntityFreshView *parent(const UCEntityFreshContext *c,const UCEntityFreshView *v) {
    UCEntityEventTarget *t=uc_entity_event_lookup(c->registration->registry,*v->target.parent_id);
    return t?c->resolve(c->owner,t):NULL;
}
static void multiply(const float *a,const float *b,float *out) {
    unsigned r,k;
    for(r=0;r<4;r++)for(k=0;k<4;k++)
        out[r*4+k]=(b[k]*a[r*4]+b[4+k]*a[r*4+1])+
            (b[8+k]*a[r*4+2]+b[12+k]*a[r*4+3]);
}
static int shell_place(const float *tag,float *out) {
    static const float zero_rotation[16]={1,-0.0f,0,0,0,1,-0.0f,0,0,0,1,0,0,0,0,1};
    float first[16],second[16],value[16];unsigned i;
    for(i=0;i<16;i++)if(!isfinite(tag[i])||fabsf(tag[i])>1e6f)return 1;
    multiply(zero_rotation,zero_rotation,first);
    multiply(first,zero_rotation,second);multiply(second,tag,value);
    for(i=0;i<12;i++)out[i]=value[i];
    for(i=0;i<3;i++)out[12+i]=(value[i]*0.0f+value[4+i]*0.0f)+
        (value[8+i]*0.0f+value[12+i]);
    out[15]=1;return 0;
}
static int construct(const UCEntityFreshContext *c,const UCEntityFreshView *v,int placed_mode,
    uint32_t tag,void *tag_owner,UCEntityFreshTagWorld tag_world,int base_only,
    const void *const *source_context,const UCEntityAuthored *const *authored,const float *matrix) {
    UCEntityRegistrationResult registered;
    UCShotLightFlashPlacement placed;
    const UCEntityFreshView *p;
    UCEntityFreshActor *a;
    uint32_t old,next;
    unsigned i;
    if(!c||!v||!c->registration||!c->registration->registry||
        !c->registration->next_id||!c->registration->live||!c->registration->creation||
        !c->registration->flags||!c->registration->insert_record||
        *c->registration->next_id>=0x7FFFFBFFu||!c->definition||
        !c->resolve||!v->target.identity||
        !v->target.flags||!v->target.parent_id||!v->transform||!v->source_index||
        !v->id||!v->flags60||!v->definition70||!v->model74||!v->helper78||
        (authored&&(!*authored||!(*authored)->definition||((*authored)->condition_count&&!(*authored)->conditions)))||
        (base_only!=1&&(!c->group_counter||!c->memory.allocate||!c->memory.release||
        !v->aux84||*v->aux84||!v->actor||fegetround()!=FE_TONEAREST||
        (_mm_getcsr()&0xE040u)))||(placed_mode==4&&(!tag_world||(tag!=4&&tag!=5)))||
        ((placed_mode==3||placed_mode==5||placed_mode==6)&&!matrix)||
        (placed_mode==3&&(fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u))))return 1;
    if(placed_mode==3)for(i=0;i<3;i++)if(!isfinite(matrix[i])||fabsf(matrix[i])>1e6f)return 1;
    if(placed_mode==6)for(i=0;i<6;i++)if(!isfinite(matrix[i])||fabsf(matrix[i])>1e6f)return 1;
    *v->target.flags=8;*v->model74=NULL;*v->flags60=0;
    for(i=0;i<16;i++)v->transform[i]=(i%5)==0?1.0f:0.0f;
    *v->helper78=NULL;
    if(uc_entity_register(c->registration,&v->target,&registered))return 2;
    *v->id=registered.id;
    for(i=0;i<3;i++)v->target.identity->listeners[i]=NULL;
    if(source_context&&*source_context)return 2;
    *v->definition70=*c->definition;
    if(authored){
        const UCEntityAuthored *d=*authored;
        if(!d||!d->definition)return 2;
        *v->target.flags|=0x1000;*v->source_index=d->source_index;
        memcpy(v->transform,d->definition->transform,64);
        if(d->asset_present){
            uint32_t flags=d->definition->flags;
            if(flags&1)*v->flags60|=2;
            if(flags&4)*v->flags60|=0x10;
            *v->target.flags|=0x20000;
        }
        *v->definition70=d->definition->class_name;
    }else if(placed_mode==6){
        float z[16],x[16],y[16],zx[16];
        uc_euler_rotation(0,0,matrix[5]*.01745329238474369f,z);
        uc_euler_rotation(matrix[3]*.01745329238474369f,0,0,x);
        uc_euler_rotation(0,matrix[4]*.01745329238474369f,0,y);
        multiply(z,x,zx);multiply(zx,y,v->transform);
        memcpy(v->transform+12,matrix,12);v->transform[15]=1;
        *v->target.flags=0x400;*v->source_index=-1;
    }else if(placed_mode==3){
        /*20A88E, descriptor positive-zero Euler. Rotation products resolve to
         * identity; translation is copied from the live descriptor after record
         * insertion. No parent's transform contributes to absolute placement. */
        for(i=0;i<12;i++)v->transform[i]=(i%5)==0?1.0f:0.0f;
        memcpy(v->transform+12,matrix,12);v->transform[15]=1;
        *v->target.flags=0x400;*v->source_index=-1;
    }else if(placed_mode==5){
        /*20B83F/20A5E0 copies the descriptor's four rows, without a parent
         * matrix lookup or arithmetic. The later group lookup still occurs. */
        memcpy(v->transform,matrix,64);*v->target.flags=0x400;*v->source_index=-1;
    }else if(placed_mode){
        p=parent(c,v);
        if(!p||p->target.identity!=uc_entity_event_lookup(c->registration->registry,*v->target.parent_id)||
            !p->transform)return 2;
        if(placed_mode==1){
            memcpy(v->transform,p->transform,64);
            *v->target.flags=0x400;*v->source_index=-1;
        }else if(placed_mode==4){
            float world[16];
            if(tag_world(tag_owner,p->target.identity,tag,world)||shell_place(world,v->transform))return 2;
            *v->target.flags=0x400;*v->source_index=-1;
        }else{
            if(matrix?uc_shot_lightflash_place_offset(p->transform,matrix,&placed):uc_shot_lightflash_place(p->transform,&placed))return 2;
            memcpy(v->transform,placed.transform,sizeof placed.transform);
            *v->target.flags=placed.flags;*v->source_index=(int32_t)placed.source_index;
        }
    }else{
        for(i=0;i<12;i++)v->transform[i]=(i%5)==0?1.0f:0.0f;
        *v->target.flags=0x400;*v->source_index=-1;
    }
    if(base_only==1)return 0;
    *v->aux84=uc_entity_fresh_aux_create(&c->memory);
    if(!*v->aux84)return 2;
    p=authored?NULL:parent(c,v);a=v->actor;
    if(p&&(p->target.identity!=uc_entity_event_lookup(c->registration->registry,*v->target.parent_id)||
        !p->target.flags))return 2;
    /*2185B2 first checks the parent's actor flag. Base-only behavior roots
     * legitimately parent scripted actors but have no health/group fields. */
    if(p&&(*p->target.flags&0x08000000u)&&!p->model74)return 2;
    if(p&&(*p->target.flags&0x08000000u)&&*p->model74){
        if(!p->actor)return 2;
        a->group94=p->actor->group94;
    }
    else {old=*c->group_counter;next=old+1u;*c->group_counter=next;
        if((int32_t)next>=0x100000)*c->group_counter=1;
        a->group94=(uint16_t)old;}
    a->word98=0xFFFF;a->maximumA0=0;a->health9C=0;a->valueA4=0;a->word90=16;
    if(uc_entity_fresh_aux_clear(*v->aux84))return 2;
    a->distance88=5000.0f;a->distance8C=(float)sqrt(5000.0);
    *v->target.flags|=0x08000000u;
    if(base_only==2)return 0; /*218A10 stops at218550, before scripted actor tail. */
    a->wordC4=0;a->idB0=-1;a->idB4=-1;a->handleBC=NULL;a->audioC0=NULL;
    a->scriptC8=NULL;a->flagCC=0;a->flagCD=0;a->flagCE=0;
    if(authored){
        const UCEntityAuthored *d=*authored;
        if(!d||!d->definition||(d->condition_count&&!d->conditions))return 2;
        if(d->condition_count){a->flagCE=1;
            for(uint32_t j=0;j<d->condition_count;j++)d->conditions[j].used=0;
        }
    }
    return 0;
}
int uc_entity_fresh_construct(const UCEntityFreshContext *c,const UCEntityFreshView *v) {
    return construct(c,v,2,0,NULL,NULL,0,NULL,NULL,NULL);
}
int uc_entity_fresh_construct_lightflash(const UCEntityFreshContext *c,const UCEntityFreshView *v,const float *position) {
    if(!position)return 1;return construct(c,v,2,0,NULL,NULL,0,NULL,NULL,position);
}
int uc_entity_fresh_construct_unplaced(const UCEntityFreshContext *c,const UCEntityFreshView *v) {
    return construct(c,v,0,0,NULL,NULL,0,NULL,NULL,NULL);
}
int uc_entity_base_construct_unplaced(const UCEntityFreshContext *c,const UCEntityFreshView *v) {
    return construct(c,v,0,0,NULL,NULL,1,NULL,NULL,NULL);
}
int uc_entity_base_construct_position(const UCEntityFreshContext *c,const UCEntityFreshView *v,const float position[3]) {
    return construct(c,v,3,0,NULL,NULL,1,NULL,NULL,position);
}
int uc_entity_fresh_construct_parent(const UCEntityFreshContext *c,const UCEntityFreshView *v) {
    return construct(c,v,1,0,NULL,NULL,0,NULL,NULL,NULL);
}
int uc_entity_fresh_construct_shell(const UCEntityFreshContext *c,const UCEntityFreshView *v,
    uint32_t tag,void *owner,UCEntityFreshTagWorld tag_world) {
    return construct(c,v,4,tag,owner,tag_world,0,NULL,NULL,NULL);
}
int uc_entity_base_construct_parent(const UCEntityFreshContext *c,const UCEntityFreshView *v,
    const void *const *source_context) {
    if(!source_context||*source_context)return 1;
    return construct(c,v,1,0,NULL,NULL,1,source_context,NULL,NULL);
}

int uc_entity_fresh_construct_authored(const UCEntityFreshContext *c,
    const UCEntityFreshView *v,const UCEntityAuthored *const *active){
    if(!active)return 1;
    return construct(c,v,0,0,NULL,NULL,0,NULL,active,NULL);
}
int uc_entity_base_construct_authored(const UCEntityFreshContext *c,
    const UCEntityFreshView *v,const UCEntityAuthored *const *active){
    if(!active)return 1;
    return construct(c,v,0,0,NULL,NULL,1,NULL,active,NULL);
}
int uc_entity_health_construct_authored(const UCEntityFreshContext *c,
    const UCEntityFreshView *v,const UCEntityAuthored *const *active){
    if(!active)return 1;
    return construct(c,v,0,0,NULL,NULL,2,NULL,active,NULL);
}
int uc_entity_health_construct_matrix(const UCEntityFreshContext *c,
    const UCEntityFreshView *v,const float matrix[16]){
    return construct(c,v,5,0,NULL,NULL,2,NULL,NULL,matrix);
}
int uc_entity_fresh_construct_matrix(const UCEntityFreshContext *c,
    const UCEntityFreshView *v,const float matrix[16]){
    return construct(c,v,5,0,NULL,NULL,0,NULL,NULL,matrix);
}
int uc_entity_fresh_construct_pose(const UCEntityFreshContext*c,const UCEntityFreshView*v,const float pose[6]){
    return construct(c,v,6,0,NULL,NULL,0,NULL,NULL,pose);
}
