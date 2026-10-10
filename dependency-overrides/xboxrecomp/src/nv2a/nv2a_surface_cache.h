#ifndef NV_VERTEX_BINDING_INVALIDATE
#define NV_VERTEX_BINDING_INVALIDATE() ((void)0)
#endif
#ifndef NV2A_SURFACE_CACHE_H
#define NV2A_SURFACE_CACHE_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef NV_SURFACE_SCALE
#define NV_SURFACE_SCALE() 1u
#endif
#ifndef NV_SURFACE_MSAA
#define NV_SURFACE_MSAA() 1u
#endif
#ifndef NV_SURFACE_WRITE_SERIAL
#define NV_SURFACE_WRITE_SERIAL() 0ull
#endif
static int nv_surface_snapshot_reuse_option=-1;
static uint64_t nv_surface_snapshot_requests,nv_surface_snapshot_hits;
static int nv_surface_snapshot_reuse_enabled(void){
    if(nv_surface_snapshot_reuse_option<0){const char*s=getenv("UCRR_SURFACE_SNAPSHOT_REUSE");nv_surface_snapshot_reuse_option=s&&!strcmp(s,"1");}
    return nv_surface_snapshot_reuse_option;
}
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <d3d11.h>
#include <d3dcompiler.h>
#ifndef NV_SURFACE_SET_PS
#define NV_SURFACE_SET_PS(c,p) ID3D11DeviceContext_PSSetShader(c,p,NULL,0)
#endif
#ifndef NV_SURFACE_SET_TARGETS
#define NV_SURFACE_SET_TARGETS ID3D11DeviceContext_OMSetRenderTargets
#endif
#ifndef NV_SURFACE_SET_VIEWPORTS
#define NV_SURFACE_SET_VIEWPORTS ID3D11DeviceContext_RSSetViewports
#endif
#ifndef NV_SURFACE_SET_BLEND
#define NV_SURFACE_SET_BLEND ID3D11DeviceContext_OMSetBlendState
#define NV_SURFACE_SET_DEPTH ID3D11DeviceContext_OMSetDepthStencilState
#define NV_SURFACE_SET_RASTER ID3D11DeviceContext_RSSetState
#endif

/* Linear BGRA8 color and Z24S8, with optional host multisampling.
 * Stable identities, bounded allocation, no guest-memory ownership transfer.
 * CPU/GPU coherence after guest CPU writes remains a separate requirement. */
typedef struct NvSurface {
    uint32_t offset,pitch,width,height,physical_width,physical_height,samples;
    ID3D11Texture2D *texture;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *srv;
    ID3D11ShaderResourceView *stencil_srv;
    ID3D11Texture2D *depth_color;
    ID3D11RenderTargetView *depth_color_rtv;
    ID3D11Texture2D *physical_copy;
    ID3D11RenderTargetView *physical_copy_rtv;
    ID3D11ShaderResourceView *physical_copy_srv;
    uint64_t physical_copy_serial;
} NvSurface;
#ifndef UCRR_PREPARATION_COMMAND
#define UCRR_PREPARATION_COMMAND 0
#endif
#if UCRR_PREPARATION_COMMAND
/* Prepared acquisition only; never certifies surface/guest content. Resident
 * NvSurface metadata is stable until this cache's shutdown. Keep this memo in
 * the cache so destruction/recreation clears all borrowed references. */
typedef struct NvSurfacePairKey {
    uint32_t format,pitch,color,depth,clip_h,clip_v;
    ID3D11Device *device;
    const uint8_t *ram;
    size_t ram_size;
} NvSurfacePairKey;
typedef struct NvSurfacePairMemo {
    NvSurfacePairKey key;
    NvSurface *color,*depth;
    unsigned valid;
} NvSurfacePairMemo;
static int nv_surface_pair_equal(const NvSurfacePairKey*a,const NvSurfacePairKey*b){
    return a->format==b->format&&a->pitch==b->pitch&&a->color==b->color&&
        a->depth==b->depth&&a->clip_h==b->clip_h&&a->clip_v==b->clip_v&&
        a->device==b->device&&a->ram==b->ram&&a->ram_size==b->ram_size;
}
#endif
/* Overflow entries never move or evict GPU-written contents. Metadata is
 * charged to the same byte budget as their resources; borrowed NvSurface
 * pointers therefore remain valid until cache shutdown. */
