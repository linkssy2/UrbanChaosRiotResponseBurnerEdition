#include "glass_registry.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
struct UCGlassRegistry {UCGlassRender **items;uint32_t count,capacity;float matrix[16];uint8_t enabled,busy,named;char name[16];};
static uint32_t lower(const UCGlassRegistry *o,const void *source){uint32_t i=0;while(i<o->count&&(uintptr_t)o->items[i]->source<(uintptr_t)source)i++;return i;}
int uc_glass_registry_create(uint32_t n,const float identity[16],UCGlassRegistry **out){
    if(!n||n>4096||!identity||!out)return 1;
    UCGlassRegistry *o=calloc(1,sizeof *o);if(!o)return 2;
    o->items=malloc(n*sizeof *o->items);if(!o->items){free(o);return 2;}
    o->capacity=n;o->enabled=1;memcpy(o->matrix,identity,64);*out=o;return 0;
}
int uc_glass_registry_add(UCGlassRegistry *o,const void *source,UCMaterialResources *resources,const char *shader){
    if(!o||o->busy||!source||!resources||!shader||o->count==o->capacity)return 1;
    uint32_t i=lower(o,source);if(i<o->count&&o->items[i]->source==source)return 1;
    const UCMaterialResource *material=NULL;if(uc_material_resources_get(resources,shader,&material))return 2;
    UCGlassRender *r=malloc(sizeof *r);if(!r)return 2;*r=(UCGlassRender){source,material,NULL,0};
    memmove(o->items+i+1,o->items+i,(o->count-i)*sizeof *o->items);o->items[i]=r;o->count++;return 0;
}
int uc_glass_registry_remove(UCGlassRegistry *o,const void *source){
    if(!o||o->busy)return 1;uint32_t i=lower(o,source);if(i==o->count||o->items[i]->source!=source)return 0;
    free(o->items[i]);memmove(o->items+i,o->items+i+1,(o->count-i-1)*sizeof *o->items);o->count--;return 0;
}
int uc_glass_registry_draw(UCGlassRegistry *o,uint32_t mode,const float identity[16],const UCGlassRenderHooks *h,void *context){
    if(!o||o->busy)return 1;if(mode!=2||!o->count)return 0;
    o->busy=1;for(uint32_t i=0;i<o->count;i++)if(uc_glass_render(o->items[i],identity,h,context)){o->busy=0;return 2;}
    o->busy=0;return 0;
}
int uc_glass_registry_close(UCGlassRegistry *o,UCSceneModelGroup *(*primary)(void *),void *context){
    if(!o||o->busy||!primary)return 1;o->busy=1;
    for(uint32_t i=0;i<o->count;i++)free(o->items[i]);
    uint32_t removed;int result=uc_scene_model_group_remove(primary(context),o,&removed);
    free(o->items);free(o);return result?2:0;
}
uint32_t uc_glass_registry_count(const UCGlassRegistry *o){return o?o->count:0;}
const UCGlassRender *uc_glass_registry_at(const UCGlassRegistry *o,uint32_t i){return o&&!o->busy&&i<o->count?o->items[i]:NULL;}
const float *uc_glass_registry_matrix(const UCGlassRegistry *o){return o?o->matrix:NULL;}
uint8_t uc_glass_registry_enabled(const UCGlassRegistry *o){return o?o->enabled:0;}
int uc_glass_registry_set_matrix(UCGlassRegistry*o,const float*m){if(!o||o->busy||!m)return 1;memcpy(o->matrix,m,64);return 0;}
int uc_glass_registry_set_name(UCGlassRegistry*o,const char*s){if(!o||o->busy||!s)return 1;size_t n=0;while(n<16&&s[n])n++;memcpy(o->name,s,n);memset(o->name+n,0,16-n);o->named=1;return 0;}
const char*uc_glass_registry_name(const UCGlassRegistry*o){return o&&o->named?o->name:NULL;}
