#ifndef NV_GAMEPLAY_BATCH_H
#define NV_GAMEPLAY_BATCH_H
#include "nv2a_regs.h"
static int nv_gameplay_batch_method_barrier(unsigned mode,int graphics_class,uint32_t method,uint32_t param){
    if(!graphics_class)return 1;
    if(method==NV097_ARRAY_ELEMENT16||method==NV097_ARRAY_ELEMENT32)return 0;
    if(method==NV097_SET_BEGIN_END)return param&&param!=mode;
    return 1;
}
/* Raw packets own their vertex inputs. Zero Kelvin NOP changes no effective
 * pipeline/resource state. Nonzero NOP can run guest fixups and MUST still end
 * the group. Keep the packed gameplay policy and all ownership barriers intact. */
static int nv_raw_batch_method_barrier(unsigned mode,int graphics_class,uint32_t method,uint32_t param,int retain_zero_nop){
    if(retain_zero_nop&&graphics_class&&method==NV097_NO_OPERATION&&param==0)return 0;
    return nv_gameplay_batch_method_barrier(mode,graphics_class,method,param);
}
/* Immediate-context-owner only. Caller owns the unchanged pipeline interval;
 * no guest memory, texture updates, queries or target changes may cross it. */
#define NV_GAMEPLAY_BATCH_MAX 8
typedef struct NvGameplayDraw {unsigned vertices,indices,primitive_count,vertex_offset,index_offset;} NvGameplayDraw;
typedef struct NvGameplayBatch {
    NvVertexProgramData program;
    NvVertexSnapshotLayout layout;
    float conversion[6][4];
    unsigned mode,primitive,outputs,count,vertex_bytes,index_count;
    NvGameplayDraw draw[NV_GAMEPLAY_BATCH_MAX];
    uint8_t vertices[16384*256];uint16_t indices[16384];
    uint64_t groups,draws,maps,singles,failures,fallbacks;
} NvGameplayBatch;
static int nv_gameplay_batch_append(NvGameplayBatch*b,const void*vertices,unsigned count,
    const uint16_t*indices,unsigned index_count,unsigned prim_count,const NvVertexSnapshotLayout*layout){
    if(!vertices||!indices||!count||count>16384||index_count<3||index_count>16384||!prim_count||
       !layout->stride||layout->stride>256||(layout->stride&3)||b->count>=NV_GAMEPLAY_BATCH_MAX)return 0;
    unsigned bytes=count*layout->stride;
    if(b->vertex_bytes>sizeof(b->vertices)-bytes||b->index_count>16384-index_count)return 0;
    if(b->count&&memcmp(&b->layout,layout,sizeof(*layout)))return 0;
    for(unsigned n=0;n<index_count;++n)if(indices[n]>=count)return 0;
    if(!b->count)b->layout=*layout;
    NvGameplayDraw*d=&b->draw[b->count];d->vertices=count;d->indices=index_count;d->primitive_count=prim_count;
    d->vertex_offset=b->vertex_bytes;d->index_offset=b->index_count;
    memcpy(b->vertices+b->vertex_bytes,vertices,bytes);memcpy(b->indices+b->index_count,indices,index_count*2);
    b->vertex_bytes+=bytes;b->index_count+=index_count;++b->count;return 1;
}
typedef int (*NvGameplayBatchDraw)(void*,NvVertexGPU*,const NvGameplayDraw*,unsigned,const UINT*,const UINT*,ID3D11VertexShader*,ID3D11VertexShader*);
static void nv_gameplay_batch_invalidate(NvVertexGPU*g){
    g->program_valid=g->conversion_valid=0;g->arena.valid=0;g->arena.cursor=0;
}
static unsigned nv_gameplay_batch_flush(NvGameplayBatch*b,NvVertexGPU*g,ID3D11DeviceContext*ctx,NvGameplayBatchDraw draw,void*user){
    if(!b->count)return 0;
    unsigned success=0,vertex_base=g->vertex_cursor,index_base=g->index_cursor;
    unsigned constant_bytes=5632+b->count*768,constant_base=g->arena.cursor;
    if(vertex_base>sizeof(b->vertices)-b->vertex_bytes)vertex_base=0;
    if(index_base>16384-b->index_count)index_base=0;
    if(constant_base>65536-constant_bytes)constant_base=0;
    ID3D11Buffer*buffers[3]={g->vertices,g->indices,g->arena.buffer};
    /* This uploader bypasses upload_core; conservatively retire resident
     * ranges before it can overwrite or DISCARD their backing allocation. */
    nv_geometry_expire(&g->geometry);
    nv_geometry_expire(&g->index_geometry);
    unsigned base[3]={vertex_base,index_base*2,constant_base};
    int uploaded=1;
    /* Do not issue any draw until ALL uploads succeed. Failed upload can replay
     * owned packets through the old path without duplicating earlier draws. */
    for(unsigned n=0;n<3;++n){D3D11_MAPPED_SUBRESOURCE m={0};
        if(FAILED(ID3D11DeviceContext_Map(ctx,(ID3D11Resource*)buffers[n],0,base[n]?D3D11_MAP_WRITE_NO_OVERWRITE:D3D11_MAP_WRITE_DISCARD,0,&m))){uploaded=0;++b->failures;break;}
        ++b->maps;uint8_t*out=(uint8_t*)m.pData+base[n];
        if(n==0)memcpy(out,b->vertices,b->vertex_bytes);
        else if(n==1)memcpy(out,b->indices,b->index_count*2);
        else {
            memset(out,0,constant_bytes);memcpy(out,&b->program,sizeof(b->program));memcpy(out+5376,b->conversion,sizeof(b->conversion));
            for(unsigned k=0;k<b->count;++k){uint8_t*meta=out+5632+k*768;
                memcpy(meta,b->layout.attribute,256);memcpy(meta+256,b->layout.defaults,256);
                uint32_t params[4]={b->layout.stride,vertex_base+b->draw[k].vertex_offset,b->draw[k].vertices,0};memcpy(meta+512,params,16);
            }
        }
        ID3D11DeviceContext_Unmap(ctx,(ID3D11Resource*)buffers[n],0);
    }
    ++b->groups;b->draws+=b->count;if(b->count==1)++b->singles;
    g->material_outputs=b->outputs;
    ID3D11VertexShader*shader=nv_vertex_gpu_select_layout(g,ctx,b->program.p,b->program.params[0],&b->layout),*full=shader;
    if(g->material_outputs_enabled&&b->outputs!=127){g->material_outputs=127;full=nv_vertex_gpu_select_layout(g,ctx,b->program.p,b->program.params[0],&b->layout);g->material_outputs=b->outputs;}
    if(uploaded){
        g->vertex_cursor=vertex_base+b->vertex_bytes;g->index_cursor=index_base+b->index_count;
        const UINT counts[3]={336,16,48};
        for(unsigned n=0;n<b->count;++n){UINT first[3]={constant_base/16,(constant_base+5376)/16,(constant_base+5632+n*768)/16};
            success+=!!draw(user,g,&b->draw[n],index_base+b->draw[n].index_offset,first,counts,shader,full);
        }
    }else{
        ++b->fallbacks;g->vertex_cursor=g->index_cursor=0;nv_gameplay_batch_invalidate(g);
        for(unsigned n=0;n<b->count;++n){NvGameplayDraw*d=&b->draw[n];
            if(nv_vertex_gpu_upload_core(g,ctx,b->program.p,b->program.params[0],b->program.c,b->vertices+d->vertex_offset,d->vertices,b->conversion,&b->layout)&&
               nv_vertex_gpu_upload_indices16(g,ctx,b->indices+d->index_offset,d->indices))
                success+=!!draw(user,g,d,g->index_first,g->arena.first,g->arena.count,g->active_shader,g->full_shader);
        }
    }
    nv_gameplay_batch_invalidate(g);
    b->count=b->vertex_bytes=b->index_count=0;return success;
}
#endif