typedef struct NvSurfaceOverflow {struct NvSurfaceOverflow *next;NvSurface surface;} NvSurfaceOverflow;
typedef struct NvSurfaceCache {
#if UCRR_PREPARATION_COMMAND
    NvSurfacePairMemo prepared_pair;
#endif
    NvSurface color[16],depth[16];uint64_t bytes;
    unsigned lookup_hint[2][16];
    ID3D11VertexShader *blit_vs;ID3D11PixelShader *blit_ps;
    ID3D11PixelShader *depth_color_ps;
    ID3D11PixelShader *blit_ms_ps,*depth_ms_ps;
    ID3D11RasterizerState *blit_rs;ID3D11DepthStencilState *blit_ds;
    NvSurfaceOverflow *overflow[2];
}NvSurfaceCache;
typedef struct NvSurfaceIterator {NvSurface *surface,*slots;NvSurfaceOverflow *next;unsigned index;} NvSurfaceIterator;
static NvSurfaceIterator nv_surface_iter(NvSurfaceCache*c,unsigned depth){
    NvSurfaceIterator it={depth?c->depth:c->color,depth?c->depth:c->color,c->overflow[!!depth],0};return it;
}
static void nv_surface_iter_next(NvSurfaceIterator*it){
    if(++it->index<16)it->surface=&it->slots[it->index];
    else if(it->next){it->surface=&it->next->surface;it->next=it->next->next;}
    else it->surface=NULL;
}
static unsigned nv_surface_sample_count(ID3D11Device *dev){
    unsigned n=NV_SURFACE_MSAA();UINT color=0,depth=0;
    if(n!=2&&n!=4)return 1;
    if(FAILED(ID3D11Device_CheckMultisampleQualityLevels(dev,DXGI_FORMAT_B8G8R8A8_UNORM,n,&color))||
       FAILED(ID3D11Device_CheckMultisampleQualityLevels(dev,DXGI_FORMAT_D24_UNORM_S8_UINT,n,&depth))||!color||!depth)return 1;
    return n;
}
static int nv_surface_blit(NvSurfaceCache*,ID3D11Device*,ID3D11DeviceContext*,NvSurface*,ID3D11RenderTargetView*,uint32_t,uint32_t);

static NvSurface *nv_surface_find(NvSurface *slots,uint32_t offset,uint32_t pitch,uint32_t w,uint32_t h){
    for(unsigned i=0;i<16;++i)if(slots[i].texture && slots[i].offset==offset && slots[i].pitch==pitch && slots[i].width==w && slots[i].height==h)return &slots[i];
    return NULL;
}
static NvSurface *nv_surface_find_cached(NvSurfaceCache *cache,unsigned depth,uint32_t offset,uint32_t pitch,uint32_t w,uint32_t h){
    static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_SURFACE_LOOKUP_HINT");enabled=s&&!strcmp(s,"1");}
    NvSurface *slots=depth?cache->depth:cache->color;
    unsigned bucket=((offset>>8)^pitch^(w*3u)^(h*7u))&15u;
    unsigned hint=enabled?cache->lookup_hint[!!depth][bucket]:0;
    if(hint&&hint<=16){NvSurface*s=&slots[hint-1];
        if(s->texture&&s->offset==offset&&s->pitch==pitch&&s->width==w&&s->height==h)return s;
    }
    NvSurface*s=nv_surface_find(slots,offset,pitch,w,h);
    if(enabled)cache->lookup_hint[!!depth][bucket]=s?(unsigned)(s-slots)+1:0;
    if(s)return s;
    for(NvSurfaceOverflow*n=cache->overflow[!!depth];n;n=n->next){s=&n->surface;
        if(s->texture&&s->offset==offset&&s->pitch==pitch&&s->width==w&&s->height==h)return s;
    }
    return NULL;
}
/* Opt-in diagnostic only. Reads the real GPU resource, without modifying it.
 * Caller supplies a unique session filename and a strict capture budget. */
