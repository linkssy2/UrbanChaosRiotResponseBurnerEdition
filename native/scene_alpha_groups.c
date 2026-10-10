#include "scene_alpha_groups.h"

static UCSceneTextureCursor range(const UCSceneModelRecord *r,uint32_t n,uint16_t mask)
{
    UCSceneTextureCursor c={r,n,0,mask,mask};
    while(c.current<n && ((uint16_t)r[c.current].flags&mask)!=mask)++c.current;
    return c;
}

int uc_scene_alpha_groups(const UCSceneModelRecord *r,uint32_t n,
    UCSceneModelPass *pass,const void *lights,const UCSceneAlphaGates *g,
    const UCSceneAlphaHooks *h,void *ctx)
{
    uint32_t first=0;
    if(!g || !g->disabled)return 0;
    if(*g->disabled)return 1;
    if(n>65536 || (n&&!r) || !pass || !lights || !g->passes || !g->ambient ||
       !g->texture || !g->owner || !g->active || !h || !h->ambient ||
       !h->light || !h->texture)return 0;
    *g->active=1;
    while(first<n){
        uint32_t end;
        UCSceneTextureCursor c;
        if(!(r[first].flags&0x100)){++first;continue;}
        end=first+1;
        if(r[first].flags&0x200)
            while(end<n && (r[end].flags&0x300)!=0x100)++end;
        if(*g->passes && *g->ambient){
            c=range(r+first,end-first,0x101);
            h->ambient(ctx,&c,pass,1);
        }
        if(*g->owner){
            c=range(r+first,end-first,0x108);
            h->light(ctx,&c,pass,lights,0);
        }
        if(*g->texture){
            c=range(r+first,end-first,0x110);
            h->texture(ctx,&c,pass);
        }
        first=end;
    }
    *g->active=0;
    return 1;
}
