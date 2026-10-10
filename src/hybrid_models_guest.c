#include "hybrid_models_guest.h"
#include "hybrid_guest_cpu.h"
#include "hybrid_models_sampler.h"
#include "xbox_memory_layout.h"
#include "../native/control_model_receiver.h"
#include "../native/point_light_vertex.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
extern void sub_000A24A0(void);
static uint64_t epoch,counts[12];
static UCModelRenderPalette palette;
static unsigned initialized,enabled,palette_ready;
static DWORD thread;
static uint32_t *indices;static size_t index_capacity;
static UcHybridModelVertex *vertices;static size_t vertex_capacity;
static UcHybridModelIndexedVertex *indexed_vertices;static size_t indexed_capacity;
static uint32_t vertex_map[65536];
static int indexed_enabled(void){
    static int ready,on;if(!ready){const char*v=getenv("UCRR_HYBRID_MODELS_INDEXED");on=v&&!strcmp(v,"1");ready=1;}return on;
}
static int base_indexed_enabled(void){
    static int ready,on;if(!ready){const char*v=getenv("UCRR_HYBRID_BASE_INDEXED");on=v&&!strcmp(v,"1");ready=1;}return on;
}
static const unsigned char *span(uint32_t va,size_t bytes){
    uint32_t p=va;if(va>=0x80000000u&&va<0x84000000u)p=va-0x80000000u;else if(va>=0x80000000u)return NULL;
    if(p<0x10000u||bytes>g_xbox_total_ram||(uint64_t)p+bytes>g_xbox_total_ram)return NULL;
    return (const unsigned char*)XBOX_PTR(p);
}
static uint32_t word(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t read32(uint32_t p){const void*s=span(p,4);return s?word(s):0;}
static int finite_values(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if((word(p+i)&0x7FFFFFFFu)>=0x7F800000u)return 0;return 1;}
static int member(uint32_t receiver,uint32_t light){
    uint32_t head=read32(receiver+0x104),at=read32(head+4);
    for(unsigned i=0;i<256;i++){
        const unsigned char*n=span(at,0x12);if(!n)return 0;if(n[0x11])return 0;
        uint32_t key=word(n+0xC);if(key==light)return 1;at=word(n+(key<light?8:0));
    }return 0;
}
static int ordinary_modifier(uint32_t service,uint32_t receiver,uint32_t light){
    /* A1390: four ordered groups of 0x110-byte receiver/light records. The
     * service existing does not imply this pair has a projected modifier. */
    if(!service)return 1;
    const unsigned char*m=span(service,0x28);if(!m||word(m)!=0x456D08u)return 0;
    const unsigned char*counts=span(word(m+0x20),16),*groups=span(word(m+0x24),16);
    if(!counts||!groups)return 0;
    for(unsigned i=0;i<4;i++){
        int32_t count=(int32_t)word(counts+4*i);if(count<=0)continue;if(count>32768)return 0;
        const unsigned char*records=span(word(groups+4*i),(size_t)count*0x110);if(!records)return 0;
        for(int32_t j=0;j<count;j++){
            const unsigned char*r=records+(size_t)j*0x110;
            if(word(r)==receiver&&word(r+4)==light)return word(r+0x100)==1;
        }
    }return 1;
}
static int reserve(void **p,size_t *capacity,size_t needed,size_t stride){
    if(needed<=*capacity)return 1;if(needed>1048576)return 0;
    void*q=realloc(*p,needed*stride);if(!q)return 0;*p=q;*capacity=needed;return 1;
}
static const unsigned char *data(uint32_t descriptor,size_t bytes){
    const unsigned char*d=span(descriptor,12);return d?span(word(d+4)&0x0FFFFFFFu,bytes):NULL;
}
static int update_palette(uint32_t header,uint32_t bones,uint32_t bone_count,UCModelRenderPalette*pal){
    if(!header)return 1;const unsigned char*h=span(header,8);if(!h)return 0;
    int32_t count=(int32_t)word(h);if(count<=0)return 1;if(count>4096)return 0;
    const unsigned char*u=span(word(h+4),(size_t)count*4);if(!u)return 0;
    for(int32_t i=0;i<count;i++){
        unsigned bone=half(u+i*4);int row=96+(int8_t)u[i*4+2];
        const unsigned char*b=bone<bone_count?span(bones+48*bone,48):NULL;
        if(!b||row<0||row>189||!finite_values((const float*)b,12))return 0;
        memcpy(pal->rows[row],b,48);memset(pal->valid+row,1,3);
    }return 1;
}
static int pose(const unsigned char*position,const unsigned char*skin,const UCModelRenderPalette*pal,UcHybridModelVertex*v){
    UCControlModelVertex value;
    if(skin){if(!uc_control_model_vertex(position,skin,pal,&value))return 0;}
    else{
        UCPointLightVertexInput in={0};UCPointLightVertexConstants c={0};UCPointLightVertexOutput out;
        memcpy(in.position,position,12);in.packed_normal=word(position+12);
        for(unsigned i=0;i<4;i++)c.clip_rows[i][i]=1;
        if(!uc_point_light_vertex(&in,&c,&out))return 0;
        memcpy(value.position,out.clip,12);memcpy(value.normal,out.normal,12);
    }
    memcpy(v->position,value.position,12);memcpy(v->normal,value.normal,12);return 1;
}
static int indexed_geometry(UcHybridModelBatch*b,const unsigned char*p,size_t bytes,uint32_t nvertices,
    const unsigned char*pos,const unsigned char*skin,const unsigned char*uv,const unsigned char*basis,const UCModelRenderPalette*pal){
    size_t count=0;uint32_t primitive=0;
    if(!uc_control_model_indexed(p,bytes,nvertices,NULL,0,&count,&primitive)||
       !reserve((void**)&indices,&index_capacity,count,sizeof(*indices))||
       !reserve((void**)&indexed_vertices,&indexed_capacity,count<nvertices?count:nvertices,sizeof(*indexed_vertices))||
       !uc_control_model_indexed(p,bytes,nvertices,indices,index_capacity,&count,&primitive))return 0;
    memset(vertex_map,0xff,nvertices*sizeof(*vertex_map));unsigned char checked[192]={0};uint32_t unique=0;
    for(size_t k=0;k<count;k++){
        uint32_t at=indices[k],mapped=vertex_map[at];
        if(mapped==UINT32_MAX){
            UcHybridModelIndexedVertex*v=indexed_vertices+unique;memset(v,0,sizeof(*v));
            const unsigned char*source=pos+(size_t)at*(skin?10:16);
            if(skin){
                const unsigned char*weights=skin+at*8;
                for(unsigned axis=0;axis<3;axis++){
                    int value=(int16_t)half(source+axis*2),offset=(int16_t)half(weights+axis*2);
                    if(offset< -96||offset>93)return 0;unsigned row=(unsigned)(offset+96);
                    if(!checked[row]){
                        if(!pal->valid[row]||!pal->valid[row+1]||!pal->valid[row+2]||!finite_values(pal->rows[row],12))return 0;
                        checked[row]=1;
                    }
                    v->position[axis]=value==INT16_MIN?-1.f:(float)value/32767.f;v->rows|=row<<(axis*8);
                }
                v->weights=half(weights+6);v->normal=word(source+6);
            }else {memcpy(v->position,source,12);if(!finite_values(v->position,3))return 0;v->normal=word(source+12);}
            if(uv)memcpy(v->uv,uv+at*8,8);if(!finite_values(v->uv,2))return 0;
            if(basis)memcpy(v->basis,basis+at*8,8);
            mapped=unique++;vertex_map[at]=mapped;
        }
        indices[k]=mapped;
    }
    b->indexed_vertices=indexed_vertices;b->indices=indices;b->index_count=(uint32_t)count;
    b->vertex_count=unique;b->primitive=primitive;b->palette=pal;return 1;
}
static int projection(const UcHybridLightingScene*s,const unsigned char*r,UcHybridModelBatch*b){
    memcpy(b->projection,s->projection,64);
    if(b->flags&0x100){const unsigned char*p=span(s->matrix_provider+0x110,64);if(!p)return 0;memcpy(b->projection,p,64);}
    if(b->flags&0x80){float low,high;memcpy(&low,r+0x110,4);memcpy(&high,r+0x114,4);
        if(!finite_values(&low,1)||!finite_values(&high,1))return 0;
        for(unsigned i=0;i<4;i++)b->projection[4*i+2]=b->projection[4*i+3]*low+b->projection[4*i+2]*(high-low);
    }return finite_values(b->projection,16);
}
static int visible(uint32_t manager,uint32_t handle){
    /* The original query-only owner preserves pending/wait/report generations.
     * Calls use dead stack below current ESP; no original light pass executes. */
    if(g_esp<0x12000u||g_esp>=0x80000000u||manager>=0x80000000u)return 0;
    const unsigned char*scratch=span(g_esp-8192,8192);if(!scratch)return 0;
    HybridCPU cpu;save_cpu(&cpu);
    unsigned char old[8192];memcpy(old,scratch,8192);
    g_ecx=manager;PUSH32(g_esp,handle);PUSH32(g_esp,0);sub_000A24A0();
    int result=g_eax!=0;memcpy((void*)scratch,old,8192);restore_cpu(&cpu);return result;
}
static int material(uint32_t va,UcHybridModelBatch*b){
    const unsigned char*m=span(va,0x50);if(!m||word(m)!=0x4565B0u)return 0;
    b->material_va=va;b->material_flags=half(m+0x4C);b->material_mode=m[0x24];
    memcpy(&b->specular_strength,m+0x44,4);
    if(!finite_values(&b->specular_strength,1)||
       ((b->material_flags&0x222u)&&!(b->material_flags==0x25u&&b->material_mode==1)))return 0;
    b->setup_bytes=word(m+0xC);b->setup_count=b->setup_bytes;
    if(b->setup_bytes>sizeof(b->setup))return 0;
    if(b->setup_bytes){const unsigned char*p=span(word(m+0x18),b->setup_bytes);if(!p)return 0;memcpy(b->setup,p,b->setup_bytes);}
    return 1;
}
static int texture(uint32_t image,UcHybridModelBatch*b){
    /* 96390 binds the current IFL child without advancing it. 961C0 then
     * chooses the live image texture, or its fallback, for 3E3840. */
    b->texture_image_va=image;
    for(unsigned depth=0;depth<16;depth++){
        const unsigned char*p=span(image,0x18);if(!p)return 0;
        if(word(p)==0x4568D4u){
            uint32_t header=word(p+0x10);if(!header)header=word(p+0x14);
            const unsigned char*t=span(header,20);if(!t)return 0;
            const unsigned char*state=span(0x3F3DA0,52),*device=span(read32(0x3F4238),0x96C);
            uint32_t index=read32(0x3F3E10)&65535u,raw[13],sampler[3];float bias;
            if(!state||!device||index>15)return 0;memcpy(raw,state,52);memcpy(&bias,device+0x968,4);
            if(!uc_hybrid_models_sampler(raw,bias,read32(0x3F4128+index*4),sampler))return 0;
            b->texture_header_va=header;memcpy(b->texture_header,t,20);
            b->texture_address=sampler[0];b->texture_filter=sampler[1];b->texture_control0=sampler[2];return 1;
        }
        if(word(p)!=0x456910u)return 0;
        uint32_t index=word(p+0xC),count=word(p+0x10);
        if(!count||count>65536||index>=count)return 0;
        const unsigned char*list=span(word(p+0x14),(size_t)count*4);if(!list)return 0;
        image=word(list+index*4);
    }return 0;
}
struct LightCall {const UcHybridLightingScene*s;const UcHybridLightingRecord*l;};
static int submit(void*p,const UcHybridModelBatch*b){
    struct LightCall*c=p;++counts[4];int result=uc_hybrid_models_host_draw(c->s,c->l,b);
    _mm_setcsr(0x1F80); /* A host callback must not change later source pose math. */
    if(result>0){++counts[5];counts[6]+=b->vertex_count;}else ++counts[8];return result>=0;
}
static int receiver(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,uint32_t va,uint32_t partition){
    const unsigned char*r=span(va,0x164);if(!r||word(r)!=0x457880u)return 0;
    ++counts[2];if(partition&&partition!=UINT32_MAX){++counts[9];return 1;}
    uint32_t flags=word(r+0x60);if(flags&4||!member(va,l->source_va)){++counts[3];return 1;}
    if(flags&0x30u){++counts[9];return 1;} /* Separate viewport zero/one policy. */
    if(!partition){
        uint32_t query=word(r+0xB4);if(!query){++counts[3];return 1;}
        const unsigned char*g=span(s->manager,0x60),*q=g?span(word(g+0x5C),12):NULL;
        if(!q||word(q)!=0x456D38u)return 0;
        const unsigned char*active=span(0x4F88A8,2);if(!active)return 0;
        if(active[0]&&!visible(word(g+0x5C),query)){++counts[3];return 1;}
    }
    if(!r[0xB1]&&(flags&1)&&!ordinary_modifier(read32(s->manager+0x40),va,l->source_va)){++counts[9];return 1;}
    UcHybridModelPass pass={partition,(uint16_t)l->source_index,r[0xB1]!=0,r[0xB1]!=0,1,1};
    struct LightCall call={s,l};return uc_hybrid_models_geometry(s,va,&pass,&palette,submit,&call);
}
int uc_hybrid_models_geometry(const UcHybridLightingScene*s,uint32_t va,const UcHybridModelPass*pass,
    UCModelRenderPalette*pal,UcHybridModelBatchVisitor callback,void*owner){
    const unsigned char*r=span(va,0x164);if(!s||!pass||!pal||!callback||!r||word(r)!=0x457880u)return 0;
    uint32_t partition=pass->partition,flags=word(r+0x60);if(partition&&partition!=UINT32_MAX)return 0;
    UcHybridModelBatch b={0};b.receiver_va=va;b.partition=partition;b.flags=flags;
    b.resource_va=word(r+0x68);const unsigned char*resource=span(b.resource_va,0x3C);if(!resource)return 0;
    uint32_t group=half(r+0x78),node_index=half(r+0x7A),groups=word(resource+0x38);
    const unsigned char*g=span(groups+group*0x38u,0x38);if(!g)return 0;
    b.node_va=word(g+0x34)+node_index*0x70u;const unsigned char*n=span(b.node_va,0x70);if(!n)return 0;
    uint32_t nvertices=word(n+8),nbatches=word(n+0xC),bundle=word(n+0x20),skin_va=word(n+0x3C);
    if(!nvertices||nvertices>65536||nbatches>4096||
        (pass->single_batch&&(pass->node_va!=b.node_va||pass->batch_index>=nbatches)))return 0;b.skinned=skin_va!=0;
    const unsigned char*pos=data(bundle,(size_t)nvertices*(b.skinned?10:16)),*uv=data(bundle+0x24,(size_t)nvertices*8);
    const unsigned char*basis=data(bundle+0xC,(size_t)nvertices*8),*color=data(bundle+0x18,(size_t)nvertices*4);
    const unsigned char*skin=b.skinned?data(skin_va,(size_t)nvertices*8):NULL;
    const unsigned char*batches=span(word(n+0x24),(size_t)nbatches*33);
    if(!pos||!batches||(b.skinned&&!skin))return 0;
    memcpy(b.world,r+0x20,64);memcpy(b.tint,r+0xD0,16);memcpy(b.bounds,r+0x80,48);
    if(!finite_values(b.world,16)||!finite_values(b.tint,4)||!finite_values(b.bounds,12)||(!pass->parent_clip&&!projection(s,r,&b)))return 0;
    if(pass->light_material&&!r[0xB1]){const unsigned char*manager=span(read32(s->manager+0x2C),0x48);if(!manager||word(manager)!=0x456E30u)return 0;memcpy(&b.material_scalar,manager+0x44,4);}
    for(uint32_t j=pass->single_batch?pass->batch_index:0;j<(pass->single_batch?pass->batch_index+1:nbatches);j++){
        const unsigned char*batch=batches+33*j;
        if((pass->filter_light&&half(batch+6)!=pass->light_index)||(pass->require_draw_flag&&!(batch[8]&2)))continue;
        if(!pass->single_batch&&!update_palette(word(n+0x38)?word(n+0x38)+j*8:0,word(r+0x74),word(resource+0xC),pal))return 0;
        uint32_t mat=read32(word(resource+0x34)+4*half(batch+4));
        const unsigned char*m=span(mat,0x50);if(!m||word(m)!=0x4565B0u)return 0;
        if(pass->filter_material&&(half(m+0x4C)&1u)!=(partition!=0))continue;
        if(pass->light_material){if(!material(mat,&b)||!!(b.material_flags&4u)!=b.skinned){++counts[8];continue;}}
        else {b.material_va=mat;b.material_flags=half(m+0x4C);b.material_mode=m[0x24];memcpy(&b.specular_strength,m+0x44,4);}
        b.texture_image_va=b.texture_header_va=b.texture_address=b.texture_filter=b.texture_control0=0;memset(b.texture_header,0,sizeof(b.texture_header));
        int masked=pass->light_material&&b.material_flags==0x25u&&b.material_mode==1;
        if(masked&&(!uv||!basis||!texture(word(m+0x28),&b))){++counts[8];continue;}
        uint32_t bytes=word(batch+0x15),pb=word(batch+0xD)&0x0FFFFFFFu;
        const unsigned char*p=span(pb,bytes);size_t count=0;
        b.vertices=NULL;b.indexed_vertices=NULL;b.indices=NULL;b.palette=NULL;b.index_count=0;b.primitive=5;
        if(p&&pass->light_material&&indexed_enabled()){
            if(!indexed_geometry(&b,p,bytes,nvertices,pos,skin,uv,basis,pal)){++counts[7];continue;}
            b.source_batch=j;if(!callback(owner,&b))return 0;continue;
        }
        if(!p||!uc_control_model_indices(p,bytes,nvertices,NULL,0,&count)||
            !reserve((void**)&indices,&index_capacity,count,sizeof(*indices))||
            !reserve((void**)&vertices,&vertex_capacity,count,sizeof(*vertices))||
            !uc_control_model_indices(p,bytes,nvertices,indices,index_capacity,&count))return 0;
        /* Reuse vertices within this draw only. Keep the expanded source triangle
         * order, including strip degenerates, and the original pose arithmetic. */
        int unique_base=!pass->light_material&&pass->parent_clip&&base_indexed_enabled()&&count>=12;
        if(unique_base)for(size_t k=0;k<count;k++)vertex_map[indices[k]]=UINT32_MAX;
        uint32_t unique=0;
        int valid=1;for(size_t k=0;k<count;k++){
            uint32_t at=indices[k];
            if(unique_base&&vertex_map[at]!=UINT32_MAX){indices[k]=vertex_map[at];continue;}
            UcHybridModelVertex*v=vertices+(unique_base?unique:k);
            if(!pose(pos+(size_t)at*(b.skinned?10:16),skin?skin+at*8:NULL,pal,v)){valid=0;break;}
            if(uv)memcpy(v->uv,uv+at*8,8);else memset(v->uv,0,8);
            if(!finite_values(v->uv,2)){valid=0;break;}
            if(basis)memcpy(v->basis,basis+at*8,8);else memset(v->basis,0,8);
            v->color=color?word(color+at*4):UINT32_MAX;
            memset(v->basis5,0,sizeof(v->basis5));memset(v->basis6,0,sizeof(v->basis6));
            if(masked){
                unsigned char packed_position[16];UcHybridModelVertex tangent;
                memcpy(packed_position,pos+(size_t)at*(b.skinned?10:16),b.skinned?10:16);
                for(unsigned axis=0;axis<2;axis++){
                    memcpy(packed_position+(b.skinned?6:12),v->basis+axis,4);
                    if(!pose(packed_position,skin?skin+at*8:NULL,pal,&tangent)){valid=0;break;}
                    memcpy(axis?v->basis6:v->basis5,tangent.normal,12);
                }
                if(!valid)break;
            }
            if(unique_base){vertex_map[at]=unique;indices[k]=unique++;}
        }
        if(!valid){++counts[7];continue;}
        b.source_batch=j;b.vertices=vertices;b.vertex_count=(uint32_t)count;
        if(unique_base&&unique<count){b.vertex_count=unique;b.indices=indices;b.index_count=(uint32_t)count;}
        if(!callback(owner,&b))return 0;
    }return 1;
}
void uc_hybrid_models_light(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,uint32_t iterator,uint32_t context){
    (void)context;
    if(!initialized){const char*v=getenv("UCRR_HYBRID_EXTENDED");enabled=v&&!strcmp(v,"1");thread=GetCurrentThreadId();initialized=1;}
    if(!s||!l)return;
    if(!enabled&&!(s->caller==0x9EA8Au&&l->vtable==0x457270u&&uc_hybrid_flashlight_models_enabled()))return;
    if(GetCurrentThreadId()!=thread){++counts[11];return;}
    if(s->caller!=0x9EA8Au&&s->caller!=0x9ED75u)return;
    const unsigned char*alternate=span(0x519F88,2),*debug=span(0x51B0DE,3);
    if(!alternate||!debug||alternate[0]||alternate[1]||debug[0]||debug[1]||debug[2]){++counts[9];return;}
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);++counts[0];
    if(epoch!=s->epoch){epoch=s->epoch;palette_ready=0;}
    if(!palette_ready)palette_ready=uc_hybrid_models_host_palette(&palette);
    _mm_setcsr(0x1F80);
    const unsigned char*i=span(iterator,16);if(!palette_ready||!i)goto invalid;
    uint32_t begin=word(i),end=word(i+8);uint16_t mask=half(i+0xC),value=half(i+0xE);
    if(end<begin||(end-begin)%20||(end-begin)/20>32768)goto invalid;
    const unsigned char*entries=span(begin,(size_t)(end-begin));if(!entries)goto invalid;
    for(uint32_t n=0;n<(end-begin)/20;n++){
        const unsigned char*e=entries+n*20;if((half(e+4)&mask)!=value)continue;
        ++counts[1];if(!receiver(s,l,word(e+0xC),word(e+0x10)))++counts[7];
    }restore_cpu(&cpu);return;
invalid: ++counts[10];restore_cpu(&cpu);
}
struct SingleCall {const UcHybridLightingScene*s;const UcHybridLightingRecord*l;int result;};
static int submit_single(void*owner,const UcHybridModelBatch*b){
    struct SingleCall*c=owner;++counts[4];c->result=uc_hybrid_models_host_draw(c->s,c->l,b);_mm_setcsr(0x1F80);
    if(c->result>0){++counts[5];counts[6]+=b->index_count?b->index_count:b->vertex_count;}else ++counts[8];return 1;
}
int uc_hybrid_models_batch(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,
    uint32_t receiver_va,uint32_t node,uint32_t source_batch,uint32_t partition){
    if(!s||!l)return 0;HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
    UCModelRenderPalette current;struct SingleCall call={s,l,0};
    UcHybridModelPass pass={partition,0,0,0,1,1,1,source_batch,node};
    if(uc_hybrid_models_host_palette(&current)){
        _mm_setcsr(0x1F80);
        uc_hybrid_models_geometry(s,receiver_va,&pass,&current,submit_single,&call);
    }
    restore_cpu(&cpu);return call.result;
}
int uc_hybrid_models_original_batch(uint32_t light,uint32_t receiver_va,uint32_t node,uint32_t source_batch,uint32_t partition,uint32_t modifier){
    if(!uc_hybrid_lighting_model_replacement_enabled())return 0;
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);UcHybridLightingScene s;UcHybridLightingRecord l;int result=0;
    if(uc_hybrid_lighting_model_source(light,receiver_va,modifier,&s,&l))
        result=uc_hybrid_models_batch(&s,&l,receiver_va,node,source_batch,partition);
    restore_cpu(&cpu);return result!=0;
}
void uc_hybrid_models_visitor_begin(void){palette_ready=0;}
void uc_hybrid_models_read_counts(uint64_t*out,unsigned count){if(out&&count<=12)memcpy(out,counts,count*sizeof(*out));}