static int nv_surface_dump_bmp(ID3D11Device *dev,ID3D11DeviceContext *ctx,NvSurface *s,const char *path){
    if(!s || !s->texture || !path)return 0;
    D3D11_TEXTURE2D_DESC td;ID3D11Texture2D_GetDesc(s->texture,&td);
    if(td.SampleDesc.Count!=1 || td.Format!=DXGI_FORMAT_B8G8R8A8_UNORM || td.Width>2048 || td.Height>2048)return 0;
    td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D *stage=NULL;
    if(FAILED(ID3D11Device_CreateTexture2D(dev,&td,NULL,&stage)))return 0;
    ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)stage,(ID3D11Resource*)s->texture);
    D3D11_MAPPED_SUBRESOURCE map;int ok=0;
    if(SUCCEEDED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)stage,0,D3D11_MAP_READ,0,&map))){
        FILE*f=fopen(path,"wb");
        if(f){
            uint8_t header[54]={0};uint32_t size=54+td.Width*td.Height*4,off=54,dib=40,bpp=32;
            int32_t height=-(int32_t)td.Height;uint16_t planes=1;
            header[0]='B';header[1]='M';memcpy(header+2,&size,4);memcpy(header+10,&off,4);memcpy(header+14,&dib,4);
            memcpy(header+18,&td.Width,4);memcpy(header+22,&height,4);memcpy(header+26,&planes,2);memcpy(header+28,&bpp,2);
            ok=fwrite(header,1,54,f)==54;
            for(unsigned y=0;y<td.Height&&ok;++y)ok=fwrite((uint8_t*)map.pData+y*map.RowPitch,4,td.Width,f)==td.Width;
            if(fclose(f)!=0)ok=0;
        }
        ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)stage,0);
    }
    ID3D11Texture2D_Release(stage);return ok;
}
static NvSurface *nv_surface_get(NvSurfaceCache *cache,ID3D11Device *dev,int depth,
    uint32_t offset,uint32_t pitch,uint32_t w,uint32_t h,const uint8_t *ram,size_t ram_size){
    if(!dev || !w || !h || w>2048 || h>2048 || pitch<(uint64_t)w*4 ||
       offset>=ram_size || (uint64_t)pitch*h>ram_size-offset)return NULL;
    NvSurface *slots=depth?cache->depth:cache->color;
    NvSurface *found=nv_surface_find_cached(cache,depth,offset,pitch,w,h);if(found)return found;
    unsigned scale=NV_SURFACE_SCALE();if(scale<1||scale>8)return NULL;
    unsigned samples=nv_surface_sample_count(dev);
    uint64_t bytes=(uint64_t)w*h*4*scale*scale*samples;
    if(cache->bytes+bytes>96ull*1024u*1024u*scale*scale*samples)return NULL;
    NvSurface *s=NULL;NvSurfaceOverflow *pending=NULL;
    for(unsigned i=0;i<16;++i)if(!slots[i].texture){s=&slots[i];break;}
    if(!s){
        if(cache->bytes+bytes+sizeof(*pending)>96ull*1024u*1024u*scale*scale*samples)return NULL;
        pending=(NvSurfaceOverflow*)calloc(1,sizeof(*pending));if(!pending)return NULL;s=&pending->surface;
    }
    D3D11_TEXTURE2D_DESC td={0};td.Width=w*scale;td.Height=h*scale;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=samples;
    td.Format=depth?DXGI_FORMAT_R24G8_TYPELESS:DXGI_FORMAT_B8G8R8A8_UNORM;
    td.BindFlags=(depth?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_RENDER_TARGET)|D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data={0};data.pSysMem=ram?ram+offset:NULL;data.SysMemPitch=pitch;
    uint8_t *expanded=NULL;
    if(!depth&&ram&&scale!=1){
        expanded=(uint8_t*)malloc((size_t)bytes/samples);if(!expanded){free(pending);return NULL;}
        for(unsigned y=0;y<td.Height;++y)for(unsigned x=0;x<td.Width;++x)
            memcpy(expanded+((size_t)y*td.Width+x)*4,ram+offset+(y/scale)*pitch+(x/scale)*4,4);
        data.pSysMem=expanded;data.SysMemPitch=td.Width*4;
    }
    HRESULT hr=ID3D11Device_CreateTexture2D(dev,&td,(!depth&&ram&&samples==1)?&data:NULL,&s->texture);
    if(FAILED(hr)){free(expanded);free(pending);return NULL;}
    if(depth){
        D3D11_DEPTH_STENCIL_VIEW_DESC dd={0};dd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;dd.ViewDimension=samples>1?D3D11_DSV_DIMENSION_TEXTURE2DMS:D3D11_DSV_DIMENSION_TEXTURE2D;
        hr=ID3D11Device_CreateDepthStencilView(dev,(ID3D11Resource*)s->texture,&dd,&s->dsv);
        D3D11_SHADER_RESOURCE_VIEW_DESC sd={0};sd.Format=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;sd.ViewDimension=samples>1?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;if(samples==1)sd.Texture2D.MipLevels=1;
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)s->texture,&sd,&s->srv);
        sd.Format=DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)s->texture,&sd,&s->stencil_srv);
    }
    else {
        hr=ID3D11Device_CreateRenderTargetView(dev,(ID3D11Resource*)s->texture,NULL,&s->rtv);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)s->texture,NULL,&s->srv);
    }
    if(SUCCEEDED(hr)&&!depth&&ram&&samples>1){
        /* MS textures cannot have initial subresource data. Seed all samples
         * from an ordinary upload with a full-screen draw, retaining state. */
        NvSurface upload={0};D3D11_TEXTURE2D_DESC seed=td;seed.SampleDesc.Count=1;seed.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        hr=ID3D11Device_CreateTexture2D(dev,&seed,&data,&upload.texture);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)upload.texture,NULL,&upload.srv);
        if(SUCCEEDED(hr)){ID3D11DeviceContext *ctx=NULL;ID3D11Device_GetImmediateContext(dev,&ctx);upload.width=td.Width;upload.height=td.Height;upload.samples=1;
            if(!nv_surface_blit(cache,dev,ctx,&upload,s->rtv,td.Width,td.Height))hr=E_FAIL;ID3D11DeviceContext_Release(ctx);}
        if(upload.srv)ID3D11ShaderResourceView_Release(upload.srv);if(upload.texture)ID3D11Texture2D_Release(upload.texture);
    }
    free(expanded);
    if(FAILED(hr)){
        if(s->rtv)ID3D11RenderTargetView_Release(s->rtv);
        if(s->dsv)ID3D11DepthStencilView_Release(s->dsv);
        if(s->srv)ID3D11ShaderResourceView_Release(s->srv);
        if(s->stencil_srv)ID3D11ShaderResourceView_Release(s->stencil_srv);
        ID3D11Texture2D_Release(s->texture);memset(s,0,sizeof(*s));free(pending);return NULL;
    }
    s->offset=offset;s->pitch=pitch;s->width=w;s->height=h;s->physical_width=td.Width;s->physical_height=td.Height;s->samples=samples;cache->bytes+=bytes;
    if(pending){pending->next=cache->overflow[!!depth];cache->overflow[!!depth]=pending;cache->bytes+=sizeof(*pending);}
    return s;
}
static void nv_surface_shutdown(NvSurfaceCache *cache){
    if(nv_surface_snapshot_reuse_enabled())fprintf(stderr,"[SURFACE-REUSE] requests=%llu hits=%llu\n",(unsigned long long)nv_surface_snapshot_requests,(unsigned long long)nv_surface_snapshot_hits);
    if(cache->blit_vs)ID3D11VertexShader_Release(cache->blit_vs);
    if(cache->blit_ps)ID3D11PixelShader_Release(cache->blit_ps);
    if(cache->depth_color_ps)ID3D11PixelShader_Release(cache->depth_color_ps);
    if(cache->blit_ms_ps)ID3D11PixelShader_Release(cache->blit_ms_ps);
    if(cache->depth_ms_ps)ID3D11PixelShader_Release(cache->depth_ms_ps);
    if(cache->blit_rs)ID3D11RasterizerState_Release(cache->blit_rs);
    if(cache->blit_ds)ID3D11DepthStencilState_Release(cache->blit_ds);
    for(unsigned d=0;d<2;++d)for(NvSurfaceIterator it=nv_surface_iter(cache,d);it.surface;nv_surface_iter_next(&it)){
        NvSurface *s=it.surface;
        if(s->srv)ID3D11ShaderResourceView_Release(s->srv);
        if(s->stencil_srv)ID3D11ShaderResourceView_Release(s->stencil_srv);
        if(s->depth_color_rtv)ID3D11RenderTargetView_Release(s->depth_color_rtv);
        if(s->depth_color)ID3D11Texture2D_Release(s->depth_color);
        if(s->physical_copy_srv)ID3D11ShaderResourceView_Release(s->physical_copy_srv);
        if(s->physical_copy_rtv)ID3D11RenderTargetView_Release(s->physical_copy_rtv);
        if(s->physical_copy)ID3D11Texture2D_Release(s->physical_copy);
        if(s->rtv)ID3D11RenderTargetView_Release(s->rtv);
        if(s->dsv)ID3D11DepthStencilView_Release(s->dsv);
        if(s->texture)ID3D11Texture2D_Release(s->texture);
    }
    for(unsigned d=0;d<2;++d){NvSurfaceOverflow*n=cache->overflow[d];while(n){NvSurfaceOverflow*next=n->next;free(n);n=next;}}
    memset(cache,0,sizeof(*cache));
}
/* Typed BGRA sampling performs channel conversion to the host RGBA target.
 * Exact-size scanout only; no implicit scaling or extra Present. */
