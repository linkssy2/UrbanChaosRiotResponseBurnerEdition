/* Isolated CONTROL experiment: replace the shared light visitor, never blend
 * native light over an original light pass. Guest data is copied, not retained. */
#include "hybrid_lighting_guest.h"
#include "hybrid_lighting_host.h"
extern int uc_hybrid_scenery_lighting_visible(void);
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <windows.h>
#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "hybrid_guest_cpu.h"
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
#include "hybrid_models_guest.h"
#include "hybrid_casters_guest.h"
#endif
static int span(uint32_t va,uint32_t size,uint32_t*out){
    uint32_t p=va;
    if(va>=0x80000000u && va<0x84000000u)p=va-0x80000000u;
    else if(va>=0x80000000u)return 0;
    if((p&3u)||p<0x10000u||(uint64_t)p+size>g_xbox_total_ram)return 0;
    if(out)*out=p;return 1;
}
static int read32(uint32_t va,uint32_t*out){uint32_t p;if(!span(va,4,&p))return 0;memcpy(out,(const void*)XBOX_PTR(p),4);return 1;}
static int finite_values(const float*v,unsigned n){for(unsigned i=0;i<n;i++){uint32_t x;memcpy(&x,v+i,4);if((x&0x7FFFFFFFu)>=0x7F800000u)return 0;}return 1;}
static int take(uint32_t va,void*out,unsigned n){uint32_t p;if(!span(va,n,&p))return 0;memcpy(out,(const void*)XBOX_PTR(p),n);return 1;}

enum { HYBRID_LIGHT_LIMIT=512, HYBRID_SCOPE_LIMIT=4 };
static struct {
    UcHybridLightingScene scene;
    UcHybridLightingRecord lights[HYBRID_LIGHT_LIMIT];
    UCNativeControlSpot spots[HYBRID_LIGHT_LIMIT];
    uint32_t spot_sources[HYBRID_LIGHT_LIMIT];
    unsigned spot_count;
    unsigned count,valid,applied,hide_models,simple_models;
    uint32_t level_root;
} scopes[HYBRID_SCOPE_LIMIT];
/* Public symbol for bounded post-run inspection; no per-frame file writes. */
uint64_t uc_hybrid_counts[12];
/* scopes, suppressed, point, directional, spot, unknown, invalid, overflow,
 * host_ok, host_failed, unsupported_scope, owner_error. */
static unsigned initialized,enabled,depth,diagnostic,extended,flashlight,flashlight_models,control_flashlight,control_models,control_scenery_effects,model_replacement;
static DWORD owner_tid;
static uint64_t serial;
static volatile LONG character_lighting_visible=1,character_lighting_quality;
int uc_hybrid_character_lighting_quality(void){return InterlockedCompareExchange(&character_lighting_quality,0,0)!=0;}
void uc_hybrid_character_lighting_set_quality(int simple){InterlockedExchange(&character_lighting_quality,!!simple);}
int uc_hybrid_character_lighting_visible(void){return InterlockedCompareExchange(&character_lighting_visible,0,0)!=0;}
void uc_hybrid_character_lighting_show(int visible){InterlockedExchange(&character_lighting_visible,!!visible);}
static int camera(UcHybridLightingScene *s) {
    uint32_t m,t,p,e;
    if(!read32(0x519F60,&m)||!span(m,0x58,NULL)||!read32(m,&t)||t!=0x4561F8u||
       !read32(m+0x54,&p)||!span(p,0x194,NULL)||!read32(p,&t)||t!=0x456D88u||
       !read32(m+0x38,&e)||!span(e,12,NULL)||!read32(e,&t)||t!=0x456EF0u)return 0;
    s->manager=m;s->matrix_provider=p;
    if(!take(p+0x10,s->camera,64)||!take(p+0x50,s->view,64)||!take(p+0xD0,s->projection,64)||
       !take(e+8,&s->exposure,4)||!finite_values(s->camera,16)||!finite_values(s->view,16)||
       !finite_values(s->projection,16)||!finite_values(&s->exposure,1)||s->exposure<=0)return 0;
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){
        double sum=0;for(unsigned k=0;k<4;k++)sum+=(double)s->view[r*4+k]*s->projection[k*4+c];
        s->view_projection[r*4+c]=(float)sum;
    }
    return finite_values(s->view_projection,16);
}
/* 209540 copies the loaded level name into the scene root +2C; 2431B0
 * returns that buffer for mission logic. 56F90 owns the derived root type. */
