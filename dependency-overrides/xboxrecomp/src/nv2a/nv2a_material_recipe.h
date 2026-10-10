#ifndef NV2A_MATERIAL_RECIPE_H
#define NV2A_MATERIAL_RECIPE_H
#include <string.h>
#include "../d3d/d3d8_material_recipe.h"
#include "nv2a_texture_address.h"
#include "nv2a_texture_filter.h"
typedef struct NvMaterialStageKey {
    uint32_t address,filter,control,levels;
} NvMaterialStageKey;
typedef struct NvMaterialKey { NvMaterialStageKey stage[4]; } NvMaterialKey;
typedef struct NvMaterialEntry {
    unsigned valid;
    NvMaterialKey key;
    D3D8MaterialRecipe recipe;
} NvMaterialEntry;
typedef struct NvMaterialCache {
    NvMaterialEntry entries[32];
    unsigned last;
} NvMaterialCache;

static void nv_material_compile(D3D8MaterialRecipe *out,const NvMaterialKey *key)
{
    memset(out,0,sizeof(*out));
    for(unsigned t=0;t<4;++t){
        const NvMaterialStageKey *k=&key->stage[t];
        D3D8MaterialSampler *s=&out->sampler[t];
        unsigned u=nv_texture_address(k->address,0),v=nv_texture_address(k->address,1);
        unsigned w=nv_texture_address(k->address,2);
        unsigned min=nv_texture_spatial_filter(k->filter,0),mag=nv_texture_spatial_filter(k->filter,1);
        s->u=u?u:3;s->v=v?v:3;s->w=w?w:1;
        s->min_filter=min?min:1;s->mag_filter=mag?mag:1;
        float bias=nv_texture_lod_bias(k->filter);memcpy(&s->lod_bits,&bias,4);
        if(!u||!v)out->unsupported|=1u<<(t*3);
        if(!min||!mag)out->unsupported|=2u<<(t*3);
        unsigned mode=(k->filter>>16)&255;
        if(k->levels>1&&mode>=3&&mode<=6){
            unsigned minimum=(k->control>>18)&4095,maximum=(k->control>>6)&4095;
            if(!minimum&&maximum>=k->levels-1)s->mip_filter=mode>=5?2:1;
            else out->unsupported|=4u<<(t*3);
        }
    }
}

/* Fast last-material hit, bounded alternate-material cache, collision checked
 * by the complete key. Key includes effective uploaded mip levels, not merely
 * guest texture registers. Device reset cannot stale this value-only cache. */
static const D3D8MaterialRecipe *nv_material_prepare(NvMaterialCache *cache,
    const NvMaterialKey *key,unsigned *compiled)
{
    NvMaterialEntry *entry=&cache->entries[cache->last];
    *compiled=0;
    if(entry->valid&&!memcmp(&entry->key,key,sizeof(*key)))return &entry->recipe;
    uint32_t hash=2166136261u;
    for(unsigned t=0;t<4;++t){
        const NvMaterialStageKey *s=&key->stage[t];
        hash=(hash^s->address)*16777619u;hash=(hash^s->filter)*16777619u;
        hash=(hash^s->control)*16777619u;hash=(hash^s->levels)*16777619u;
    }
    cache->last=hash&31;entry=&cache->entries[cache->last];
    if(!entry->valid||memcmp(&entry->key,key,sizeof(*key))){
        entry->key=*key;nv_material_compile(&entry->recipe,key);entry->valid=1;*compiled=1;
    }
    return &entry->recipe;
}
#endif
