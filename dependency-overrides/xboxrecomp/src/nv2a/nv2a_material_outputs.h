#ifndef NV_MATERIAL_OUTPUTS_H
#define NV_MATERIAL_OUTPUTS_H
#include <d3d11shader.h>
/* Whole semantic conservatism: never prune a component of a live semantic.
 * Unknown signatures/reflection failures retain the full legacy contract. */
static unsigned nv_material_outputs(const void *code,size_t bytes){
    ID3D11ShaderReflection *r=NULL;D3D11_SHADER_DESC desc;
    unsigned mask=0;
    if(FAILED(D3DReflect(code,bytes,&IID_ID3D11ShaderReflection,(void**)&r)))return 127;
    if(FAILED(r->lpVtbl->GetDesc(r,&desc))){r->lpVtbl->Release(r);return 127;}
    for(unsigned i=0;i<desc.InputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC p;
        if(FAILED(r->lpVtbl->GetInputParameterDesc(r,i,&p))){mask=127;break;}
        if(!p.ReadWriteMask)continue;
        if(!_stricmp(p.SemanticName,"SV_POSITION"))continue;
        if(!_stricmp(p.SemanticName,"COLOR")&&p.SemanticIndex<2)mask|=1u<<p.SemanticIndex;
        else if(!_stricmp(p.SemanticName,"TEXCOORD")&&p.SemanticIndex<5)mask|=1u<<(p.SemanticIndex+2);
        else {mask=127;break;}
    }
    r->lpVtbl->Release(r);return mask;
}
#endif
