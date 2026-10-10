#ifndef NV2A_RETAINED_RESOURCES_H
#define NV2A_RETAINED_RESOURCES_H
/* Include after g_pg and the lighting/live-texture types. Certificates describe
 * the exact successful preparation, never a second address-only cache lookup.
 * Host mutation callbacks must flush BEFORE resource writes/state changes.
 * The certificate alone does not supply that host ordering contract. */
enum { NV_RETAIN_UNKNOWN, NV_RETAIN_NONE, NV_RETAIN_2D, NV_RETAIN_LIGHT };
typedef struct NvRetainedResource {
    unsigned kind;
    unsigned char binding[sizeof(g_pg.tex[0])];
    LiveTextureCacheEntry *entry;
    IDirect3DTexture8 *texture;
    ID3D11Resource *resource;
    ID3D11ShaderResourceView *view;
    const uint8_t *source,*palette,*copy;
    size_t bytes,palette_bytes;
    uint64_t host_content_revision;
    uint32_t offset,format,width,height,levels,palette_register,palette_handle;
#ifdef UCRR_RESOURCE_REVISIONS
    NvTextureRevision revision;
#endif
    NvLightTexture *light;
    uint32_t signs;
#ifdef UCRR_RAM_WATCH_SHADOW
    NvLightWatchCertificate light_certificate;
#endif
} NvRetainedResource;
static struct {
    NvRetainedResource prepared[4],held[4];
    const uint8_t *ram;
    size_t ram_size;
    const uint8_t *registers;
    size_t registers_size;
    unsigned active;
} raw_retained_resources;