static uint32_t westside_level(void) {
    uint32_t root,t,p; char name[5];
    if(!read32(0x51C4DC,&root)||!span(root,0x6C,&p)||!read32(root,&t)||t!=0x450C48u||
       !take(root+0x2C,name,sizeof(name))||memcmp(name,"1-01",sizeof(name)))return 0;
    return p;
}
static int same_camera(const UcHybridLightingScene*a,const UcHybridLightingScene*b) {
    return a->manager==b->manager&&a->matrix_provider==b->matrix_provider&&
        !memcmp(a->camera,b->camera,sizeof(a->camera))&&!memcmp(a->view,b->view,sizeof(a->view))&&
        !memcmp(a->projection,b->projection,sizeof(a->projection))&&!memcmp(&a->exposure,&b->exposure,sizeof(a->exposure));
}
static void initialize(void) {
    if(initialized)return;initialized=1;
    const char *v=getenv("UCRR_HYBRID_NATIVE_LIGHTING");enabled=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_DIAGNOSTIC");diagnostic=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_CONTROL_FLASHLIGHT");control_flashlight=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_CONTROL_MODELS");control_models=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_MODEL_REPLACEMENT");model_replacement=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_CONTROL_SCENERY_EFFECTS");control_scenery_effects=v&&!strcmp(v,"1");
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
    v=getenv("UCRR_HYBRID_EXTENDED");extended=v&&!strcmp(v,"1");
    v=getenv("UCRR_HYBRID_FLASHLIGHT");flashlight=v?!strcmp(v,"1"):extended;
    v=getenv("UCRR_HYBRID_FLASHLIGHT_MODELS");flashlight_models=v&&!strcmp(v,"1");
