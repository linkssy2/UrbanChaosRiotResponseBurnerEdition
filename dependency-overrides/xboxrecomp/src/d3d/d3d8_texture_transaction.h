#ifndef D3D8_TEXTURE_TRANSACTION_H
#define D3D8_TEXTURE_TRANSACTION_H
/* Single-context transaction, not a cache. Commit before resource/target
 * observers or writers. Pending views retain ownership across cache eviction. */
typedef struct D3D8TextureTransaction {
    ID3D11ShaderResourceView *views[4];
    unsigned dirty, active;
} D3D8TextureTransaction;
static void d3d8_texture_transaction_commit(D3D8TextureTransaction*t,ID3D11DeviceContext*c){
    for(unsigned i=0;i<4;){
        if(!(t->dirty&(1u<<i))){++i;continue;}
        unsigned first=i;while(i<4&&(t->dirty&(1u<<i)))++i;
        ID3D11DeviceContext_PSSetShaderResources(c,first,i-first,t->views+first);
    }
    for(unsigned i=0;i<4;++i)if(t->dirty&(1u<<i)){
        if(t->views[i])ID3D11ShaderResourceView_Release(t->views[i]);
        t->views[i]=NULL;
    }
    t->dirty=0;
}
static void d3d8_texture_transaction_begin(D3D8TextureTransaction*t,ID3D11DeviceContext*c){
    d3d8_texture_transaction_commit(t,c);t->active=1;
}
static void d3d8_texture_transaction_set(D3D8TextureTransaction*t,ID3D11DeviceContext*c,unsigned slot,ID3D11ShaderResourceView*v){
    if(slot>=4)return;
    if(!t->active){ID3D11DeviceContext_PSSetShaderResources(c,slot,1,&v);return;}
    if(v)ID3D11ShaderResourceView_AddRef(v);
    if(t->views[slot])ID3D11ShaderResourceView_Release(t->views[slot]);
    t->views[slot]=v;t->dirty|=1u<<slot;
}
static void d3d8_texture_transaction_end(D3D8TextureTransaction*t,ID3D11DeviceContext*c){
    d3d8_texture_transaction_commit(t,c);t->active=0;
}
#endif
