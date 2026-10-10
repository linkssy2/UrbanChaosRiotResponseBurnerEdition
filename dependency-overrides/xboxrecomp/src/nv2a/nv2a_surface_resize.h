#ifndef NV_SURFACE_RESIZE_H
#define NV_SURFACE_RESIZE_H
#ifndef NV_RESIZE_INVALIDATE
#define NV_RESIZE_INVALIDATE() ((void)0)
#endif
/* Apply-only migration. Keep guest identities and GPU contents; publish the
 * replacement cache only after every allocation/copy succeeds. */
#include <d3d11_1.h>
static int nv_surface_resize_depth(ID3D11Device *dev,ID3D11DeviceContext *ctx,
    NvSurfaceCache *old,NvSurface *src,NvSurface *dst){
    ID3D11ShaderResourceView *packed=nv_surface_physical_snapshot(old,dev,ctx,src);
    if(!packed)return 0;
    const char *code="Texture2D<float4> t:register(t0);cbuffer C:register(b0){uint bit;};float main(float4 p:SV_POSITION,float2 uv:TEXCOORD0):SV_Depth{uint w,h;t.GetDimensions(w,h);uint4 b=(uint4)round(t.Load(int3(clamp(int2(uv*float2(w,h)),int2(0,0),int2(w-1,h-1)),0))*255);if(bit&&!(b.a&bit))discard;return (b.r+256*b.g+65536*b.b)/16777215.0;}";
    ID3DBlob *blob=NULL;ID3D11PixelShader *ps=NULL;ID3D11Buffer *cb=NULL;int ok=0;
    HRESULT hr=D3DCompile(code,strlen(code),"resize_depth",NULL,NULL,"main","ps_4_0",0,0,&blob,NULL);
    if(SUCCEEDED(hr))hr=ID3D11Device_CreatePixelShader(dev,ID3D10Blob_GetBufferPointer(blob),ID3D10Blob_GetBufferSize(blob),NULL,&ps);
    D3D11_BUFFER_DESC bd={0};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    if(SUCCEEDED(hr))hr=ID3D11Device_CreateBuffer(dev,&bd,NULL,&cb);
    if(FAILED(hr))goto done;
    ID3D11DeviceContext_OMSetRenderTargets(ctx,0,NULL,dst->dsv);
    ID3D11DeviceContext_ClearDepthStencilView(ctx,dst->dsv,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,1,0);
    D3D11_VIEWPORT vp={0,0,(FLOAT)dst->physical_width,(FLOAT)dst->physical_height,0,1};
    ID3D11DeviceContext_RSSetViewports(ctx,1,&vp);ID3D11DeviceContext_RSSetState(ctx,old->blit_rs);
    ID3D11DeviceContext_IASetInputLayout(ctx,NULL);ID3D11DeviceContext_IASetPrimitiveTopology(ctx,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(ctx,old->blit_vs,NULL,0);ID3D11DeviceContext_PSSetShader(ctx,ps,NULL,0);
    ID3D11DeviceContext_GSSetShader(ctx,NULL,NULL,0);ID3D11DeviceContext_PSSetShaderResources(ctx,0,1,&packed);
    ID3D11DeviceContext_PSSetConstantBuffers(ctx,0,1,&cb);ID3D11DeviceContext_OMSetBlendState(ctx,NULL,NULL,UINT32_MAX);
    for(unsigned pass=0;pass<9;pass++){
        unsigned bits[4]={pass?1u<<(pass-1):0,0,0,0};ID3D11DepthStencilState *ds=NULL;
        D3D11_DEPTH_STENCIL_DESC d={0};d.DepthEnable=TRUE;d.DepthWriteMask=pass?D3D11_DEPTH_WRITE_MASK_ZERO:D3D11_DEPTH_WRITE_MASK_ALL;d.DepthFunc=D3D11_COMPARISON_ALWAYS;
        d.StencilEnable=pass!=0;d.StencilReadMask=255;d.StencilWriteMask=(UINT8)bits[0];
        d.FrontFace.StencilFunc=D3D11_COMPARISON_ALWAYS;d.FrontFace.StencilPassOp=D3D11_STENCIL_OP_REPLACE;
        d.FrontFace.StencilFailOp=d.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_KEEP;d.BackFace=d.FrontFace;
        if(FAILED(ID3D11Device_CreateDepthStencilState(dev,&d,&ds)))goto done;
        ID3D11DeviceContext_UpdateSubresource(ctx,(ID3D11Resource*)cb,0,NULL,bits,0,0);
        ID3D11DeviceContext_OMSetDepthStencilState(ctx,ds,255);ID3D11DeviceContext_Draw(ctx,3,0);ID3D11DepthStencilState_Release(ds);
    }
    ok=1;
done:
    if(blob)ID3D10Blob_Release(blob);if(ps)ID3D11PixelShader_Release(ps);if(cb)ID3D11Buffer_Release(cb);return ok;
}
static int nv_surface_resize(NvSurfaceCache *cache,ID3D11Device *dev,ID3D11DeviceContext *ctx,int (*persist)(void*),void *opaque){
    ID3D11Device1 *d1=NULL;ID3D11DeviceContext1 *c1=NULL;ID3DDeviceContextState *isolated=NULL,*prior=NULL;
    NvSurfaceCache next={0};int ok=0;D3D_FEATURE_LEVEL level=ID3D11Device_GetFeatureLevel(dev),selected;
    if(FAILED(ID3D11Device_QueryInterface(dev,&IID_ID3D11Device1,(void**)&d1))||
       FAILED(ID3D11DeviceContext_QueryInterface(ctx,&IID_ID3D11DeviceContext1,(void**)&c1)))goto done;
    if(FAILED(ID3D11Device1_CreateDeviceContextState(d1,0,&level,1,D3D11_SDK_VERSION,&IID_ID3D11Device,&selected,&isolated)))goto done;
    ID3D11DeviceContext1_SwapDeviceContextState(c1,isolated,&prior);NV_RESIZE_INVALIDATE();
    for(unsigned depth=0;depth<2;depth++)for(NvSurfaceIterator it=nv_surface_iter(cache,depth);it.surface;nv_surface_iter_next(&it)){
        NvSurface *s=it.surface;if(!s->texture)continue;
        NV_RESIZE_INVALIDATE();
        NvSurface *n=nv_surface_get(&next,dev,depth,s->offset,s->pitch,s->width,s->height,NULL,(size_t)s->offset+(size_t)s->pitch*s->height);
        if(!n)goto restore;
        if(depth){if(!nv_surface_resize_depth(dev,ctx,cache,s,n))goto restore;}
        else {NvSurface copy=*s;copy.width=n->physical_width;copy.height=n->physical_height;
            if(!nv_surface_blit(&next,dev,ctx,&copy,n->rtv,n->physical_width,n->physical_height))goto restore;}
    }
    ok=SUCCEEDED(ID3D11Device_GetDeviceRemovedReason(dev));
restore:
    ID3D11DeviceContext_ClearState(ctx);ID3D11DeviceContext1_SwapDeviceContextState(c1,prior,NULL);NV_RESIZE_INVALIDATE();
    if(ok&&persist)ok=persist(opaque);
    if(ok){nv_surface_shutdown(cache);*cache=next;memset(&next,0,sizeof next);}
done:
    nv_surface_shutdown(&next);if(prior)ID3DDeviceContextState_Release(prior);if(isolated)ID3DDeviceContextState_Release(isolated);
    if(c1)ID3D11DeviceContext1_Release(c1);if(d1)ID3D11Device1_Release(d1);return ok;
}

#endif
