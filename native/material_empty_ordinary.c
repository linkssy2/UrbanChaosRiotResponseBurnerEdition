#include "material_empty_ordinary.h"
int uc_material_empty_ordinary(const UCMaterialAlphaPrepass*p,UCMaterialEmptyState s,
    const UCMaterialEmptyHooks*h,void*c){
    uint32_t bits;const UCMaterialEmptyContext*current;
    if(!p||p->resource_index||p->cull_none>1||!s.alpha_active||!s.bias_bits||!s.dirty||
       !s.lod_bits||s.dirty==s.lod_bits||s.dirty==s.bias_bits||s.lod_bits==s.bias_bits||
       !h||!h->transform||!h->context)return 0;
    h->transform(c);bits=*s.alpha_active?0:*s.bias_bits;
    current=h->context(c);if(!current)return 0;
    if(current->override_enabled){current=h->context(c);if(!current)return 0;bits=current->lod_bits;}
    *s.dirty|=1;*s.lod_bits=bits;return 1;
}
