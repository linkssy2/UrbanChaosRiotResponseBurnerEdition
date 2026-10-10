#include "hybrid_particle_birth_owner_guest.h"
#include "hybrid_particle_birth_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002D3670(void),sub_002D3760(void),sub_002D3780(void),sub_002D681C(void);
static int birth_owner_enabled(void){static int flag=-1;if(flag<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_BIRTH_OWNER");flag=p&&!strcmp(p,"1");}return flag;}
/* Complete original239510 particle birth over the live descriptor, source
 * matrices and output record. Shared RNG/CRT calls remain ordered services;
 * shape-specific rounding, residual FP state and in-place stores are retained.
 * No failure/fallback is possible after the first original RNG draw. */
#define birth_push(v) do{double value=(v);g_fp_top=(g_fp_top+7u)&7u;g_fp_stack[g_fp_top]=value;}while(0)
#define birth_pop() (g_fp_top=(g_fp_top+1u)&7u)
#define birth_top() g_fp_stack[g_fp_top]
#define birth_st(i) g_fp_stack[(g_fp_top+(i))&7u]
#define birth_st1() birth_st(1)
static void birth_service(void(*service)(void),uint32_t ret,uint32_t frame){g_ebp=frame;PUSH32(g_esp,ret);service();}
static int birth_view(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int birth_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}

static void birth_hemisphere(uint32_t frame){
 birth_service(sub_002D3760,0x0023953Du,frame);
 MEMF(g_esp + 0x20) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F800000);
 PUSH32(g_esp, 0xBF800000u);
 birth_service(sub_002D3780,0x00239550u,frame);
 MEMF(g_esp + 0x2C) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F800000);
 PUSH32(g_esp, 0xBF800000u);
 birth_service(sub_002D3780,0x00239563u,frame);
 MEMF(g_esp + 0x38) = (float)birth_top();
 birth_top() = birth_top() * MEMF(g_esp + 0x38);
 PUSH32(g_esp, 0x3F000000);
 birth_push(MEMF(g_esp + 0x38));
 PUSH32(g_esp, 0xBF000000u);
 birth_top() = birth_top() * MEMF(g_esp + 0x3C);
 birth_st1() = birth_st1() + birth_top(); birth_pop();
 birth_push(MEMF(g_esp + 0x38));
 birth_top() = birth_top() * MEMF(g_esp + 0x38);
 birth_st1() = birth_st1() + birth_top(); birth_pop();
 birth_top() = sqrt(birth_top());
 birth_top() = MEMF(0x44BF94) / birth_top();
 MEMF(g_esp + 0x30) = (float)birth_top(); birth_pop();
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x30));
 g_xmm1 = g_xmm0;
 g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x38);
 MEMF(g_esp + 0x38) = g_xmm1.f[0];
 g_xmm1 = g_xmm0;
 g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x3C);
 g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x40);
 MEMF(g_esp + 0x3C) = g_xmm1.f[0];
 MEMF(g_esp + 0x40) = g_xmm0.f[0];
 birth_service(sub_002D3780,0x002395CAu,frame);
 MEMF(g_esp + 0x30) = (float)birth_top(); birth_pop();
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x30));
 g_esp = g_esp + 0x18;
}

static void birth_sphere(uint32_t frame){
 PUSH32(g_esp, 0x3F800000);
 PUSH32(g_esp, 0xBF800000u);
 birth_service(sub_002D3780,0x00239603u,frame);
 MEMF(g_esp + 0x24) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F800000);
 PUSH32(g_esp, 0xBF800000u);
 birth_service(sub_002D3780,0x00239616u,frame);
 MEMF(g_esp + 0x28) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F800000);
 PUSH32(g_esp, 0xBF800000u);
 birth_service(sub_002D3780,0x00239629u,frame);
 MEMF(g_esp + 0x4C) = (float)birth_top(); birth_pop();
 g_ecx = MEM32(g_esp + 0x34);
 g_eax = MEM32(g_esp + 0x30);
 g_edx = MEM32(g_esp + 0x4C);
 MEM32(g_esp + 0x40) = g_ecx;
 birth_push(MEMF(g_esp + 0x40));
 birth_top() = birth_top() * MEMF(g_esp + 0x40);
 MEM32(g_esp + 0x3C) = g_eax;
 birth_push(MEMF(g_esp + 0x3C));
 MEM32(g_esp + 0x38) = g_edx;
 birth_top() = birth_top() * MEMF(g_esp + 0x3C);
 PUSH32(g_esp, 0x3F000000);
 PUSH32(g_esp, 0xBF000000u);
 birth_st1() = birth_st1() + birth_top(); birth_pop();
 birth_push(MEMF(g_esp + 0x40));
 MEM32(g_esp + 0x58) = g_eax;
 birth_top() = birth_top() * MEMF(g_esp + 0x40);
 MEM32(g_esp + 0x5C) = g_ecx;
 birth_st1() = birth_st1() + birth_top(); birth_pop();
 birth_top() = sqrt(birth_top());
 birth_top() = MEMF(0x44BF94) / birth_top();
 MEMF(g_esp + 0x3C) = (float)birth_top(); birth_pop();
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x3C));
 g_xmm1 = g_xmm0;
 g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x40);
 MEMF(g_esp + 0x40) = g_xmm1.f[0];
 g_xmm1 = g_xmm0;
 g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x44);
 g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x48);
 MEMF(g_esp + 0x44) = g_xmm1.f[0];
 MEMF(g_esp + 0x48) = g_xmm0.f[0];
 birth_service(sub_002D3780,0x002396B4u,frame);
 MEMF(g_esp + 0x3C) = (float)birth_top(); birth_pop();
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x3C));
 g_esp = g_esp + 0x20;
}