#endif
    /* Original model materials own these contributions in this mode. Even a
     * conflicting extended/native-model setting must not draw them twice. */
    if(control_models){extended=0;flashlight_models=0;}
    owner_tid=GetCurrentThreadId();
}
int uc_hybrid_extended_enabled(void){initialize();return enabled&&extended;}
int uc_hybrid_flashlight_models_enabled(void){initialize();return enabled&&flashlight&&flashlight_models&&!control_flashlight;}
static int control_model_scope(void){
    return control_models&&depth&&depth<=HYBRID_SCOPE_LIMIT&&
        (scopes[depth-1].scene.caller==0x9EA8Au||scopes[depth-1].scene.caller==0x9ED75u);
}
static uint16_t half(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t word(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
int uc_hybrid_lighting_model_replacement_enabled(void){initialize();return model_replacement&&enabled&&control_models;}
int uc_hybrid_lighting_model_source(uint32_t light,uint32_t receiver,uint32_t modifier,
    UcHybridLightingScene *scene,UcHybridLightingRecord *record){
    initialize();
    if(!model_replacement||!enabled||!control_model_scope()||modifier||!scene||!record||
       GetCurrentThreadId()!=owner_tid||!scopes[depth-1].valid||
       scopes[depth-1].scene.caller!=0x9EA8Au)return 0;
    uint32_t p,type,flags,alternate,debug;
    if(!span(0x519F88u,2,&alternate)||!span(0x51B0DCu,5,&debug))return 0;
    const uint8_t *a=(const uint8_t*)XBOX_PTR(alternate),*d=(const uint8_t*)XBOX_PTR(debug);
    if(a[0]||a[1]||d[2]||d[3]||d[4])return 0; /* Alternate/debug material shaders stay original. */
    if(!span(receiver,0x164,&p)||!read32(receiver,&type)||type!=0x457880u||
       *((const uint8_t*)XBOX_PTR(p+0xB1))||!read32(receiver+0x60,&flags)||
       (flags&0x1B4u))return 0; /* Original viewport/projection variants stay original. */
    UcHybridLightingRecord r={0};
    if(!span(light,0xD8,&r.source_va)||!read32(light,&r.vtable)||
       (r.vtable!=0x457338u&&r.vtable!=0x4573F0u))return 0;
    if(!take(light+0x10,r.world,64)||!take(light+0xC0,r.local,16)||
       !take(light+0x60,r.color,16)||!take(light+0x70,&r.intensity,4)||
       !take(light+0x78,&r.source_index,4)||!finite_values(r.world,16)||
       !finite_values(r.local,4)||!finite_values(r.color,4)||
       !finite_values(&r.intensity,1))return 0;
    if(r.vtable==0x4573F0u){
        if(!take(light+0xD0,&r.radius,4)||!finite_values(&r.radius,1))return 0;
        r.specular=*((const uint8_t*)XBOX_PTR(r.source_va+0xD4));
    }
    *scene=scopes[depth-1].scene;*record=r;return 1;
}
static int scenery_slot(unsigned slot){return slot==0||slot==264||(slot>=289&&slot<=292);}
static int scenery_scope(void){
    return control_scenery_effects&&control_model_scope()&&scopes[depth-1].valid&&
        scopes[depth-1].level_root&&scopes[depth-1].scene.caller==0x9EA8Au;
}
/* A6A50/B5450: the qualified 1-01 GEO has 48 static one-node resources, each
 * sharing the same ordered 297 material names. No names survive in materials.
 * Validate that exact layout; do not infer native ownership from live specular. */
static int scenery_node(uint32_t receiver,uint32_t *resource,uint32_t *node){
    uint32_t p,r,g,n;
    if(!span(receiver,0xB4,&p)||word((const void*)XBOX_PTR(p))!=0x457880u||
       !*((const uint8_t*)XBOX_PTR(p+0xB1))||word((const void*)XBOX_PTR(p+0x78))!=0||
       !read32(p+0x68,&r)||!span(r,0x3C,&r)||word((const void*)XBOX_PTR(r))!=0x457008u||
       word((const void*)XBOX_PTR(r+4))!=297||word((const void*)XBOX_PTR(r+8))!=1||
       word((const void*)XBOX_PTR(r+0xC))!=1||!read32(r+0x38,&g)||!span(g,0x38,&g)||
       !read32(g+0x34,&n)||!span(n,0x70,&n)||word((const void*)XBOX_PTR(n+0x38))||
       word((const void*)XBOX_PTR(n+0x3C)))return 0;
    *resource=r;*node=n;return 1;
}
static int scenery_point(uint32_t light,uint32_t *index){
    uint32_t t;return read32(light,&t)&&t==0x4573F0u&&read32(light+0x78,index)&&*index>=1&&*index<=102;
}
static int simple_emergency_point(uint32_t light){
    uint32_t index;
    /* Qualified 1-01 GEO identities: WL01, EM01/02, Siren02/01,
     * ALARM02/01 and Alight01. Their original LightFitting owners retain
     * color/intensity/flicker updates. Indexed lights are not necessarily
     * steady lights; index<0 alone would miss these emergency sources.
     * No cross-level identity or color heuristic is inferred here. */
    if(!scopes[depth-1].level_root||!scenery_point(light,&index))return 0;
    return index==65||index==66||index==67||index==70||index==71||
        index==83||index==91||index==98;
}
static int scenery_batches(uint32_t node,uint32_t *base,uint32_t *count){
    return read32(node+0xC,count)&&*count&&*count<=4096&&read32(node+0x24,base)&&span(*base,*count*33,base);
}
int uc_hybrid_lighting_skip_receiver(uint32_t light,uint32_t receiver,uint32_t context){
    initialize();if(!enabled||GetCurrentThreadId()!=owner_tid)return 0;
    uint32_t type,p;
    /* Receiving only: keep the earlier caster pass and Begin/End untouched.
     * +B1=0 is the proven character/movable-model family, not an NPC type.
     * Simple mode omits ordinary local point-light receiver passes, retaining
     * qualified emergency sources; directional light, spotlight/flashlight
     * and existing base shading remain. This is an
     * intentional quality reduction, not a visually equivalent optimization.
     * Gameplay C8930 puts gun/arms/flashlight in this WORLD scope, with
     * B5D80/B5DD0 depth/projection flags80/100. Preserve those receivers too;
     * private9ED75 is also used by equipment preview, not a player identity. */
    if(depth&&depth<=HYBRID_SCOPE_LIMIT&&(scopes[depth-1].hide_models||scopes[depth-1].simple_models)&&
       scopes[depth-1].scene.caller==0x9EA8Au&&read32(light,&type)&&
       ((type==0x4573F0u&&(scopes[depth-1].hide_models||!simple_emergency_point(light)))||
        (scopes[depth-1].hide_models&&(type==0x457338u||type==0x457270u)))&&
       span(receiver,0xB2,&p)&&read32(receiver,&type)&&type==0x457880u&&
       !*((const uint8_t*)XBOX_PTR(p+0xB1))&&
       !(word((const void*)XBOX_PTR(p+0x60))&0x180u))return 1;
    if(!control_model_scope())return 0;
    if(!read32(light,&type)||(type!=0x4573F0u&&type!=0x457338u))return 0;
    /* A6170 tags map instances and B60A0 stores that constructor argument in
     * +B1. BAFB0 uses it for indexed map light batches. Ordinary character,
     * weapon and movable model instances use zero, regardless of skinning.
     * Admit only that proven class; unknown receivers retain suppression. */
    if(!span(receiver,0xB2,&p)||!read32(receiver,&type)||type!=0x457880u)return 1;
    if(!*((const uint8_t*)XBOX_PTR(p+0xB1)))return 0;
    /* Native geometry is Westside-only. Other missions retain their original
     * map receiver passes; never discard lighting without a replacement. */
    if(!scopes[depth-1].level_root)return !uc_hybrid_scenery_lighting_visible();
    uint32_t index,r,n,c,base,count;
    if(!scenery_scope()||!scenery_point(light,&index)||!span(context,0x24,&c)||
       word((const void*)XBOX_PTR(c))!=3||word((const void*)XBOX_PTR(c+0x20))!=light||
       (word((const void*)XBOX_PTR(c+8))!=0&&word((const void*)XBOX_PTR(c+8))!=UINT32_MAX)||
       !scenery_node(receiver,&r,&n)||!scenery_batches(n,&base,&count))return 1;
    /* Cheap source membership prefilter avoids original matrix/query/stream
     * preparation on map receivers with no omitted batch for this light. */
    const uint8_t*b=(const uint8_t*)XBOX_PTR(base);
    for(unsigned j=0;j<count;j++,b+=33)
        if(half(b+6)==index&&(b[8]&2)&&scenery_slot(half(b+4)))return 0;
    return 1;
}
int uc_hybrid_lighting_skip_scenery_batch(uint32_t light,uint32_t receiver,uint32_t node,uint32_t batch){
    initialize();if(!enabled||GetCurrentThreadId()!=owner_tid||!scenery_scope())return 0;
    uint32_t type,p,index,r,n,base,count,table,m;
    if(!read32(light,&type)||type!=0x4573F0u||!span(receiver,0xB2,&p)||
       !*((const uint8_t*)XBOX_PTR(p+0xB1)))return 0; /* Original actors/flashlight unchanged. */
    if(!scenery_point(light,&index)||!scenery_node(receiver,&r,&n)||node!=n||
       !scenery_batches(n,&base,&count)||batch<base||(batch-base)%33||(batch-base)/33>=count)return 1;
    const uint8_t*b=(const uint8_t*)XBOX_PTR(batch);unsigned slot=half(b+4);
    if(half(b+6)!=index||!(b[8]&2)||!scenery_slot(slot)||
       !read32(r+0x34,&table)||!span(table,297*4,&table)||!read32(table+slot*4,&m)||!span(m,0x50,&m))return 1;
    const uint8_t*v=(const uint8_t*)XBOX_PTR(m);
    if(word(v)!=0x4565B0u)return 1;
    /* 90E15 turns source header[8]!=FF into mode1. All five admitted mode1
     * slots are excluded by native constructor/dynamic receivers. Slot0 is
     * the exact untextured material (63/255 strength), also always excluded. */
    if(slot==0)return !(half(v+0x4C)==0&&v[0x24]==0&&word(v+0x44)==0x3E7CFCFEu&&
        word(v+8)==35&&word(v+0xC)==0&&word(v+0x1C)==1&&!word(v+0x28)&&!word(v+0x30));
    if(v[0x24]!=1||word(v+0x44)!=0)return 1;
    if(slot==264)return !(half(v+0x4C)==0x21&&word(v+0xC)==69);
    return !(half(v+0x4C)==(slot==290?8:0)&&word(v+0xC)==0);
}
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
extern void sub_000AF5F0(void);
static int spot_snapshot(uint32_t light,UCNativeControlSpot *out){
    uint32_t p,stack;float world[16],local[4],direction[4],radius,cone;
    if(!span(light,0x260,&p)||!span(g_esp-1024,1028,&stack)||
       ((uint64_t)p+0x260>stack&&(uint64_t)stack+1028>p))return 0;
    if(!take(light+0x10,world,64)||!take(light+0xC0,local,16)||!take(light+0xD0,direction,16)||
       !take(light+0xE0,&radius,4)||!take(light+0xE4,&cone,4)||!finite_values(world,16)||
       !finite_values(local,4)||!finite_values(direction,4)||!finite_values(&radius,1)||radius<=0||
       !finite_values(&cone,1)||cone<=0||cone>=1.5707963267948966f)return 0;
    float length=(direction[0]*direction[0]+direction[1]*direction[1])+direction[2]*direction[2];
    if(!finite_values(&length,1)||length<=0)return 0;
    // AF5F0 is the original pure geometry update also called by AFE10. Its
    // complete current CONTROL lift is retained; no Begin/material/GPU code.
    // Restore its caller-stack scratch as well as the guest CPU. Only the
    // original derived fields on this validated live light are published.
    uint8_t scratch[1028];memcpy(scratch,(const void*)XBOX_PTR(stack),sizeof(scratch));
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);g_ecx=p;g_esp=stack+1024;
    sub_000AF5F0();int valid=g_esp==stack+1028;
    memcpy((void*)XBOX_PTR(stack),scratch,sizeof(scratch));restore_cpu(&cpu);
    valid=valid&&take(light+0x130,out->projection,64)&&take(light+0x170,out->texture_bias,64)&&
        take(light+0x240,out->center,16)&&take(light+0x60,out->color,16)&&take(light+0x70,&out->intensity,4);
    out->radius=radius;
    return valid&&finite_values(out->projection,16)&&finite_values(out->texture_bias,16)&&finite_values(out->center,4)&&
        finite_values(out->color,4)&&finite_values(&out->intensity,1)&&out->intensity>=0;
}
#endif
void uc_hybrid_lighting_read_counts(uint64_t *out,unsigned count){
    if(out&&count<=12)memcpy(out,uc_hybrid_counts,count*sizeof(*out));
}
uint64_t uc_hybrid_lighting_scene_begin(void) {
    initialize();if(!enabled)return 0;
    if(GetCurrentThreadId()!=owner_tid){++uc_hybrid_counts[11];return 0;}
    ++depth;++uc_hybrid_counts[0];if(depth>HYBRID_SCOPE_LIMIT){++uc_hybrid_counts[7];return 0;}
    unsigned i=depth-1;memset(&scopes[i],0,sizeof(scopes[i]));
    /* Snapshot once per source scene: one atomic read, consistent partitions. */
    scopes[i].hide_models=!uc_hybrid_character_lighting_visible();
    scopes[i].simple_models=uc_hybrid_character_lighting_quality();
    scopes[i].scene.epoch=++serial;scopes[i].scene.scheduler=g_ecx;
    read32(g_esp,&scopes[i].scene.caller);
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
    scopes[i].level_root=westside_level();
    scopes[i].valid=camera(&scopes[i].scene);restore_cpu(&cpu);
    return scopes[i].scene.epoch;
}
int uc_hybrid_lighting_collect(uint32_t light,uint32_t iterator,uint32_t context,uint32_t shadow_permission) {
    (void)iterator;(void)context;initialize();if(!enabled)return 0;
    /* Only explicit CONTROL modes retain known original paths. Invalid
     * inputs, unknown scopes and unsupported subclasses stay suppressed. */
    if(GetCurrentThreadId()!=owner_tid){++uc_hybrid_counts[1];++uc_hybrid_counts[11];return 1;}
    /* The original Enabled predicate already passed at 98E5E. Its known spot
     * path does not depend on native camera/level/scope admission. Resume the
     * unchanged Begin/receiver/End before AF5F0 or any native preparation. */
    if(control_flashlight){
        uint32_t vtable;
        if(span(light,0xD8,NULL)&&read32(light,&vtable)&&vtable==0x457270u){++uc_hybrid_counts[4];return 0;}
    }
    if(!depth||depth>HYBRID_SCOPE_LIMIT){++uc_hybrid_counts[1];++uc_hybrid_counts[7];return 1;}
    unsigned i=depth-1;UcHybridLightingRecord r;memset(&r,0,sizeof(r));r.source_va=light;
    uint32_t canonical;
    if(!span(light,0xD8,&canonical)||!read32(light,&r.vtable)){++uc_hybrid_counts[1];++uc_hybrid_counts[6];return 1;}
    r.source_va=canonical;
    int original_models=control_model_scope()&&(r.vtable==0x457338u||r.vtable==0x4573F0u);
    if(!original_models)++uc_hybrid_counts[1];
    if(r.vtable==0x457338u){
        ++uc_hybrid_counts[3];
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
        if(extended&&scopes[i].valid){
            HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
            take(light+0x10,r.world,64);take(light+0xC0,r.local,16);take(light+0x60,r.color,16);
            take(light+0x70,&r.intensity,4);take(light+0x78,&r.source_index,4);
            if(finite_values(r.world,16)&&finite_values(r.local,4)&&finite_values(r.color,4)&&finite_values(&r.intensity,1))
                uc_hybrid_models_light(&scopes[i].scene,&r,iterator,context);
            else ++uc_hybrid_counts[6];restore_cpu(&cpu);
        }
#endif
        return !original_models;
    }
    if(r.vtable==0x457270u){
        ++uc_hybrid_counts[4];
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
        if((extended||flashlight)&&scopes[i].valid){
            HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);UCNativeControlSpot spot;int model_valid=0;
            take(light+0x10,r.world,64);take(light+0xC0,r.local,16);take(light+0x60,r.color,16);
            take(light+0x70,&r.intensity,4);take(light+0x78,&r.source_index,4);
            if(span(light,0xEC,NULL)){
                take(light+0xD0,r.direction,16);take(light+0xE0,&r.radius,4);take(light+0xE4,&r.cone,4);
                r.specular=*((const uint8_t*)XBOX_PTR(canonical+0xE8));
                r.shadow=shadow_permission&&*((const uint8_t*)XBOX_PTR(canonical+0x75));
                model_valid=finite_values(r.world,16)&&finite_values(r.local,4)&&finite_values(r.direction,4)&&finite_values(r.color,4)&&
                   finite_values(&r.intensity,1)&&finite_values(&r.radius,1)&&finite_values(&r.cone,1);
            }
            unsigned n=scopes[i].spot_count,k=0;for(;k<n;k++)if(scopes[i].spot_sources[k]==canonical)break;
            if(scopes[i].scene.caller==0x9EA8Au||(extended&&scopes[i].scene.caller==0x9ED75u)){
                r.spot_valid=spot_snapshot(light,&spot);
                if(r.spot_valid)r.spot=spot;else ++uc_hybrid_counts[6];
            }
            if(flashlight&&scopes[i].scene.caller==0x9EA8Au){ // Scenery stays in its world scope.
                if(k==HYBRID_LIGHT_LIMIT)++uc_hybrid_counts[7];
                else if(r.spot_valid){
                    scopes[i].spots[k]=spot;scopes[i].spot_sources[k]=canonical;if(k==n)++scopes[i].spot_count;
                }
            }
            if(model_valid&&(extended||(flashlight&&flashlight_models&&scopes[i].scene.caller==0x9EA8Au)))
                uc_hybrid_models_light(&scopes[i].scene,&r,iterator,context);
            restore_cpu(&cpu);
        }
#endif
        return 1;
    }
    if(r.vtable!=0x4573F0u){++uc_hybrid_counts[5];return 1;}
    ++uc_hybrid_counts[2];
    /* CONTROL owns this private weapon pass. apply rejects its camera before
     * any native consumer, and CONTROL_MODELS already disables extended work. */
    if(original_models&&scopes[i].scene.caller==0x9ED75u)return 0;
    /* Unsupported levels cannot reach native scenery apply. Preserve original
     * model admission without copying records that have no native consumer. */
    if(!extended&&!scopes[i].level_root)return !original_models;
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
    take(light+0x10,r.world,64);take(light+0xC0,r.local,16);take(light+0x60,r.color,16);
    take(light+0x70,&r.intensity,4);take(light+0x78,&r.source_index,4);take(light+0xD0,&r.radius,4);
    uint32_t p;span(light,0xD8,&p);
    r.shadow=shadow_permission&&*((const uint8_t*)XBOX_PTR(p+0x75));
    r.specular=*((const uint8_t*)XBOX_PTR(p+0xD4));
    int valid=finite_values(r.world,16)&&finite_values(r.local,4)&&finite_values(r.color,4)&&
        finite_values(&r.intensity,1)&&finite_values(&r.radius,1)&&r.radius>0&&r.intensity>=0;
    if(valid){
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
        if(extended&&scopes[i].valid)uc_hybrid_models_light(&scopes[i].scene,&r,iterator,context);
#endif
        unsigned n=scopes[i].count,k=0;for(;k<n;k++)if(scopes[i].lights[k].source_va==canonical)break;
        if(k==HYBRID_LIGHT_LIMIT)++uc_hybrid_counts[7];
        else {if(k<n)r.shadow|=scopes[i].lights[k].shadow;scopes[i].lights[k]=r;if(k==n)++scopes[i].count;}
    }else ++uc_hybrid_counts[6];
    restore_cpu(&cpu);return !original_models;
}
void uc_hybrid_lighting_scene_apply(uint64_t token) {
    if(!enabled||!depth||depth>HYBRID_SCOPE_LIMIT)return;
    if(GetCurrentThreadId()!=owner_tid){++uc_hybrid_counts[11];return;}
    unsigned i=depth-1;if(!token||token!=scopes[i].scene.epoch||scopes[i].applied)return;
    scopes[i].applied=1;
    /* This late pass covers world scenery only. Model contributions already
     * run synchronously during their original light visitor, including FP. */
    if(scopes[i].scene.caller!=0x9EA8Au){++uc_hybrid_counts[10];return;}
    HybridCPU cpu;save_cpu(&cpu);_mm_setcsr(0x1F80);
    UcHybridLightingScene current;memset(&current,0,sizeof(current));
    const char*reason=NULL;
    if(!scopes[i].level_root||westside_level()!=scopes[i].level_root)reason="unsupported level";
    else if(!scopes[i].valid||!camera(&current)||!same_camera(&scopes[i].scene,&current))reason="camera changed";
    int ok=0;
    if(reason)uc_hybrid_lighting_host_reject(reason);
    else {
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
        if(extended)uc_hybrid_casters_collect(&scopes[i].scene,scopes[i].lights,scopes[i].count);
#endif
        ok=uc_hybrid_lighting_host_apply(&scopes[i].scene,scopes[i].lights,scopes[i].count);
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
        if(ok&&flashlight&&!control_flashlight)ok=uc_hybrid_lighting_host_spots(&scopes[i].scene,scopes[i].spots,scopes[i].spot_count);
#endif
    }
    ++uc_hybrid_counts[ok?8:9];restore_cpu(&cpu);
}
void uc_hybrid_lighting_scene_end(uint64_t token) {
    if(!enabled||!depth||GetCurrentThreadId()!=owner_tid)return;
    if(depth<=HYBRID_SCOPE_LIMIT&&token!=scopes[depth-1].scene.epoch){++uc_hybrid_counts[6];return;}
    --depth;
    /* A separate diagnostic run writes once at a bounded scene boundary.
     * Benchmark launchers leave this off. ExitProcess does not run atexit. */
    if(diagnostic&&!depth&&uc_hybrid_counts[0]>=1024){
        diagnostic=0;wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);
        wchar_t *p=wcsrchr(path,L'\\');if(!n||n>=MAX_PATH||!p)return;
        swprintf(p+1,MAX_PATH-(p+1-path),L"hybrid-lighting-%lu.json",GetCurrentProcessId());
        FILE *f=_wfopen(path,L"wb");if(!f)return;
        fputs("{\"guest_counts\":[",f);for(unsigned j=0;j<12;j++)fprintf(f,"%s%llu",j?",":"",(unsigned long long)uc_hybrid_counts[j]);
        fputs("],\"host\":",f);HybridCPU cpu;save_cpu(&cpu);
        uc_hybrid_lighting_host_capture(f);restore_cpu(&cpu);fputs("}\n",f);fclose(f);
    }
}
