#ifndef NV2A_DRAW_STATE_RECORD_H
#define NV2A_DRAW_STATE_RECORD_H
#include "../d3d/d3d8_draw_state_record.h"
/* Indexed triangle-strip state only. Include after original enum translators.
 * Raw inputs are copied after depth-offset retirement. No dynamic resource key. */
typedef struct NvDrawStateKey {
    uint32_t depth,depth_func,depth_mask,stencil_enabled;
    uint32_t stencil[8];
    uint32_t cull_enabled,cull_face,front_face,fog;
    uint32_t blend,src,dst,alpha,alpha_func,alpha_ref,color_mask;
} NvDrawStateKey;
static void nv_draw_state_compile(D3D8DrawStateRecord*r,const NvDrawStateKey*k)
{
    memset(r,0,sizeof(*r));
#define NV_DS(s,v) do{unsigned i=r->count++;r->state[i]=(s);r->value[i]=(v);}while(0)
    NV_DS(D3DRS_ZENABLE,k->depth);
    NV_DS(D3DRS_ZFUNC,k->depth_func>=0x200&&k->depth_func<=0x207?k->depth_func-0x1FF:D3DCMP_ALWAYS);
    NV_DS(D3DRS_ZWRITEENABLE,!!k->depth_mask);NV_DS(D3DRS_LIGHTING,FALSE);
    if(k->stencil_enabled){
        NV_DS(D3DRS_STENCILENABLE,!!k->stencil[0]);
        NV_DS(D3DRS_STENCILFUNC,k->stencil[1]>=0x200&&k->stencil[1]<=0x207?k->stencil[1]-0x1FF:8);
        NV_DS(D3DRS_STENCILREF,k->stencil[2]);NV_DS(D3DRS_STENCILMASK,k->stencil[3]);
        NV_DS(D3DRS_STENCILWRITEMASK,k->stencil[4]);
        NV_DS(D3DRS_STENCILFAIL,nv_stencil_op(k->stencil[5]));
        NV_DS(D3DRS_STENCILZFAIL,nv_stencil_op(k->stencil[6]));
        NV_DS(D3DRS_STENCILPASS,nv_stencil_op(k->stencil[7]));
    }
    unsigned cull=nv_cull_mode(k->cull_enabled,k->cull_face,k->front_face);
    if(cull==4){r->culled=1;return;} /* Preserve writes preceding original cull. */
    r->unsupported_cull=!cull;
    NV_DS(D3DRS_CULLMODE,cull>=1&&cull<=3?cull:D3DCULL_NONE);
    NV_DS(D3DRS_FOGCOLOR,(k->fog&0xFF00FF00u)|((k->fog&255)<<16)|((k->fog>>16)&255));
    NV_DS(D3DRS_ALPHABLENDENABLE,k->blend);NV_DS(D3DRS_SRCBLEND,nv2a_blend_to_d3d(k->src));
    NV_DS(D3DRS_DESTBLEND,nv2a_blend_to_d3d(k->dst));NV_DS(D3DRS_ALPHATESTENABLE,k->alpha);
    NV_DS(D3DRS_ALPHAFUNC,k->alpha_func>=0x200&&k->alpha_func<=0x207?k->alpha_func-0x1FF:D3DCMP_ALWAYS);
    NV_DS(D3DRS_ALPHAREF,k->alpha_ref&255);
    NV_DS(D3DRS_COLORWRITEENABLE,((k->color_mask>>16)&1)|(((k->color_mask>>8)&1)<<1)|
        ((k->color_mask&1)<<2)|(((k->color_mask>>24)&1)<<3));
#undef NV_DS
}
typedef struct NvDrawStateCache {unsigned valid;NvDrawStateKey key;D3D8DrawStateRecord record;} NvDrawStateCache;
static const D3D8DrawStateRecord*nv_draw_state_prepare(NvDrawStateCache*c,const NvDrawStateKey*k){
    if(!c->valid||memcmp(&c->key,k,sizeof(*k))){nv_draw_state_compile(&c->record,k);c->key=*k;c->valid=1;}
    return &c->record;
}
#endif
