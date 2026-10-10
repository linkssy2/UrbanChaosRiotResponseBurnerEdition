#ifndef NV_VERTEX_BINDING_INVALIDATE
#define NV_VERTEX_BINDING_INVALIDATE() ((void)0)
#endif
#ifndef NV_CONSTANT_ARENA_H
#define NV_CONSTANT_ARENA_H
#include <d3d11_1.h>
typedef struct NvConstantArena {
    ID3D11DeviceContext1 *context;
    ID3D11Buffer *buffer;
    UINT first[3],count[3],cursor,valid;
    uint64_t maps,discards;
} NvConstantArena;
static void nv_constant_arena_shutdown(NvConstantArena*a){
    if(a->buffer)ID3D11Buffer_Release(a->buffer);
    if(a->context)ID3D11DeviceContext1_Release(a->context);
    memset(a,0,sizeof(*a));
}
static int nv_constant_arena_init(NvConstantArena*a,ID3D11Device*dev){
    D3D11_FEATURE_DATA_D3D11_OPTIONS options={0};
    if(FAILED(ID3D11Device_CheckFeatureSupport(dev,D3D11_FEATURE_D3D11_OPTIONS,&options,sizeof(options)))||
       !options.ConstantBufferOffsetting||!options.MapNoOverwriteOnDynamicConstantBuffer)return 0;
    ID3D11DeviceContext*ctx=NULL;ID3D11Device_GetImmediateContext(dev,&ctx);
    HRESULT hr=ID3D11DeviceContext_QueryInterface(ctx,&IID_ID3D11DeviceContext1,(void**)&a->context);
    ID3D11DeviceContext_Release(ctx);if(FAILED(hr))return 0;
    D3D11_BUFFER_DESC d={0};d.ByteWidth=65536;d.Usage=D3D11_USAGE_DYNAMIC;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;d.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    if(FAILED(ID3D11Device_CreateBuffer(dev,&d,NULL,&a->buffer))){nv_constant_arena_shutdown(a);return 0;}
    return 1;
}
/* One map for all changed per-draw blocks. Offsets/counts use 16-byte constants
 * and are rounded to the required 16-constant (256-byte) granularity. Earlier
 * draws retain old DISCARD allocations. No draw executes while mapped. */
static int nv_constant_arena_upload(NvConstantArena*a,ID3D11DeviceContext*ctx,
    const void*program,const void*conversion,const void*metadata,unsigned program_dirty,unsigned conversion_dirty){
    const UINT sizes[3]={5264,96,528},padded[3]={5376,256,768};
    const void*data[3]={program,conversion,metadata};
    unsigned dirty[3]={program_dirty||!a->valid,conversion_dirty||!a->valid,1};
    UINT needed=768+(dirty[0]?5376:0)+(dirty[1]?256:0),cursor=a->cursor;
    if(!a->valid||cursor>65536-needed){cursor=0;dirty[0]=dirty[1]=1;}
    D3D11_MAPPED_SUBRESOURCE mapped={0};
    if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)a->buffer,0,cursor?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD,0,&mapped))){a->valid=0;a->cursor=0;return 0;}
    if(!cursor)++a->discards;
    for(unsigned i=0;i<3;++i)if(dirty[i]){
        memcpy((uint8_t*)mapped.pData+cursor,data[i],sizes[i]);
        memset((uint8_t*)mapped.pData+cursor+sizes[i],0,padded[i]-sizes[i]);
        a->first[i]=cursor/16;a->count[i]=padded[i]/16;cursor+=padded[i];
    }
    ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)a->buffer,0);
    a->cursor=cursor;a->valid=1;++a->maps;return 1;
}
static void nv_constant_arena_bind(NvConstantArena*a){
    NV_VERTEX_BINDING_INVALIDATE();
    ID3D11Buffer*buffers[3]={a->buffer,a->buffer,a->buffer};
    ID3D11DeviceContext1_VSSetConstantBuffers1(a->context,0,3,buffers,a->first,a->count);
}
#endif
