#ifndef D3D8_TARGET_BINDING_H
#define D3D8_TARGET_BINDING_H
/* Single-RT immediate-context contract. Every OM writer must use this helper.
 * Present/unknown external state mutations invalidate it explicitly. Owned refs
 * prevent pointer reuse; no SRV bindings, queries, clears or draws are skipped. */
typedef struct D3D8TargetBinding {
    ID3D11DeviceContext *context;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    unsigned valid;
    uint64_t requests,skipped;
} D3D8TargetBinding;
static void d3d8_target_binding_clear(D3D8TargetBinding*b){
    if(b->rtv)ID3D11RenderTargetView_Release(b->rtv);
    if(b->dsv)ID3D11DepthStencilView_Release(b->dsv);
    if(b->context)ID3D11DeviceContext_Release(b->context);
    memset(b,0,sizeof(*b));
}
static void d3d8_target_binding_set(D3D8TargetBinding*b,ID3D11DeviceContext*c,
    UINT count,ID3D11RenderTargetView*const*views,ID3D11DepthStencilView*d,int enabled){
    if(!enabled||count!=1||!views){
        b->valid=0;ID3D11DeviceContext_OMSetRenderTargets(c,count,views,d);return;
    }
    if(b->context!=c){d3d8_target_binding_clear(b);b->context=c;ID3D11DeviceContext_AddRef(c);}
    ++b->requests;
    ID3D11RenderTargetView *r=views[0];
    if(b->valid&&b->rtv==r&&b->dsv==d){++b->skipped;return;}
    ID3D11DeviceContext_OMSetRenderTargets(c,count,views,d);
    if(r!=b->rtv){if(r)ID3D11RenderTargetView_AddRef(r);if(b->rtv)ID3D11RenderTargetView_Release(b->rtv);b->rtv=r;}
    if(d!=b->dsv){if(d)ID3D11DepthStencilView_AddRef(d);if(b->dsv)ID3D11DepthStencilView_Release(b->dsv);b->dsv=d;}
    b->valid=1;
}
#endif
