#ifndef UCRR_VERTEX_BINDING_OWNER_H
#define UCRR_VERTEX_BINDING_OWNER_H
/* Ordered immediate-context bindings only, never resource contents.
 * Every external writer must invalidate before reuse. Owned references avoid
 * pointer recycling even if a hostile writer replaces a context binding. */
typedef struct VertexBindingOwner {
    ID3D11DeviceContext *context;
    ID3D11VertexShader *shader;
    ID3D11Buffer *program,*conversion;
    D3D11_PRIMITIVE_TOPOLOGY topology;
    unsigned valid;
    uint64_t requests,skipped;
} VertexBindingOwner;
static void vertex_binding_clear(VertexBindingOwner*b){
    if(b->shader)ID3D11VertexShader_Release(b->shader);
    if(b->program)ID3D11Buffer_Release(b->program);
    if(b->conversion)ID3D11Buffer_Release(b->conversion);
    if(b->context)ID3D11DeviceContext_Release(b->context);
    memset(b,0,sizeof(*b));
}
static void vertex_binding_set(VertexBindingOwner*b,ID3D11DeviceContext*c,
    ID3D11VertexShader*v,ID3D11Buffer*p,ID3D11Buffer*q,
    D3D11_PRIMITIVE_TOPOLOGY topology,ID3D11ShaderResourceView*vertices,int enabled){
    ID3D11Buffer*buffers[2]={p,q};
    if(!enabled){
        b->valid=0;
        ID3D11DeviceContext_VSSetShader(c,v,NULL,0);
        ID3D11DeviceContext_VSSetConstantBuffers(c,0,2,buffers);
        ID3D11DeviceContext_VSSetShaderResources(c,7,1,&vertices);
        ID3D11DeviceContext_IASetInputLayout(c,NULL);
        ID3D11DeviceContext_IASetPrimitiveTopology(c,topology);
        return;
    }
    if(c!=b->context){vertex_binding_clear(b);b->context=c;ID3D11DeviceContext_AddRef(c);}
    b->requests+=4;
    if(b->valid&&v==b->shader)++b->skipped;
    else ID3D11DeviceContext_VSSetShader(c,v,NULL,0);
    if(b->valid&&p==b->program&&q==b->conversion)++b->skipped;
    else ID3D11DeviceContext_VSSetConstantBuffers(c,0,2,buffers);
    ID3D11DeviceContext_VSSetShaderResources(c,7,1,&vertices);
    if(b->valid)++b->skipped;
    else ID3D11DeviceContext_IASetInputLayout(c,NULL);
    if(b->valid&&topology==b->topology)++b->skipped;
    else ID3D11DeviceContext_IASetPrimitiveTopology(c,topology);
    if(v!=b->shader){if(v)ID3D11VertexShader_AddRef(v);if(b->shader)ID3D11VertexShader_Release(b->shader);b->shader=v;}
    if(p!=b->program){if(p)ID3D11Buffer_AddRef(p);if(b->program)ID3D11Buffer_Release(b->program);b->program=p;}
    if(q!=b->conversion){if(q)ID3D11Buffer_AddRef(q);if(b->conversion)ID3D11Buffer_Release(b->conversion);b->conversion=q;}
    b->topology=topology;b->valid=1;
}
#endif
