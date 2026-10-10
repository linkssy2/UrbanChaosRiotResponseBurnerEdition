#include "point_light_draw.h"
#include <string.h>
static int selection_valid(const UCPointLightSelection *s){
    uint32_t i;
    if(!s||s->capacity>65536||s->count>s->capacity||(s->capacity&&!s->items))return 0;
    for(i=0;i<s->count;i++)if(!s->items[i]||(i&&(uintptr_t)s->items[i-1]>=(uintptr_t)s->items[i]))return 0;
    return 1;
}
int uc_point_light_collect(UCPointLightSelection *s,const UCPointLightDrawModel *m,
    uint32_t check,void *ctx,int (*visible)(void *,const void *)){
    const UCModelMembershipLight *const *members;uint32_t n,i,j,needed;
    if(!selection_valid(s)||!m||!m->membership)return 0;
    if((uint8_t)check&&m->visibility_subject&&!visible)return 0;
    members=uc_model_light_membership_members(m->membership,&n);needed=s->count;
    for(i=j=0;i<n;i++){
        while(j<s->count&&(uintptr_t)s->items[j]<(uintptr_t)members[i])j++;
        if(j==s->count||s->items[j]!=members[i])needed++;
    }
    if(needed>s->capacity)return 0;
    if((uint8_t)check&&!(m->visibility_subject&&visible(ctx,m->visibility_subject))&&!m->asset_visible)return 1;
    for(i=0;i<n;i++){
        for(j=0;j<s->count&&(uintptr_t)s->items[j]<(uintptr_t)members[i];j++){}
        if(j<s->count&&s->items[j]==members[i])continue;
        memmove(s->items+j+1,s->items+j,(s->count-j)*sizeof(*s->items));s->items[j]=members[i];s->count++;
    }
    return 1;
}
static int cursor_valid(const UCPointLightDrawCursor *c){return c&&c->count<=65536&&c->current<=c->count&&(!c->count||c->records);}
static void advance(UCPointLightDrawCursor *c){
    while(c->current<c->count&&(c->records[c->current].flags&c->mask)!=c->expected)c->current++;
}
int uc_point_light_draw_reset(UCPointLightDrawCursor *c){if(!cursor_valid(c))return 0;c->current=0;advance(c);return 1;}
int uc_point_light_draw(UCPointLightDrawCursor *c,const UCPointLightSelection *s,
    UCPointLightDrawPass *p,uint32_t *counter,uint32_t shadow_request,
    uint8_t suppress_shadows,uint8_t disabled,const UCPointLightDrawHooks *h,void *ctx){
    uint32_t i;
    if(!cursor_valid(c))return 0;
    if(c->current==c->count)return 1;
    if(!selection_valid(s)||!p||!counter||!h||!h->manager_begin||!h->render_state||!h->light_begin||!h->receiver||!h->light_end||!h->manager_end)return 0;
    for(i=0;i<s->count;i++)if(!s->items[i]->state||((uint8_t)shadow_request&&!suppress_shadows&&s->items[i]->state->shadows))return 0;
    for(i=0;i<c->count;i++)if(!c->records[i].model||!c->records[i].model->membership)return 0;
    h->manager_begin(ctx);
    h->render_state(ctx,0x4036C,0xFF);h->render_state(ctx,0x40360,0xFF);h->render_state(ctx,0x40368,0x7F);
    for(i=0;i<s->count;i++){
        const UCModelMembershipLight *light=s->items[i];
        if(!light->state->enabled||!(uint8_t)h->light_begin(ctx,light))continue;
        p->light=light;(*counter)++;
        if(!disabled){
            p->mode=3;c->current=0;advance(c);
            while(c->current<c->count){
                const UCPointLightDrawRecord *r=c->records+c->current;
                p->partition=r->partition;
                if(uc_model_light_membership_contains(r->model->membership,light))h->receiver(ctx,r->model,p);
                c->current++;advance(c);
            }
        }
        h->light_end(ctx,light);
    }
    h->manager_end(ctx);return 1;
}