/* Retain CONTROL's material wrapper/setup and replace only its final batch draw.
 * Original material setup and palette updates precede this route. Projected modifiers remain original. */
static struct {uint32_t receiver,node,batch,descriptor,material;} base_call;
static int base_enabled(void){static int ready,on;if(!ready){const char*v=getenv("UCRR_HYBRID_NATIVE_BASE");on=v&&!strcmp(v,"1");ready=1;}return on;}
void uc_hybrid_models_base_end(void){memset(&base_call,0,sizeof base_call);}
void uc_hybrid_models_base_begin(uint32_t receiver_va,uint32_t node,uint32_t batch,uint32_t modifier){
    uc_hybrid_models_base_end();if(!base_enabled()||modifier)return;
    const unsigned char*r=span(receiver_va,0x164),*n=span(node,0x70);
    if(!r||!n||word(r)!=0x457880u||(word(r+0x60)&4u)||batch>=word(n+0xC))return;
    const unsigned char*res=span(word(r+0x68),0x3C);if(!res||word(res)!=0x457008u)return;
    uint32_t bundle=word(n+0x20),nv=word(n+8);
    if(!nv||nv>65536||!data(bundle+0x18,(size_t)nv*4)||!data(bundle+0x24,(size_t)nv*8))return;
    uint32_t descriptor=word(n+0x24)+batch*33;const unsigned char*b=span(descriptor,33);
    if(!b||half(b+6)!=65535||!(b[8]&2)||half(b+4)>=word(res+4))return;
    uint32_t mat=read32(word(res+0x34)+4*half(b+4));const unsigned char*m=span(mat,0x50);
    if(!m||word(m)!=0x4565B0u||!!(half(m+0x4C)&4u)!=!!word(n+0x3C)||m[0x24]>1||!word(m+8)||word(m+8)>256||word(m+0x1C)!=1)return;
    if(!span(word(m+0x14),word(m+8)))return; /* Original executes setup; host qualifies its resulting program/state. */
    base_call.receiver=receiver_va;base_call.node=node;base_call.batch=batch;base_call.descriptor=descriptor;base_call.material=mat;
}
static int base_callback(void*owner,const UcHybridModelBatch*b){*(int*)owner=uc_hybrid_models_host_base(b);return 1;}
int uc_hybrid_models_base_submit(uint32_t batch_va,uint32_t material_va,uint32_t pass){
    if(!base_call.receiver||batch_va!=base_call.descriptor||material_va!=base_call.material||pass)return 0;
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
    /* The actual uploaded clip rows already include CONTROL's selected camera
     * and depth range. Do not reconstruct a second viewport for first-person.
     * The optional base index path reuses these same CPU poses within this draw. */
    UcHybridLightingScene scene={0};UCModelRenderPalette current={0};int result=0;
    UcHybridModelPass selected={0,65535,1,1,0,0,1,base_call.batch,base_call.node,1};
    if(!read32(base_call.node+0x3C)||uc_hybrid_models_host_palette(&current)){
        _mm_setcsr(0x1F80);
        uc_hybrid_models_geometry(&scene,base_call.receiver,&selected,&current,base_callback,&result);
    }
    restore_cpu(&cpu);return result!=0; /* Fatal submission must never replay. */
}
