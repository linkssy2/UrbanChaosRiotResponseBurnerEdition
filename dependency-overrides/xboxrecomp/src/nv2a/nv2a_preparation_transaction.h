#ifndef NV2A_PREPARATION_TRANSACTION_H
#define NV2A_PREPARATION_TRANSACTION_H
/* Include after NvDrawStateKey/compiler and all PGRAPH fields are available.
 * Synchronous value preparation only. No resource, geometry, constant, target,
 * viewport or D3D11 binding is retained here. Every phase still executes in
 * the original order and may decline independently after an intervening write. */
#if UCRR_PREPARATION_TRANSACTION
#define NV_TX_WAYS 64
typedef struct NvPreparationSource {
    NvDrawStateKey prefix;
    uint32_t material[4][3],raw[37];
} NvPreparationSource;
typedef struct NvPreparationEntry {
    NvPreparationSource source;
    uint32_t hash;
    uint64_t serial,age;
    unsigned prefix_valid,material_valid,raw_valid;
    uint32_t levels[4];
    D3D8DrawStateRecord prefix;
    D3D8MaterialRecipe material;
    D3D8CombinerTransaction raw;
} NvPreparationEntry;
typedef struct NvPreparationHandle {
    NvPreparationEntry *entry;
    uint64_t serial,epoch;
} NvPreparationHandle;
static struct {
    NvPreparationSource source;
    unsigned source_valid,last;
    uint64_t serial,age;
    NvPreparationEntry entries[NV_TX_WAYS];
} nv_tx_cache;

