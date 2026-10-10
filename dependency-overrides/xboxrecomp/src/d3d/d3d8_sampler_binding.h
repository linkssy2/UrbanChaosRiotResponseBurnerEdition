#ifndef D3D8_SAMPLER_BINDING_H
#define D3D8_SAMPLER_BINDING_H
/* Fixed-mode, immediate-context-owner cache. All PS sampler writes must use
 * this helper or clear it. Owned references protect identity/lifetime. */
typedef struct D3D8SamplerBinding {ID3D11DeviceContext*context;ID3D11SamplerState*samplers[4];unsigned valid;uint64_t requests,skipped;} D3D8SamplerBinding;
static void d3d8_sampler_binding_clear(D3D8SamplerBinding*b){
 for(unsigned i=0;i<4;++i)if(b->samplers[i])ID3D11SamplerState_Release(b->samplers[i]);
 if(b->context)ID3D11DeviceContext_Release(b->context);memset(b,0,sizeof(*b));
}
static void d3d8_sampler_binding_set(D3D8SamplerBinding*b,ID3D11DeviceContext*c,unsigned stage,ID3D11SamplerState*s,int enabled){
 if(!enabled||stage>=4){ID3D11DeviceContext_PSSetSamplers(c,stage,1,&s);return;}
 if(b->context!=c){d3d8_sampler_binding_clear(b);b->context=c;ID3D11DeviceContext_AddRef(c);}
 ++b->requests;
 if((b->valid&(1u<<stage))&&b->samplers[stage]==s){++b->skipped;return;}
 ID3D11DeviceContext_PSSetSamplers(c,stage,1,&s);
 if(b->samplers[stage]!=s){if(s)ID3D11SamplerState_AddRef(s);if(b->samplers[stage])ID3D11SamplerState_Release(b->samplers[stage]);b->samplers[stage]=s;}
 b->valid|=1u<<stage;
}
#endif