static int raw_retained_2d_was_tracked(const LiveTextureCacheEntry *e){
#ifdef UCRR_RESOURCE_REVISIONS
    return e && (e->revision.source_id!=0 || e->revision.exact);
#else
    (void)e;return 0;
#endif
}
static int raw_retained_2d_tokens_clean(const LiveTextureCacheEntry *e,
    const uint8_t *source,size_t bytes,const uint8_t *palette,size_t palette_bytes){
#ifdef UCRR_RESOURCE_REVISIONS
    if(!texture_revision_enabled() || !source_copies_enabled() || !e || !e->valid ||
       !e->signature_valid || !e->texture || !e->source_copy.data ||
       e->source_copy.source_size!=bytes || e->source_copy.palette_size!=palette_bytes)return 0;
    const D3D8Texture *host=(const D3D8Texture*)e->texture;
    const NvTextureRevision *r=&e->revision;
    if(!host->d3d11_texture || !host->srv || host->surface_srv || host->locked || host->dirty ||
       !e->host_content_revision || e->host_content_revision!=host->retained_content_revision ||
       r->source!=source || r->bytes!=bytes || r->palette!=palette || r->palette_bytes!=palette_bytes)return 0;
    if(r->exact){
        uintptr_t ram=(uintptr_t)g_pg.guest_memory,p=(uintptr_t)palette;
        if(g_pg.guest_memory!=(const uint8_t*)xbox_GetMemoryBase() || e->offset>g_pg.guest_memory_size ||
           bytes>g_pg.guest_memory_size-e->offset || source!=g_pg.guest_memory+e->offset ||
           (palette_bytes&&(p<ram || p-ram>g_pg.guest_memory_size || palette_bytes>g_pg.guest_memory_size-(p-ram))))return 0;
        return texture_revision_exact_clean(r,&e->source_copy,source,bytes,palette,palette_bytes);
    }
    if(!r->source_id || !r->source_token || !uc_resource_clean(r->source_id,r->source_token))return 0;
    return !palette_bytes || (palette && r->palette_id && r->palette_token &&
        uc_resource_clean(r->palette_id,r->palette_token));
#else
    (void)e;(void)source;(void)bytes;(void)palette;(void)palette_bytes;return 0;
#endif
}
static void raw_retained_prepare_reset(void){
    memset(raw_retained_resources.prepared,0,sizeof(raw_retained_resources.prepared));
}
static void raw_retained_prepare_none(unsigned stage){
    if(stage>=4)return;
    NvRetainedResource *r=&raw_retained_resources.prepared[stage];
    memset(r,0,sizeof(*r));r->kind=NV_RETAIN_NONE;
}
static void raw_retained_prepare_2d(unsigned stage,LiveTextureCacheEntry *entry,
    const uint8_t *source,size_t bytes,const uint8_t *palette,size_t palette_bytes){
    if(stage>=4 || !raw_draw_retain_enabled())return;
    NvRetainedResource *r=&raw_retained_resources.prepared[stage];
    memset(r,0,sizeof(*r));
    if(!raw_retained_2d_tokens_clean(entry,source,bytes,palette,palette_bytes))return;
    D3D8Texture *host=(D3D8Texture*)entry->texture;
    r->kind=NV_RETAIN_2D;r->entry=entry;r->texture=entry->texture;
    r->resource=(ID3D11Resource*)host->d3d11_texture;r->view=host->srv;
    r->source=source;r->palette=palette;r->bytes=bytes;r->palette_bytes=palette_bytes;
    r->copy=entry->source_copy.data;r->offset=entry->offset;r->format=entry->format;
    r->width=entry->width;r->height=entry->height;r->levels=entry->levels;
    r->host_content_revision=entry->host_content_revision;
#ifdef UCRR_RESOURCE_REVISIONS
    r->revision=entry->revision;
#endif
    if(palette_bytes){
        r->palette_register=g_pg.trace_methods[(NV097_SET_TEXTURE_PALETTE+stage*0x40)/4];
        r->palette_handle=g_pg.trace_methods[(r->palette_register&1?NV097_SET_CONTEXT_DMA_B:NV097_SET_CONTEXT_DMA_A)/4];
    }
}
static void raw_retained_prepare_light(unsigned stage,ID3D11ShaderResourceView *view){
    if(stage>=4 || !raw_draw_retain_enabled())return;
    NvRetainedResource *r=&raw_retained_resources.prepared[stage];
    memset(r,0,sizeof(*r));
#ifdef UCRR_RAM_WATCH_SHADOW
    if(!view || g_pg.guest_memory!=(const uint8_t*)xbox_GetMemoryBase())return;
    for(NvLightIterator it=nv_light_iter(&lighting_textures);it.texture;nv_light_iter_next(&it)){
        NvLightTexture *t=it.texture;
        if(t->view!=view)continue;
        if(!t->resource || !t->source || t->offset!=g_pg.tex[stage].offset ||
           t->format!=g_pg.tex[stage].format || t->signs!=(g_pg.tex[stage].filter>>28) ||
           t->offset>g_pg.guest_memory_size || t->size>g_pg.guest_memory_size-t->offset ||
           !nv_light_watch_certificate(t->source,t->size,t->offset,t->format,t->signs,&r->light_certificate))return;
        r->kind=NV_RETAIN_LIGHT;r->light=t;r->resource=t->resource;r->view=t->view;
        r->copy=t->source;r->bytes=t->size;r->offset=t->offset;r->format=t->format;r->signs=t->signs;
        return;
    }
#else
    (void)view;
#endif
}
static int raw_retained_resource_clean(const NvRetainedResource *r,unsigned stage){
    if(r->kind==NV_RETAIN_NONE)return 1;
    if(r->kind==NV_RETAIN_2D){
#ifdef UCRR_RESOURCE_REVISIONS
        const LiveTextureCacheEntry *e=r->entry;
        if(!e || e->texture!=r->texture || e->source_copy.data!=r->copy ||
           e->offset!=r->offset || e->format!=r->format || e->width!=r->width ||
           e->height!=r->height || e->levels!=r->levels || e->host_content_revision!=r->host_content_revision ||
           !raw_retained_2d_tokens_clean(e,r->source,r->bytes,r->palette,r->palette_bytes))return 0;
        const D3D8Texture *host=(const D3D8Texture*)e->texture;
        const NvTextureRevision *v=&e->revision;
        if((ID3D11Resource*)host->d3d11_texture!=r->resource || host->srv!=r->view ||
           v->exact!=r->revision.exact || v->source_id!=r->revision.source_id || v->source_token!=r->revision.source_token ||
           v->palette_id!=r->revision.palette_id || v->palette_token!=r->revision.palette_token)return 0;
        if(r->palette_bytes){
            uint32_t packed=g_pg.trace_methods[(NV097_SET_TEXTURE_PALETTE+stage*0x40)/4],address;
            uint32_t handle=g_pg.trace_methods[(packed&1?NV097_SET_CONTEXT_DMA_B:NV097_SET_CONTEXT_DMA_A)/4];
            /* Descriptor RAM is mutable independently of texture/palette data.
             * Revalidate this small address lookup independently of contents. */
            if(packed!=r->palette_register || handle!=r->palette_handle ||
               !nv2a_dma_resolve_access(g_pg.gpu_registers,g_pg.gpu_registers_size,handle,
                   packed&0xFFFFFFC0u,r->palette_bytes,g_pg.guest_memory_size,&address,0) ||
               g_pg.guest_memory+address!=r->palette)return 0;
        }
        return 1;
#else
        return 0;
#endif
    }
    if(r->kind==NV_RETAIN_LIGHT){
#ifdef UCRR_RAM_WATCH_SHADOW
        const NvLightTexture *t=r->light;
        return t && t->resource==r->resource && t->view==r->view && t->source==r->copy &&
            t->size==r->bytes && t->offset==r->offset && t->format==r->format && t->signs==r->signs &&
            nv_light_watch_certificate_clean(&r->light_certificate);
#else
        return 0;
#endif
    }
    return 0;
}
static void raw_retained_resources_release(void){
    /* Single renderer owner: capture rejects before acquiring references, then
     * performs non-reentrant AddRefs and marks active before its final check.
     * Outside that acquisition, inactive therefore implies zero held refs. */
    if(!raw_retained_resources.active)return;
    for(unsigned t=0;t<4;++t){
        NvRetainedResource *r=&raw_retained_resources.held[t];
        if(r->view)ID3D11ShaderResourceView_Release(r->view);
        if(r->resource)ID3D11Resource_Release(r->resource);
        if(r->texture)r->texture->lpVtbl->Release(r->texture);
    }
    memset(raw_retained_resources.held,0,sizeof(raw_retained_resources.held));
    raw_retained_resources.active=0;
}
static int raw_retained_resources_clean(void){
    if(!raw_retained_resources.active || raw_retained_resources.ram!=g_pg.guest_memory ||
       raw_retained_resources.ram_size!=g_pg.guest_memory_size ||
       raw_retained_resources.registers!=g_pg.gpu_registers ||
       raw_retained_resources.registers_size!=g_pg.gpu_registers_size)return 0;
    for(unsigned t=0;t<4;++t){
        const NvRetainedResource *r=&raw_retained_resources.held[t];
        if(memcmp(r->binding,&g_pg.tex[t],sizeof(g_pg.tex[t])) || !raw_retained_resource_clean(r,t))return 0;
    }
    return 1;
}
static int raw_retained_resources_capture(void){
    raw_retained_resources_release();
    if(!raw_draw_retain_enabled())return 0;
    for(unsigned t=0;t<4;++t)
        if(!raw_retained_resource_clean(&raw_retained_resources.prepared[t],t))return 0;
    for(unsigned t=0;t<4;++t){
        NvRetainedResource *r=&raw_retained_resources.held[t];
        *r=raw_retained_resources.prepared[t];
        memcpy(r->binding,&g_pg.tex[t],sizeof(g_pg.tex[t]));
        if(r->texture)r->texture->lpVtbl->AddRef(r->texture);
        if(r->resource)ID3D11Resource_AddRef(r->resource);
        if(r->view)ID3D11ShaderResourceView_AddRef(r->view);
    }
    raw_retained_resources.ram=g_pg.guest_memory;raw_retained_resources.ram_size=g_pg.guest_memory_size;
    raw_retained_resources.registers=g_pg.gpu_registers;raw_retained_resources.registers_size=g_pg.gpu_registers_size;
    raw_retained_resources.active=1;
    if(!raw_retained_resources_clean()){raw_retained_resources_release();return 0;}
    return 1;
}
#endif
