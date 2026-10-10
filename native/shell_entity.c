#include "shell_entity.h"
typedef struct Construction {const UCShellEntityConstruction *c;int failure;} Construction;
static int preflight(void *p,const UCShellState *s,const UCShellProperties *properties){
    const UCShellEntityConstruction *c=((Construction*)p)->c;const UCEntityFreshView *v=c->view;
    (void)properties;
    return v&&c->entities&&c->entities->registration&&c->entities->registration->registry&&
        c->entities->resolve&&c->tag_world&&c->current_names&&(c->tag==4||c->tag==5)&&
        v->transform==s->transform&&(const void*)v->id==&s->entity_id&&
        (const void*)v->target.parent_id==&s->parent_id&&v->model74==&s->model;
}
static int base(void *p,UCShellState *s){
    const UCShellEntityConstruction *c=((Construction*)p)->c;(void)s;
    return uc_entity_fresh_construct_shell(c->entities,c->view,c->tag,c->owner,c->tag_world);
}
static const float *parent(void *p,uint32_t id){
    const UCShellEntityConstruction *c=((Construction*)p)->c;
    UCEntityEventTarget *t=uc_entity_event_lookup(c->entities->registration->registry,(int32_t)id);
    const UCEntityFreshView *v=t?c->entities->resolve(c->entities->owner,t):NULL;
    return v?v->transform+12:NULL;
}
static void group(void *p,UCShellState *s,int add){
    Construction *x=p;UCEntityNames *names=x->c->current_names(x->c->owner);(void)add;
    x->failure=uc_entity_names_bind(names,"PLAYER_BULLET_SHELLS",(int32_t)s->entity_id);
}
int uc_shell_entity_construct(UCShellState *s,const UCShellProperties *p,UCGameRandom *r,
    const UCShellEntityConstruction *c){
    if(!c)return 1;
    Construction x={c,0};UCShellServices services={0};
    services.context=&x;services.preflight=preflight;services.construct_base=base;
    services.parent_position=parent;services.group=group;
    int result=uc_shell_construct(s,p,r,&services);return result?result:x.failure?2:0;
}
