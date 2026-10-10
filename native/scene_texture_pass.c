#include "scene_texture_pass.h"
#include <math.h>
static void advance(UCSceneTextureCursor *c){
    while(c->current<c->count&&((uint16_t)c->records[c->current].flags&c->mask)!=c->expected)c->current++;
}
int uc_scene_texture_pass(UCSceneTextureCursor *c,UCSceneModelPass *p,int present,
    float exposure,uint8_t disabled0,uint8_t disabled1,const UCSceneTexturePassHooks *h,void *ctx){
    float rgba[4]={1,1,1,1};
    if(!c||c->count>65536||c->current>c->count||(c->count&&!c->records))return 0;
    if(c->current==c->count)return 1;
    if(!p||!h||!h->render_state||!h->constant119||!h->world||!h->receiver)return 0;
    if(present){
        if(!isfinite(exposure)||exposure==0)return 0;
        rgba[0]=rgba[1]=rgba[2]=(float)(1.0/(double)exposure);
        if(!isfinite(rgba[0]))return 0;
    }
    if(!disabled0&&!disabled1)for(uint32_t i=0;i<c->count;i++)if(!c->records[i].model)return 0;
    h->render_state(ctx,0x4035C,0);h->constant119(ctx,rgba);
    if(!disabled0&&!disabled1){
        p->mode=2;h->render_state(ctx,0x40344,0x306);h->render_state(ctx,0x40348,0);
        c->current=0;advance(c);
        while(c->current<c->count){
            const UCSceneModelRecord *r=c->records+c->current;
            h->world(ctx,r->model);p->partition=r->partition;h->receiver(ctx,r->model,p);
            c->current++;advance(c);
        }
    }
    h->render_state(ctx,0x40304,0);h->render_state(ctx,0x40300,0);return 1;
}
