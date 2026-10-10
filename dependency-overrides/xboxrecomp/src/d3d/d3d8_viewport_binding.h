#ifndef D3D8_VIEWPORT_BINDING_H
#define D3D8_VIEWPORT_BINDING_H
/* Owned immediate-context, single valid viewport only. All viewport writers,
 * including temporary blits, must route here; unknown mutations invalidate. */
typedef struct D3D8ViewportBinding {
    ID3D11DeviceContext *context;
    D3D11_VIEWPORT viewport;
    unsigned valid;
    uint64_t requests,skipped;
} D3D8ViewportBinding;
static void d3d8_viewport_binding_clear(D3D8ViewportBinding*b){
    if(b->context)ID3D11DeviceContext_Release(b->context);
    memset(b,0,sizeof(*b));
}
static void d3d8_viewport_binding_set(D3D8ViewportBinding*b,ID3D11DeviceContext*c,
    UINT n,const D3D11_VIEWPORT*v,int enabled){
    if(!enabled||n!=1||!v){b->valid=0;ID3D11DeviceContext_RSSetViewports(c,n,v);return;}
    if(b->context!=c){d3d8_viewport_binding_clear(b);b->context=c;ID3D11DeviceContext_AddRef(c);}
    ++b->requests;
    if(b->valid&&!memcmp(&b->viewport,v,sizeof(*v))){++b->skipped;return;}
    ID3D11DeviceContext_RSSetViewports(c,n,v);
    /* Conservative finite, nonnegative range used by this runtime. Never cache
     * a possibly rejected host request; unknown ranges retain the raw API path. */
    b->valid=v->TopLeftX>=0&&v->TopLeftY>=0&&v->Width>=0&&v->Height>=0&&
        v->TopLeftX+v->Width<=16384&&v->TopLeftY+v->Height<=16384&&
        v->MinDepth>=0&&v->MaxDepth<=1&&v->MinDepth<=v->MaxDepth;
    if(b->valid)b->viewport=*v;
}
#endif