static int nv_surface_blit(NvSurfaceCache *cache,ID3D11Device *dev,ID3D11DeviceContext *ctx,
    NvSurface *src,ID3D11RenderTargetView *dst,uint32_t width,uint32_t height){
    if(!src || !src->srv || !dst)return 0;
    if(!((width==src->width && height==src->height) ||
         (width==src->physical_width && height==src->physical_height)))return 0;
    if(!cache->blit_vs){
        const char *v="struct O{float4 p:SV_POSITION;float2 uv:TEXCOORD0;};O main(uint id:SV_VertexID){O o;o.p=float4(id==2?3:-1,id==1?3:-1,0,1);o.uv=o.p.xy*float2(.5,-.5)+.5;return o;}";
        /* Integer box resolve at 1x through 8x; no crop or sampler state. */
        const char *p="Texture2D<float4> t:register(t0);float4 main(float4 pos:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET{uint w,h;t.GetDimensions(w,h);float2 size=float2(w,h);int2 n=clamp(int2(round(abs(float2(ddx(uv.x),ddy(uv.y)))*size)),1,8);int2 a=int2(floor(uv*size/n))*n;float4 c=0;[loop]for(int y=0;y<n.y;++y)[loop]for(int x=0;x<n.x;++x)c+=t.Load(int3(clamp(a+int2(x,y),int2(0,0),int2(w-1,h-1)),0));return c/(n.x*n.y);}";
        ID3DBlob *vb=NULL,*pb=NULL;
        HRESULT hr=D3DCompile(v,strlen(v),"scanout_vs",NULL,NULL,"main","vs_4_0",0,0,&vb,NULL);
        if(SUCCEEDED(hr))hr=D3DCompile(p,strlen(p),"scanout_ps",NULL,NULL,"main","ps_4_0",0,0,&pb,NULL);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateVertexShader(dev,ID3D10Blob_GetBufferPointer(vb),ID3D10Blob_GetBufferSize(vb),NULL,&cache->blit_vs);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreatePixelShader(dev,ID3D10Blob_GetBufferPointer(pb),ID3D10Blob_GetBufferSize(pb),NULL,&cache->blit_ps);
        D3D11_RASTERIZER_DESC rd={0};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;rd.MultisampleEnable=TRUE;
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateRasterizerState(dev,&rd,&cache->blit_rs);
        D3D11_DEPTH_STENCIL_DESC dd={0};dd.DepthFunc=D3D11_COMPARISON_ALWAYS;
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateDepthStencilState(dev,&dd,&cache->blit_ds);
        if(vb)ID3D10Blob_Release(vb);if(pb)ID3D10Blob_Release(pb);
        if(FAILED(hr))return 0;
    }
    if(!cache->blit_ps || !cache->blit_rs || !cache->blit_ds)return 0;
    ID3D11PixelShader **ms_shader=src->dsv?&cache->depth_ms_ps:&cache->blit_ms_ps;
    if(src->samples>1&&!*ms_shader){
        /* Average color coverage; depth/stencil byte reinterpretation keeps
         * sample zero, never an invalid averaged packed depth/stencil word. */
        const char *p=src->dsv?
          "Texture2DMS<float> d:register(t0);Texture2DMS<uint4> s:register(t1);float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET{uint w,h,n;d.GetDimensions(w,h,n);int2 q=int2(uv*float2(w,h));uint z=(uint)round(saturate(d.Load(q,0))*16777215.0);return float4(z&255,(z>>8)&255,z>>16,s.Load(q,0).g)/255.0;}":
          "Texture2DMS<float4> t:register(t0);float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET{uint w,h,s;t.GetDimensions(w,h,s);float2 size=float2(w,h);int2 n=clamp(int2(round(abs(float2(ddx(uv.x),ddy(uv.y)))*size)),1,8);int2 a=int2(floor(uv*size/n))*n;float4 c=0;[loop]for(int y=0;y<n.y;++y)[loop]for(int x=0;x<n.x;++x)[loop]for(uint i=0;i<s;++i)c+=t.Load(clamp(a+int2(x,y),int2(0,0),int2(w-1,h-1)),i);return c/(n.x*n.y*s);}";
        ID3DBlob *pb=NULL;HRESULT hr=D3DCompile(p,strlen(p),"msaa_resolve",NULL,NULL,"main","ps_4_1",0,0,&pb,NULL);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreatePixelShader(dev,ID3D10Blob_GetBufferPointer(pb),ID3D10Blob_GetBufferSize(pb),NULL,ms_shader);
        if(pb)ID3D10Blob_Release(pb);if(FAILED(hr))return 0;
    }
    if(src->dsv && !cache->depth_color_ps){
        /* Xbox Z24S8 packs Z in high24 and S in low8. Texture format40
         * interprets that word as B8G8R8A8: R=Zlow,G=Zmid,B=Zhigh,A=S.
         * D3D11 D24S8 has the reverse word layout, so a typed copy is wrong. */
        const char*p="Texture2D<float> d:register(t0);Texture2D<uint4> s:register(t1);float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET{uint w,h;d.GetDimensions(w,h);int3 q=int3(int2(uv*float2(w,h)),0);uint z=(uint)round(saturate(d.Load(q))*16777215.0);return float4(z&255,(z>>8)&255,z>>16,s.Load(q).g)/255.0;}";
        ID3DBlob *pb=NULL;HRESULT hr=D3DCompile(p,strlen(p),"depth_bytes",NULL,NULL,"main","ps_4_0",0,0,&pb,NULL);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreatePixelShader(dev,ID3D10Blob_GetBufferPointer(pb),ID3D10Blob_GetBufferSize(pb),NULL,&cache->depth_color_ps);
        if(pb)ID3D10Blob_Release(pb);if(FAILED(hr))return 0;
    }
    ID3D11VertexShader *vs=NULL;ID3D11PixelShader *ps=NULL;ID3D11GeometryShader *gs=NULL;
    ID3D11InputLayout *layout=NULL;ID3D11RasterizerState *rs=NULL;ID3D11DepthStencilState *ds=NULL;
    ID3D11BlendState *blend=NULL;ID3D11ShaderResourceView *srv[2]={NULL,NULL};ID3D11RenderTargetView *rtv=NULL;ID3D11DepthStencilView *dsv=NULL;
    D3D11_PRIMITIVE_TOPOLOGY topology;D3D11_VIEWPORT oldvp[16];UINT nvp=16,stencil,mask;FLOAT factors[4];
    ID3D11DeviceContext_VSGetShader(ctx,&vs,NULL,NULL);ID3D11DeviceContext_PSGetShader(ctx,&ps,NULL,NULL);ID3D11DeviceContext_GSGetShader(ctx,&gs,NULL,NULL);
    ID3D11DeviceContext_IAGetInputLayout(ctx,&layout);ID3D11DeviceContext_IAGetPrimitiveTopology(ctx,&topology);
    ID3D11DeviceContext_RSGetState(ctx,&rs);ID3D11DeviceContext_RSGetViewports(ctx,&nvp,oldvp);
    ID3D11DeviceContext_OMGetDepthStencilState(ctx,&ds,&stencil);ID3D11DeviceContext_OMGetBlendState(ctx,&blend,factors,&mask);
    ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rtv,&dsv);ID3D11DeviceContext_PSGetShaderResources(ctx,0,2,srv);
    NV_SURFACE_SET_TARGETS(ctx,1,&dst,NULL);NV_SURFACE_SET_DEPTH(ctx,cache->blit_ds,0);NV_SURFACE_SET_BLEND(ctx,NULL,NULL,UINT32_MAX);
    NV_SURFACE_SET_RASTER(ctx,cache->blit_rs);D3D11_VIEWPORT vp={0,0,(FLOAT)width,(FLOAT)height,0,1};NV_SURFACE_SET_VIEWPORTS(ctx,1,&vp);
    NV_VERTEX_BINDING_INVALIDATE();
    ID3D11DeviceContext_IASetInputLayout(ctx,NULL);ID3D11DeviceContext_IASetPrimitiveTopology(ctx,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(ctx,cache->blit_vs,NULL,0);NV_SURFACE_SET_PS(ctx,src->samples>1?*ms_shader:src->dsv?cache->depth_color_ps:cache->blit_ps);ID3D11DeviceContext_GSSetShader(ctx,NULL,NULL,0);
    ID3D11ShaderResourceView *sources[2]={src->srv,src->dsv?src->stencil_srv:NULL};
    ID3D11DeviceContext_PSSetShaderResources(ctx,0,2,sources);ID3D11DeviceContext_Draw(ctx,3,0);
    ID3D11DeviceContext_PSSetShaderResources(ctx,0,2,srv);NV_SURFACE_SET_TARGETS(ctx,1,&rtv,dsv);
    NV_SURFACE_SET_BLEND(ctx,blend,factors,mask);NV_SURFACE_SET_DEPTH(ctx,ds,stencil);
    NV_SURFACE_SET_RASTER(ctx,rs);NV_SURFACE_SET_VIEWPORTS(ctx,nvp,oldvp);
    ID3D11DeviceContext_IASetInputLayout(ctx,layout);ID3D11DeviceContext_IASetPrimitiveTopology(ctx,topology);
    ID3D11DeviceContext_VSSetShader(ctx,vs,NULL,0);NV_SURFACE_SET_PS(ctx,ps);ID3D11DeviceContext_GSSetShader(ctx,gs,NULL,0);
    if(vs)ID3D11VertexShader_Release(vs);if(ps)ID3D11PixelShader_Release(ps);if(gs)ID3D11GeometryShader_Release(gs);
    if(layout)ID3D11InputLayout_Release(layout);if(rs)ID3D11RasterizerState_Release(rs);if(ds)ID3D11DepthStencilState_Release(ds);
    if(blend)ID3D11BlendState_Release(blend);for(unsigned i=0;i<2;++i)if(srv[i])ID3D11ShaderResourceView_Release(srv[i]);if(rtv)ID3D11RenderTargetView_Release(rtv);if(dsv)ID3D11DepthStencilView_Release(dsv);
    return 1;
}
/* Display scanout may stretch to any client size; feedback blits retain their
 * strict guest-size contract. Borrow only: no ownership or identity changes. */
