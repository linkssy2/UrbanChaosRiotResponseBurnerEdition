#include "scene_clear.h"
int uc_scene_clear(const UCSceneClearState*s,const UCSceneClearHooks*h,void*v){
 if(!s||!h||!s->owner||!s->depth||!s->texture||!s->ambient||!s->color||!s->color_mask||
    !h->state||!h->clear||!h->busy||!h->submit)return 1;
 unsigned color=*s->owner||*s->texture||*s->ambient;
 uint32_t mask=color?0x10101:0;
 h->state(v,0x40358,mask);*s->color_mask=mask;
 uint32_t argb=*s->color&0xFFFFFF;
 uint32_t flags=(*s->depth?3u:0)|(color?0xF0u:0);
 h->clear(v,flags,argb,1,0);
 if(!h->busy(v))h->submit(v);
 return 0;
}
