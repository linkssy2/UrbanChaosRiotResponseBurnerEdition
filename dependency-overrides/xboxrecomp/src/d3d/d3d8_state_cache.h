#ifndef D3D8_STATE_CACHE_H
#define D3D8_STATE_CACHE_H
#include <string.h>
#include <stdint.h>
/* Immutable host states, exact initialized descriptors, bounded round-robin
 * eviction. Cache and caller each own a reference, so evicting a bound entry
 * never releases the caller's state. Clear before changing devices. */
#define D3D8_STATE_CACHE_SIZE 32
#define D3D8_DEFINE_STATE_CACHE(Name,Object,Desc,Create,AddRef,Release) \
typedef struct Name { \
    struct {Desc desc;Object *object;} entries[D3D8_STATE_CACHE_SIZE]; \
    unsigned next;uint64_t creates,hits; \
} Name; \
static HRESULT Name##_get(Name *cache,ID3D11Device *device,const Desc *desc,Object **out){ \
    *out=NULL; \
    for(unsigned i=0;i<D3D8_STATE_CACHE_SIZE;++i){ \
        if(cache->entries[i].object&&!memcmp(desc,&cache->entries[i].desc,sizeof(*desc))){ \
            *out=cache->entries[i].object;AddRef(*out);++cache->hits;return S_OK; \
        } \
    } \
    Object *created=NULL;HRESULT hr=Create(device,desc,&created); \
    if(FAILED(hr))return hr; \
    unsigned slot=cache->next;cache->next=(slot+1)%D3D8_STATE_CACHE_SIZE; \
    if(cache->entries[slot].object)Release(cache->entries[slot].object); \
    cache->entries[slot].object=created;cache->entries[slot].desc=*desc;++cache->creates; \
    AddRef(created);*out=created;return hr; \
} \
static void Name##_clear(Name *cache){ \
    for(unsigned i=0;i<D3D8_STATE_CACHE_SIZE;++i)if(cache->entries[i].object)Release(cache->entries[i].object); \
    memset(cache,0,sizeof(*cache)); \
}
D3D8_DEFINE_STATE_CACHE(D3D8RasterCache,ID3D11RasterizerState,D3D11_RASTERIZER_DESC,
    ID3D11Device_CreateRasterizerState,ID3D11RasterizerState_AddRef,ID3D11RasterizerState_Release)
D3D8_DEFINE_STATE_CACHE(D3D8DepthCache,ID3D11DepthStencilState,D3D11_DEPTH_STENCIL_DESC,
    ID3D11Device_CreateDepthStencilState,ID3D11DepthStencilState_AddRef,ID3D11DepthStencilState_Release)
D3D8_DEFINE_STATE_CACHE(D3D8SamplerCache,ID3D11SamplerState,D3D11_SAMPLER_DESC,
    ID3D11Device_CreateSamplerState,ID3D11SamplerState_AddRef,ID3D11SamplerState_Release)
D3D8_DEFINE_STATE_CACHE(D3D8BlendCache,ID3D11BlendState,D3D11_BLEND_DESC,
    ID3D11Device_CreateBlendState,ID3D11BlendState_AddRef,ID3D11BlendState_Release)
#undef D3D8_DEFINE_STATE_CACHE
#endif
