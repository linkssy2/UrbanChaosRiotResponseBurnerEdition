#ifndef D3D8_PIXEL_BINDING_H
#define D3D8_PIXEL_BINDING_H
/* Immediate-context owner only; all PS writers (including temporary blits)
 * must pass through this cache. Only zero-class-instance bindings are used.
 * Owned references prevent identity reuse. Clear before context teardown. */
typedef struct D3D8PixelBinding {ID3D11DeviceContext *context;ID3D11PixelShader *shader;unsigned valid;uint64_t requests,skipped;} D3D8PixelBinding;
static void d3d8_pixel_binding_clear(D3D8PixelBinding*b){
    if(b->shader)ID3D11PixelShader_Release(b->shader);
    if(b->context)ID3D11DeviceContext_Release(b->context);
    memset(b,0,sizeof(*b));
}
static void d3d8_pixel_binding_set(D3D8PixelBinding*b,ID3D11DeviceContext*c,ID3D11PixelShader*p,int enabled){
    if(!enabled){ID3D11DeviceContext_PSSetShader(c,p,NULL,0);return;}
    if(b->context!=c){d3d8_pixel_binding_clear(b);b->context=c;ID3D11DeviceContext_AddRef(c);}
    ++b->requests;
    if(b->valid&&b->shader==p){++b->skipped;return;}
    ID3D11DeviceContext_PSSetShader(c,p,NULL,0);
    if(p!=b->shader){if(p)ID3D11PixelShader_AddRef(p);if(b->shader)ID3D11PixelShader_Release(b->shader);b->shader=p;}
    b->valid=1;
}
#endif