static int nv_surface_present(NvSurfaceCache *cache,ID3D11Device *dev,ID3D11DeviceContext *ctx,
 NvSurface *source,ID3D11RenderTargetView *target,uint32_t width,uint32_t height){
 if(!source||!width||!height||width>16384||height>16384)return 0;
 NvSurface view=*source;view.width=width;view.height=height;
 return nv_surface_blit(cache,dev,ctx,&view,target,width,height);
}
static ID3D11Texture2D *nv_surface_native_snapshot(NvSurfaceCache*cache,ID3D11Device*dev,ID3D11DeviceContext*ctx,NvSurface*s){
    if(!s||!s->srv||(s->dsv&&!s->stencil_srv))return NULL;
    if(!s->depth_color){
        uint64_t bytes=(uint64_t)s->width*s->height*4;
        if(cache->bytes+bytes>96ull*1024u*1024u*NV_SURFACE_SCALE()*NV_SURFACE_SCALE()*nv_surface_sample_count(dev))return NULL;
        D3D11_TEXTURE2D_DESC td={0};td.Width=s->width;td.Height=s->height;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;
        td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
        HRESULT hr=ID3D11Device_CreateTexture2D(dev,&td,NULL,&s->depth_color);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateRenderTargetView(dev,(ID3D11Resource*)s->depth_color,NULL,&s->depth_color_rtv);
        if(FAILED(hr)){if(s->depth_color)ID3D11Texture2D_Release(s->depth_color);s->depth_color=NULL;return NULL;}
        cache->bytes+=bytes;
    }
    return nv_surface_blit(cache,dev,ctx,s,s->depth_color_rtv,s->width,s->height)?s->depth_color:NULL;
}
static ID3D11Texture2D *nv_surface_depth_snapshot(NvSurfaceCache*cache,ID3D11Device*dev,ID3D11DeviceContext*ctx,NvSurface*s){
    return s&&s->dsv?nv_surface_native_snapshot(cache,dev,ctx,s):NULL;
}
/* Independent full-resolution snapshot permits sampling a surface while drawing
 * into it. Keep normalized guest UVs and preserve sub-native-pixel detail. */
