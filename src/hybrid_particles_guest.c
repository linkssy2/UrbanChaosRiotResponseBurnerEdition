#include "hybrid_particles_guest.h"
#include "hybrid_guest_cpu.h"
#include "xbox_memory_layout.h"
#include "../native/particle_billboard_batch.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
static unsigned initialized,enabled;
static DWORD owner_thread;
/* attempts, rejected, drawn, fatal, drawn_records, wrong_thread. No file IO. */
static uint64_t counts[6];
static const unsigned char *span(uint32_t va,uint32_t bytes){
    uint32_t p=va;
    if(va>=0x80000000u&&va<0x84000000u)p=va-0x80000000u;
    else if(va>=0x80000000u)return NULL;
    if(p<0x10000u||(uint64_t)p+bytes>g_xbox_total_ram)return NULL;
    return (const unsigned char*)XBOX_PTR(p);
}
static uint32_t word(const unsigned char*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint32_t read32(uint32_t va){const unsigned char*p=span(va,4);return p?word(p):0;}
static int finite_values(const float *v,unsigned n){
    for(unsigned i=0;i<n;i++){uint32_t w;memcpy(&w,v+i,4);if((w&0x7FFFFFFFu)>=0x7F800000u)return 0;}return 1;
}
static int snapshot(uint32_t frame,uint32_t count,UcHybridParticleBatch*b){
    const unsigned char*s=span(frame,0x730);if(!s||(frame&15u)||!count||count>39||s[0x17])return 0;
    uint32_t total=word(s+0x1C),first=word(s+0x28),n=word(s+0x38);
    if(n!=count||first>=total||total>1048576u||count!=(total-first<39?total-first:39))return 0;
    b->owner_va=word(s+0x24);const unsigned char*owner=span(b->owner_va,8);if(!owner)return 0;
    b->source_va=word(owner+4);const unsigned char*source=span(b->source_va,0x24);
    if(!source||word(source)!=0x483020u)return 0;
    const unsigned char*resource=span(word(source+0x20),0x98);if(!resource)return 0;
    b->source_texture_va=word(resource+0x94);if(!span(b->source_texture_va,4))return 0;
    b->vertex_shader=read32(0x51A204);b->pixel_shader=read32(0x51A1F4);
    if(!b->vertex_shader||!b->pixel_shader)return 0;
    memcpy(b->clip_rows,s+0x170,64);memcpy(b->up,s+0x90,16);memcpy(b->right,s+0xE0,16);
    if(!finite_values(&b->clip_rows[0][0],16)||!finite_values(b->up,4)||!finite_values(b->right,4))return 0;
    UCParticleSprite sprites[39];
    for(unsigned i=0;i<count;i++){
        memcpy(&sprites[i],s+0x4C0+i*16,16);memcpy(&sprites[i].angle,s+0x1B0+i*4,4);
        memcpy(sprites[i].color,s+0x250+i*16,16);
    }
    if(uc_particle_billboard_batch(sprites,count,b->billboards))return 0;
    b->count=count;const UCParticleBillboard*last=b->billboards+count-1;
    memcpy(b->last_raw,last->center,12);b->last_raw[3]=last->color;
    memcpy(b->last_raw+4,&last->radius,4);b->last_raw[5]=0x40400000u;
    b->last_raw[6]=word(s+0xB8);memcpy(b->last_raw+7,&last->cosine,4);
    memcpy(b->last_raw+8,&last->sine,4);b->last_raw[9]=0;
    b->last_raw[10]=b->last_raw[8]^0x80000000u;b->last_raw[11]=b->last_raw[7];b->last_raw[12]=0;
    return 1;
}
int uc_hybrid_particles_try_draw(uint32_t frame,uint32_t count){
    if(!initialized){const char*v=getenv("UCRR_HYBRID_NATIVE_PARTICLES");enabled=v&&!strcmp(v,"1");owner_thread=GetCurrentThreadId();initialized=1;}
    if(!enabled)return 0;
    if(GetCurrentThreadId()!=owner_thread){++counts[5];return 0;}
    ++counts[0];HybridCPU cpu;save_cpu(&cpu);UcHybridParticleBatch batch;int result=0;
    if(snapshot(frame,count,&batch))result=uc_hybrid_particles_host_draw(&batch);
    restore_cpu(&cpu);
    if(result>0){++counts[2];counts[4]+=count;return 1;}
    if(result<0){++counts[3];enabled=0;return -1;}
    ++counts[1];return 0;
}
void uc_hybrid_particles_read_counts(uint64_t*out,unsigned count){if(out&&count<=6)memcpy(out,counts,count*sizeof(*out));}