static void birth_box(uint32_t frame){
 PUSH32(g_esp, 0x3F000000);
 PUSH32(g_esp, 0xBF000000u);
 birth_service(sub_002D3780,0x002396D5u,frame);
 MEMF(g_esp + 0x20) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F000000);
 PUSH32(g_esp, 0xBF000000u);
 birth_service(sub_002D3780,0x002396E8u,frame);
 MEMF(g_esp + 0x2C) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0x3F000000);
 PUSH32(g_esp, 0xBF000000u);
 birth_service(sub_002D3780,0x002396FBu,frame);
 MEMF(g_esp + 0x4C) = (float)birth_top(); birth_pop();
 g_edx = MEM32(g_esp + 0x34);
 g_eax = MEM32(g_esp + 0x30);
 g_ecx = MEM32(g_esp + 0x4C);
 g_esp = g_esp + 0x18;
 MEM32(g_esp + 0x38) = g_edx;
 MEM32(g_esp + 0x3C) = g_eax;
 MEM32(g_esp + 0x20) = g_ecx;
 MEM32(g_esp + 0x24) = g_edx;
 MEM32(g_esp + 0x28) = g_eax;
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x28));
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x24));
 g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x20));
}

int uc_hybrid_particle_birth_owner(void){
 if(!birth_owner_enabled())return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10800u||!birth_view(s-2048,2072)||!birth_view(0x10000,0x5f0000)||!birth_view(0x51c178,1)||MEM8(0x51c178)||
    (unsigned)g_fp_top>7||(_mm_getcsr()&0x1f80u)!=0x1f80u)return 0;
 uint32_t descriptor=MEM32(s+4),matrix=MEM32(s+8),previous=MEM32(s+12),output=MEM32(s+20);
 if(!birth_view(descriptor,44)||MEM32(descriptor)>2||!birth_view(matrix,64)||!birth_view(previous,64)||!birth_view(output,32)||
    !birth_view(0x51dd10,1004)||!birth_view(0x504670,4)||MEM32(0x51e0f8)>=250||MEM32(0x504670)>=250)return 0;
 if((MEM32(descriptor+28)&0x7f800000u)==0x7f800000u||(MEM32(descriptor+32)&0x7f800000u)==0x7f800000u||
    MEM32(0x492d20)!=0x30000000u||MEM32(0x44bf80)!=0xbf800000u||MEM32(0x44bf94)!=0x3f800000u||MEM32(0x44c5a8)!=0x40490fdbu)return 0;
 const uint32_t reads[]={descriptor,matrix,previous,output};const unsigned sizes[]={44,64,64,32};
 for(unsigned i=0;i<4;i++)if(!birth_apart(reads[i],sizes[i],s-2048,2072))return 0;
 /* Keep the original acos input finite through the preceding publication.
  * Exceptional CRT domains decline before any RNG draw, not after effects. */
 if(!birth_apart(output,32,descriptor+28,8)||!birth_apart(descriptor+28,8,0x51dd10,1004)||!birth_apart(descriptor+28,8,0x504670,4))return 0;
 const uint32_t fixed[]={0x51dd10,0x504670,0x51c178,0x492d20,0x44bf80,0x44bf94,0x44c5a8};const unsigned fixed_sizes[]={1004,4,1,4,4,4,4};
 for(unsigned i=0;i<7;i++)if(!birth_apart(s-2048,2072,fixed[i],fixed_sizes[i])||!birth_apart(output,32,fixed[i],fixed_sizes[i]))return 0;
 /* The original callers own valid completing RNG/CRT services. The globals
  * remain guest-owned; no copied stream or host particle graph is introduced. */
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-68;
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+8);g_eax=MEM32(g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(!g_eax)birth_box(frame);
 else{
  --g_eax;if(!g_eax)birth_sphere(frame);else{--g_eax;birth_hemisphere(frame);}
 g_xmm1 = g_xmm2;
 g_xmm1.f[0] = g_xmm1.f[0] * MEMF(g_esp + 0x20);
 g_xmm0 = g_xmm2;
 g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x24);
 g_xmm2.f[0] = g_xmm2.f[0] * MEMF(g_esp + 0x28);
 }
 g_xmm3 = XMM_SCALAR(MEMF(g_ebx + 4));
 g_edi = MEM32(frame + 0xC);
 g_xmm3.f[0] = g_xmm3.f[0] * g_xmm1.f[0];
 g_xmm1 = XMM_SCALAR(MEMF(g_ebx + 8));
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0xC));
 g_xmm0.f[0] = g_xmm0.f[0] * g_xmm2.f[0];
 MEMF(g_esp + 0x28) = g_xmm0.f[0];
 MEMF(g_esp + 0x24) = g_xmm1.f[0];
 g_xmm1 = XMM_MEM(g_edi + 0x30);
 MEMF(g_esp + 0x20) = g_xmm3.f[0];
 g_xmm3 = XMM_MEM(g_edi + 0x20);
 g_ecx = g_esp + 0x20;
 g_xmm0 = XMM_MEM(g_ecx);
 g_xmm2 = g_xmm0;
 g_xmm2 = XMM_SHUFFLE(g_xmm2, g_xmm0, 0xAA);
 g_xmm3 = XMM_MUL(g_xmm3, g_xmm2);
 g_xmm2 = XMM_MEM(g_edi + 0x10);
 g_xmm3 = XMM_ADD(g_xmm3, g_xmm1);
 g_xmm1 = g_xmm0;
 g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0x55);
 g_xmm2 = XMM_MUL(g_xmm2, g_xmm1);
 g_xmm1 = g_xmm0;
 g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);
 g_xmm0 = XMM_MEM(g_edi);
 g_xmm0 = XMM_MUL(g_xmm0, g_xmm1);
 g_xmm1 = XMM_SCALAR(MEMF(g_edi + 0x34));
 g_xmm0 = XMM_ADD(g_xmm0, g_xmm2);
 g_xmm2 = XMM_SCALAR(MEMF(g_edi + 0x38));
 g_xmm0 = XMM_ADD(g_xmm0, g_xmm3);
 XMM_STORE(g_esp + 0x40, g_xmm0);
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x40));
 MEMF(g_esp + 0x34) = g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x44));
 g_edx = MEM32(g_esp + 0x34);
 MEMF(g_esp + 0x38) = g_xmm0.f[0];
 g_eax = MEM32(g_esp + 0x38);
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x48));
 MEMF(g_esp + 0x3C) = g_xmm0.f[0];
 g_ecx = MEM32(g_esp + 0x3C);
 g_xmm0 = XMM_SCALAR(MEMF(g_edi + 0x30));
 MEM32(g_esp + 0x24) = g_eax;
 g_eax = MEM32(frame + 0x10);
 g_xmm3 = XMM_SCALAR(MEMF(g_eax + 0x30));
 g_xmm4 = XMM_SCALAR(MEMF(g_eax + 0x34));
 g_xmm5 = XMM_SCALAR(MEMF(g_eax + 0x38));
 g_xmm0.f[0] = g_xmm0.f[0] - g_xmm3.f[0];
 g_xmm1.f[0] = g_xmm1.f[0] - g_xmm4.f[0];
 g_xmm2.f[0] = g_xmm2.f[0] - g_xmm5.f[0];
 MEM32(g_esp + 0x20) = g_edx;
 MEM32(g_esp + 0x28) = g_ecx;
 MEMF(g_esp + 0x34) = g_xmm0.f[0];
 MEMF(g_esp + 0x38) = g_xmm1.f[0];
 MEMF(g_esp + 0x3C) = g_xmm2.f[0];
 birth_service(sub_002D3760,0x00239826u,frame);
 MEMF(g_esp + 0x1C) = (float)birth_top(); birth_pop();
 g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x1C));
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x34));
 g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x38));
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x3C));
 g_esi = MEM32(frame + 0x18);
 g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
 g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x20));
 g_xmm3.f[0] = g_xmm3.f[0] - g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x24));
 g_xmm0.f[0] = g_xmm0.f[0] - g_xmm1.f[0];
 MEMF(g_esp + 0x24) = g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x28));
 g_ecx = MEM32(g_esp + 0x24);
 MEMF(g_esp + 0x20) = g_xmm3.f[0];
 g_eax = MEM32(g_esp + 0x20);
 g_edx = g_esi;
 MEM32(g_edx) = g_eax;
 g_xmm0.f[0] = g_xmm0.f[0] - g_xmm2.f[0];
 MEMF(g_esp + 0x28) = g_xmm0.f[0];
 g_eax = MEM32(g_esp + 0x28);
 g_xmm0 = XMM_SCALAR(MEMF(frame + 0x14));
 MEM32(g_edx + 4) = g_ecx;
 MEM32(g_edx + 8) = g_eax;
 MEMF(g_esi + 0xC) = g_xmm0.f[0];
 birth_service(sub_002D3760,0x002398A6u,frame);
 birth_push(MEMF(g_ebx + 0x1C));
 birth_top() = birth_top() * MEMF(0x44C5A8);
 birth_top() = cos(birth_top());
 birth_top() = MEMF(0x44BF94) - birth_top();
 birth_push(MEMF(g_ebx + 0x20));
 birth_top() = birth_top() * MEMF(0x44C5A8);
 birth_top() = cos(birth_top());
 birth_top() = MEMF(0x44BF94) - birth_top();
 birth_top() = birth_top() - birth_st1();
 birth_top() = birth_top() * g_fp_stack[(g_fp_top + 2) & 7];
 birth_top() = birth_top() + birth_st1();
 birth_top() = MEMF(0x44BF94) - birth_top();
 MEMF(g_esp + 0x18) = (float)birth_top(); birth_pop();
 birth_pop();
 birth_pop();
 birth_push(MEMF(g_esp + 0x18));
 birth_push(MEMF(0x44BF80));
 g_fp_cmp = RECOMP_FCMP(birth_top(), birth_st1()); birth_pop();
 birth_pop();
 if(g_fp_cmp>0){g_xmm0=XMM_SCALAR(MEMF(0x44bf80));MEMF(g_esp+0x18)=g_xmm0.f[0];}
 else{
  g_xmm1=XMM_SCALAR(MEMF(g_esp+0x18));g_xmm0=XMM_SCALAR(MEMF(0x44bf94));
  double a=g_xmm1.f[0],b=g_xmm0.f[0];unsigned flags=(a!=a||b!=b)?0x47u:a<b?3u:a>b?2u:0x42u;
  if(!(flags&0x41u))MEMF(g_esp+0x18)=g_xmm0.f[0];
 }
 birth_push(MEMF(g_esp + 0x18));
 birth_service(sub_002D681C,0x00239918u,frame);
 PUSH32(g_esp, 0x40C90FDB);
 MEMF(g_esp + 0x1C) = (float)birth_top(); birth_pop();
 PUSH32(g_esp, 0);
 birth_service(sub_002D3780,0x00239928u,frame);
 /* Reuse the independently gated, already qualified suffix at its source seam;
  * a local decline continues native semantics, never replays birth effects. */
 if(!(uc_hybrid_particle_birth(g_esp,frame))){
 birth_push(MEMF(g_esp + 0x20));
 birth_top() = cos(birth_top());
 g_xmm2 = XMM_MEM(g_edi + 0x20);
 g_xmm3 = XMM_MEM(g_edi + 0x10);
 g_esp = g_esp + 8;
 MEMF(g_esp + 0x34) = (float)birth_top(); birth_pop();
 birth_push(MEMF(g_esp + 0x18));
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x34));
 birth_top() = sin(birth_top());
 MEMF(g_esp + 0x20) = g_xmm0.f[0];
 { double _t = birth_st1(); birth_push(_t); }
 birth_top() = cos(birth_top());
 birth_top() = birth_top() * birth_st1();
 MEMF(g_esp + 0x38) = (float)birth_top(); birth_pop();
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x38));
 { double _t = birth_top(); birth_top() = birth_st1(); birth_st1() = _t; }
 MEMF(g_esp + 0x24) = g_xmm0.f[0];
 birth_top() = sin(birth_top());
 birth_top() = birth_top() * birth_st1();
 MEMF(g_esp + 0x3C) = (float)birth_top(); birth_pop();
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x3C));
 MEMF(g_esp + 0x28) = g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(0x44C174));
 birth_pop();
 MEMF(g_esp + 0x2C) = g_xmm0.f[0];
 g_xmm0 = XMM_MEM(g_esp + 0x20);
 g_xmm1 = g_xmm0;
 g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0xAA);
 g_xmm2 = XMM_MUL(g_xmm2, g_xmm1);
 g_xmm1 = g_xmm0;
 g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0x55);
 g_xmm3 = XMM_MUL(g_xmm3, g_xmm1);
 g_xmm1 = g_xmm0;
 g_xmm1 = XMM_SHUFFLE(g_xmm1, g_xmm0, 0);
 g_xmm0 = XMM_MEM(g_edi);
 g_xmm0 = XMM_MUL(g_xmm0, g_xmm1);
 g_xmm0 = XMM_ADD(g_xmm0, g_xmm3);
 g_xmm0 = XMM_ADD(g_xmm0, g_xmm2);
 XMM_STORE(g_esp + 0x40, g_xmm0);
 birth_service(sub_002D3760,0x002399C1u,frame);
 MEMF(g_esp + 0x1C) = (float)birth_top(); birth_pop();
 g_xmm0 = XMM_SCALAR(MEMF(g_ebx + 0x28));
 g_xmm0.f[0] = g_xmm0.f[0] - MEMF(g_ebx + 0x24);
 g_xmm0.f[0] = g_xmm0.f[0] * MEMF(g_esp + 0x1C);
 g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_ebx + 0x24);
 g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x40));
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x44));
 g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x48));
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
 MEMF(g_esp + 0x34) = g_xmm1.f[0];
 g_edx = MEM32(g_esp + 0x34);
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
 MEMF(g_esp + 0x38) = g_xmm2.f[0];
 g_eax = MEM32(g_esp + 0x38);
 g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
 g_xmm0 = XMM_SCALAR(MEMF(0x44C53C));
 g_ecx = g_esi + 0x10;
 MEM32(g_ecx) = g_edx;
 MEMF(g_esp + 0x3C) = g_xmm3.f[0];
 g_edx = MEM32(g_esp + 0x3C);
 MEM32(g_ecx + 4) = g_eax;
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm0.f[0];
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm0.f[0];
 g_xmm3.f[0] = g_xmm3.f[0] * g_xmm0.f[0];
 MEM32(g_ecx + 8) = g_edx;
 MEMF(g_esp + 0x34) = g_xmm1.f[0];
 MEMF(g_esp + 0x38) = g_xmm2.f[0];
 MEMF(g_esp + 0x3C) = g_xmm3.f[0];
 birth_service(sub_002D3760,0x00239A4Cu,frame);
 g_xmm0 = XMM_SCALAR(MEMF(g_esp + 0x34));
 MEMF(g_esp + 0x1C) = (float)birth_top(); birth_pop();
 g_xmm3 = XMM_SCALAR(MEMF(g_esp + 0x1C));
 g_xmm1 = XMM_SCALAR(MEMF(g_esp + 0x38));
 g_xmm2 = XMM_SCALAR(MEMF(g_esp + 0x3C));
 g_xmm0.f[0] = g_xmm0.f[0] * g_xmm3.f[0];
 g_xmm0.f[0] = g_xmm0.f[0] + MEMF(g_esi);
 g_xmm1.f[0] = g_xmm1.f[0] * g_xmm3.f[0];
 g_xmm1.f[0] = g_xmm1.f[0] + MEMF(g_esi + 4);
 g_xmm2.f[0] = g_xmm2.f[0] * g_xmm3.f[0];
 g_xmm2.f[0] = g_xmm2.f[0] + MEMF(g_esi + 8);
 MEMF(g_esi) = g_xmm0.f[0];
 MEMF(g_esi + 4) = g_xmm1.f[0];
 MEMF(g_esi + 8) = g_xmm2.f[0];
 MEM16(g_esi + 0x1C) = 0;
 birth_service(sub_002D3670,0x00239A9Bu,frame);
 }
 POP32(g_esp,g_edi);MEM16(g_esi+30)=LO16(g_eax);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;return 1;
}
#undef birth_push
#undef birth_pop
#undef birth_top
#undef birth_st
#undef birth_st1