static int nv_surface_copy_physical_color(ID3D11DeviceContext*ctx,NvSurface*s){
    if(!s||s->dsv||!s->texture||!s->physical_copy||s->texture==s->physical_copy)return 0;
    D3D11_TEXTURE2D_DESC src,dst;
    ID3D11Texture2D_GetDesc(s->texture,&src);
    ID3D11Texture2D_GetDesc(s->physical_copy,&dst);
    /* These owned DEFAULT resources are never mapped. Keep depth conversion,
     * multisample resolve and any format/size conversion on the shader route. */
    if(src.Usage!=D3D11_USAGE_DEFAULT||dst.Usage!=D3D11_USAGE_DEFAULT||
       src.Format!=DXGI_FORMAT_B8G8R8A8_UNORM||dst.Format!=src.Format||
       src.Width!=s->physical_width||src.Height!=s->physical_height||
       dst.Width!=src.Width||dst.Height!=src.Height||
       src.MipLevels!=1||dst.MipLevels!=1||src.ArraySize!=1||dst.ArraySize!=1||
       src.SampleDesc.Count!=1||dst.SampleDesc.Count!=1||
       src.SampleDesc.Quality!=0||dst.SampleDesc.Quality!=0)return 0;
    /* Ordered GPU copy, not a CPU readback. The caller's existing feedback
     * barrier remains responsible for prior texture consumers. No state changes. */
    ID3D11DeviceContext_CopyResource(ctx,(ID3D11Resource*)s->physical_copy,(ID3D11Resource*)s->texture);
    return 1;
}
static ID3D11ShaderResourceView *nv_surface_physical_snapshot(NvSurfaceCache*cache,ID3D11Device*dev,ID3D11DeviceContext*ctx,NvSurface*s){
    if(!s||!s->srv||(s->dsv&&!s->stencil_srv))return NULL;
    uint64_t serial=nv_surface_snapshot_reuse_enabled()?NV_SURFACE_WRITE_SERIAL():0;
    if(serial)++nv_surface_snapshot_requests;
    if(serial&&s->physical_copy_srv&&s->physical_copy_serial==serial){++nv_surface_snapshot_hits;return s->physical_copy_srv;}
    if(!s->physical_copy){
        uint64_t bytes=(uint64_t)s->physical_width*s->physical_height*4;
        if(cache->bytes+bytes>96ull*1024u*1024u*NV_SURFACE_SCALE()*NV_SURFACE_SCALE()*nv_surface_sample_count(dev))return NULL;
        D3D11_TEXTURE2D_DESC td={0};td.Width=s->physical_width;td.Height=s->physical_height;
        td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr=ID3D11Device_CreateTexture2D(dev,&td,NULL,&s->physical_copy);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateRenderTargetView(dev,(ID3D11Resource*)s->physical_copy,NULL,&s->physical_copy_rtv);
        if(SUCCEEDED(hr))hr=ID3D11Device_CreateShaderResourceView(dev,(ID3D11Resource*)s->physical_copy,NULL,&s->physical_copy_srv);
        if(FAILED(hr)){
            if(s->physical_copy_srv)ID3D11ShaderResourceView_Release(s->physical_copy_srv);
            if(s->physical_copy_rtv)ID3D11RenderTargetView_Release(s->physical_copy_rtv);
            if(s->physical_copy)ID3D11Texture2D_Release(s->physical_copy);
            s->physical_copy=NULL;s->physical_copy_rtv=NULL;s->physical_copy_srv=NULL;return NULL;
        }
        cache->bytes+=bytes;
    }
    if(nv_surface_copy_physical_color(ctx,s)||
       nv_surface_blit(cache,dev,ctx,s,s->physical_copy_rtv,s->physical_width,s->physical_height)){
        s->physical_copy_serial=serial;return s->physical_copy_srv;
    }
    s->physical_copy_serial=0;return NULL;
}
#endif
