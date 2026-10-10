#include "hybrid_casters_guest.h"
#include "hybrid_models_guest.h"
#include "hybrid_guest_cpu.h"
#include "xbox_memory_layout.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* Bounded same-thread owned copies; retained capacity, no frame file logging. */
static uint64_t counts[8]; /* scopes,registry actors,pairs,batches,vertices,unsupported,invalid,host failure */
static uint32_t receivers[32768],admitted[32768],receiver_count;
static NativeShadowCaster casters[4096];static uint32_t first_vertex[4096],caster_count,position_count;
static float (*positions)[3];static uint32_t *elements;static size_t position_capacity,index_capacity;
static DWORD owner_thread;
static const unsigned char *span(uint32_t va,size_t n){uint32_t p=va;if(va>=0x80000000u&&va<0x84000000u)p=va-0x80000000u;else if(va>=0x80000000u)return NULL;if(p<0x10000u||n>g_xbox_total_ram||(uint64_t)p+n>g_xbox_total_ram)return NULL;return (const unsigned char*)XBOX_PTR(p);}
static uint32_t word(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint32_t read32(uint32_t p){const void*v=span(p,4);return v?word(v):0;}
static int grow(void **p,size_t *capacity,size_t need,size_t stride){if(need<=*capacity)return 1;if(need>1048576)return 0;void*q=realloc(*p,need*stride);if(!q)return 0;*p=q;*capacity=need;return 1;}
static int compare(const void*a,const void*b){uint32_t x=word(a),y=word(b);return (x>y)-(x<y);}
static int add_receiver(uint32_t va){if(!va)return 1;const unsigned char*r=span(va,0x164);if(!r)return 0;if(word(r)!=0x457880u){++counts[5];return 1;}if(receiver_count==32768)return 0;receivers[receiver_count++]=va;return 1;}
/* 951D0 first traverses the registry's object map, then registered dynamic list.
 * Both feed B9ED0, whose output set removes duplicates by receiver address. */
static int registry(uint32_t manager){
 const unsigned char*m=span(read32(manager+0x48),0x90);if(!m||word(m)!=0x456308u)return 0;
 uint32_t head=word(m+0x88),stack[256],top=0,at=read32(head+4),visits=0;
 const unsigned char*h=span(head,0x16);if(!h||!h[0x15])return 0;
 for(;;){
    while(at!=head){const unsigned char*n=span(at,0x16);if(!n||n[0x15]||top==256||++visits>32768)return 0;stack[top++]=at;at=word(n);}
    if(!top)break;at=stack[--top];const unsigned char*n=span(at,0x16);if(!add_receiver(word(n+0x10)))return 0;at=word(n+8);
 }
 head=word(m+0x74);h=span(head,12);if(!h)return 0;at=word(h);visits=0;
 while(at!=head){const unsigned char*n=span(at,12);if(!n||++visits>32768||!add_receiver(word(n+8)))return 0;at=word(n);}
 qsort(receivers,receiver_count,sizeof *receivers,compare);uint32_t unique=0;
 for(uint32_t i=0;i<receiver_count;i++)if(!unique||receivers[i]!=receivers[unique-1])receivers[unique++]=receivers[i];receiver_count=unique;counts[1]+=unique;return 1;
}
static int member(uint32_t receiver,uint32_t light){
 uint32_t head=read32(receiver+0x104),at=read32(head+4);
 const unsigned char*h=span(head,0x12);if(!h||!h[0x11])return 0;
 for(unsigned i=0;i<256;i++){const unsigned char*n=span(at,0x12);if(!n||n[0x11])return 0;uint32_t key=word(n+0xc);if(key==light)return 1;at=word(n+(key<light?8:0));}return 0;
}
static int casts(uint32_t receiver,uint32_t light){const unsigned char*r=span(receiver,0x164);return r&&word(r)==0x457880u&&word(r+0xc)==0x4577c0u&&!r[0xb1]&&(word(r+0x60)&2u)&&r[0x66]&&member(receiver,light);}
struct Capture{uint32_t light;};
static int capture(void*opaque,const UcHybridModelBatch*b){
 struct Capture*c=opaque;
 /* Explicit PC opaque silhouette policy; neither transparent nor alternate
  * material semantics are guessed from texture pixels or camera visibility. */
 const unsigned char*node=span(b->node_va,0x70);
 if(!node)return 0;
 /* Mode5 is BB300 (BAF54[5]), using stencil submesh node+44. Require its
  * actual batch count: a registered actor with no volume must not acquire a
  * new shadow merely because it has an opaque visible surface. */
 if(!word(node+0x48)||b->flags&0x1b0u||b->material_mode||b->material_flags&0x21u){++counts[5];return 1;}
 if(!b->vertex_count)return 1;
 if(b->vertex_count>1048576||b->vertex_count%3||caster_count==4096||position_count>1048576-b->vertex_count||
    b->world[3]!=0||b->world[7]!=0||b->world[11]!=0||b->world[15]!=1)return 0;
 if(!grow((void**)&positions,&position_capacity,position_count+b->vertex_count,sizeof *positions))return 0;
 const float*w=b->world;
 for(uint32_t i=0;i<b->vertex_count;i++)for(unsigned k=0;k<3;k++){
    const float*p=b->vertices[i].position;float v=((p[0]*w[k]+p[1]*w[4+k])+p[2]*w[8+k])+w[12+k];if(!isfinite(v))return 0;positions[position_count+i][k]=v;
 }
 if(b->vertex_count>index_capacity){size_t previous=index_capacity;if(!grow((void**)&elements,&index_capacity,b->vertex_count,sizeof *elements))return 0;for(size_t i=previous;i<index_capacity;i++)elements[i]=(uint32_t)i;}
 first_vertex[caster_count]=position_count;NativeShadowCaster*v=casters+caster_count++;memset(v,0,sizeof *v);v->vertex_count=v->index_count=b->vertex_count;v->source_light_index=c->light;
 position_count+=b->vertex_count;++counts[3];counts[4]+=b->vertex_count;return 1;
}
int uc_hybrid_casters_collect(const UcHybridLightingScene*s,const UcHybridLightingRecord*lights,unsigned count){
 if(!s||s->caller!=0x9ea8au||count>512||(count&&!lights)||!uc_hybrid_extended_enabled())return 0;
 if(!owner_thread)owner_thread=GetCurrentThreadId();if(owner_thread!=GetCurrentThreadId()){++counts[6];return 0;}
 HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1f80);++counts[0];receiver_count=caster_count=position_count=0;
 const unsigned char*disabled=span(0x51b0e2,1);int valid=disabled!=NULL;
 if(valid&&!*disabled&&count){valid=registry(s->manager);UCModelRenderPalette palette;
    if(valid)valid=uc_hybrid_models_host_palette(&palette);
    _mm_setcsr(0x1f80); /* The host palette drain must not change later pose math. */
    for(unsigned j=0;j<count&&valid;j++){
        const UcHybridLightingRecord*l=lights+j;if(!l->shadow||l->source_index<0||l->vtable!=0x4573f0u)continue;
        unsigned admitted_count=0;for(uint32_t i=0;i<receiver_count;i++)if(casts(receivers[i],l->source_va))admitted[admitted_count++]=receivers[i];
        UcHybridModelPass pass={0,0xffff,1,1,1,0};struct Capture context={(uint32_t)l->source_index};
        for(unsigned i=0;i<admitted_count;i++){++counts[2];if(!uc_hybrid_models_geometry(s,admitted[i],&pass,&palette,capture,&context)){valid=0;break;}}
    }
 }
 if(!valid){++counts[6];caster_count=0;}
 for(uint32_t i=0;i<caster_count;i++){casters[i].positions=positions+first_vertex[i];casters[i].indices=elements;}
 int result=uc_hybrid_casters_host_submit(s,casters,caster_count);if(!result)++counts[7];restore_cpu(&cpu);return valid&&result;
}
void uc_hybrid_casters_read_counts(uint64_t*out,unsigned count){if(out&&count<=8)memcpy(out,counts,count*sizeof(*out));}