static void nv_tx_capture_prefix(NvDrawStateKey *key){
    const uint32_t *m=g_pg.trace_methods;
    key->depth=g_pg.depth_test;key->depth_func=m[0x354/4];key->depth_mask=m[0x35C/4];
    key->stencil_enabled=nv_option_stencil_state();
    static const unsigned offsets[]={0x32C,0x364,0x368,0x36C,0x360,0x370,0x374,0x378};
    for(unsigned i=0;i<8;++i)key->stencil[i]=m[offsets[i]/4];
    key->cull_enabled=g_pg.cull_enable;key->cull_face=m[0x39C/4];key->front_face=m[0x3A0/4];
    key->fog=m[0x2A8/4];key->blend=g_pg.blend_enable;key->src=g_pg.blend_sfactor;
    key->dst=g_pg.blend_dfactor;key->alpha=g_pg.alpha_test;key->alpha_func=g_pg.alpha_func;
    key->alpha_ref=g_pg.alpha_ref;key->color_mask=g_pg.color_mask;
}
static void nv_tx_capture_material(uint32_t key[4][3]){
    for(unsigned t=0;t<4;++t){key[t][0]=g_pg.tex[t].address;
        key[t][1]=g_pg.tex[t].filter;key[t][2]=g_pg.tex[t].control0;}
}
static void nv_tx_capture_raw(uint32_t key[37]){
    const uint32_t *m=g_pg.trace_methods;
    static const unsigned spans[]={0x260,0xAC0,0xAA0,0x1E40};
    for(unsigned i=0;i<4;++i)memcpy(key+8*i,m+spans[i]/4,32);
    static const unsigned tail[]={0x1E60,0x288,0x28C,0x1E70,0x1E78};
    for(unsigned i=0;i<5;++i)key[32+i]=m[tail[i]/4];
}
static uint32_t nv_tx_hash(const NvPreparationSource *source){
    const unsigned char *p=(const unsigned char*)source;uint32_t h=2166136261u;
    /* Word mixing is only an index hint; the complete exact tuple verifies a
     * candidate. No byte-aliasing or unaligned uint32_t source access. */
    for(unsigned i=0;i<sizeof(*source);i+=4){uint32_t v;memcpy(&v,p+i,4);h=(h^v)*16777619u;}
    return h;
}
static NvPreparationHandle nv_tx_begin(void){
    NvPreparationHandle result={0};
    if(!nv_tx_enabled())return result;
    unsigned dirty=nv_tx_cache.source_valid?nv_tx_dirty.dirty:NV_TX_ALL,changed=!nv_tx_cache.source_valid;
    if(dirty&NV_TX_PREFIX){NvDrawStateKey value;nv_tx_capture_prefix(&value);
        if(!nv_tx_cache.source_valid||memcmp(&value,&nv_tx_cache.source.prefix,sizeof(value))){
            nv_tx_cache.source.prefix=value;changed=1;}}
    if(dirty&NV_TX_MATERIAL){uint32_t value[4][3];nv_tx_capture_material(value);
        if(!nv_tx_cache.source_valid||memcmp(value,nv_tx_cache.source.material,sizeof(value))){
            memcpy(nv_tx_cache.source.material,value,sizeof(value));changed=1;}}
    if(dirty&NV_TX_RAW){uint32_t value[37];nv_tx_capture_raw(value);
        if(!nv_tx_cache.source_valid||memcmp(value,nv_tx_cache.source.raw,sizeof(value))){
            memcpy(nv_tx_cache.source.raw,value,sizeof(value));changed=1;}}
    nv_tx_dirty.dirty=0;nv_tx_cache.source_valid=1;
    NvPreparationEntry *entry=&nv_tx_cache.entries[nv_tx_cache.last];
    if(changed||!entry->serial){
        uint32_t hash=nv_tx_hash(&nv_tx_cache.source);uint64_t oldest=UINT64_MAX;
        unsigned found=NV_TX_WAYS,victim=0;
        for(unsigned i=0;i<NV_TX_WAYS;++i){NvPreparationEntry *e=&nv_tx_cache.entries[i];
            if(e->serial&&e->hash==hash&&!memcmp(&e->source,&nv_tx_cache.source,sizeof(e->source))){found=i;break;}
            if(e->age<oldest){oldest=e->age;victim=i;}}
        if(found==NV_TX_WAYS){found=victim;entry=&nv_tx_cache.entries[found];
            entry->source=nv_tx_cache.source;entry->hash=hash;
            entry->serial=++nv_tx_cache.serial;if(!entry->serial)entry->serial=++nv_tx_cache.serial;
            entry->prefix_valid=entry->material_valid=entry->raw_valid=0;
        }
        nv_tx_cache.last=found;entry=&nv_tx_cache.entries[found];
    }
    entry->age=++nv_tx_cache.age;
    result.entry=entry;result.serial=entry->serial;result.epoch=nv_tx_dirty.epoch;
    return result;
}
static NvPreparationEntry *nv_tx_entry(NvPreparationHandle h){
    return h.entry&&h.serial==h.entry->serial&&h.epoch==nv_tx_dirty.epoch?h.entry:NULL;
}
static const D3D8DrawStateRecord *nv_tx_prefix(NvPreparationHandle h){
    NvPreparationEntry *e=nv_tx_entry(h);if(!e)return NULL;
    if(!e->prefix_valid){nv_draw_state_compile(&e->prefix,&e->source.prefix);e->prefix_valid=1;}
    return &e->prefix;
}
static const D3D8MaterialRecipe *nv_tx_material(NvPreparationHandle h,const uint32_t levels[4],unsigned *compiled){
    NvPreparationEntry *e=nv_tx_entry(h);*compiled=0;if(!e)return NULL;
    if(!e->material_valid||memcmp(e->levels,levels,sizeof(e->levels))){
        NvMaterialKey key;
        for(unsigned t=0;t<4;++t){key.stage[t].address=e->source.material[t][0];
            key.stage[t].filter=e->source.material[t][1];key.stage[t].control=e->source.material[t][2];
            key.stage[t].levels=levels[t];}
        nv_material_compile(&e->material,&key);memcpy(e->levels,levels,sizeof(e->levels));
        e->material_valid=1;*compiled=1;
    }
    return &e->material;
}
/* Return one only when the transaction handled this raw activation. An
 * unsupported compile does not populate a negative cache: caller retries the
 * original raw path now and every subsequent draw. Resource failure remains
 * outside this function, after activation, with the original NULL unbind. */
static int nv_tx_raw(NvPreparationHandle h,const uint32_t *methods,BOOL *active){
    NvPreparationEntry *e=nv_tx_entry(h);if(!e)return 0;
    if(!e->raw_valid){
        if(!d3d8_combiners_compile_transaction(methods,&e->raw))return 0;
        e->raw_valid=1;
    }
    uint32_t colors[18];memcpy(colors,methods+0xA60/4,32);memcpy(colors+8,methods+0xA80/4,32);
    colors[16]=methods[0x1E20/4];colors[17]=methods[0x1E24/4];
    *active=d3d8_combiners_activate_transaction(&e->raw,e->serial,colors);return 1;
}
#else
typedef struct NvPreparationHandle {unsigned unused;} NvPreparationHandle;
#define nv_tx_begin() ((NvPreparationHandle){0})
#define nv_tx_prefix(h) NULL
#define nv_tx_material(h,l,c) NULL
#define nv_tx_raw(h,m,a) 0
#endif
#endif
