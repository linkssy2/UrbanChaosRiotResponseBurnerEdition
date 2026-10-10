#include "migration_particles_visual_routes_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F8FC0(void),sub_00208C50(void),sub_00219450(void);
static int route_enabled(void){static int f=-1;if(f<0){const char*p=getenv("UCRR_NATIVE_EFFECT_VISUAL_ROUTES");f=p&&!strcmp(p,"1");}return f;}
static int route_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int route_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
#define ROUTE_CALL(fn,pc) do{g_ebp=frame;PUSH32(g_esp,pc);fn();}while(0)
#define ROUTE_VCALL(target,pc,saved) do{uint32_t t_=(target);PUSH32(g_esp,pc);RECOMP_ICALL_SAFE(t_,saved);}while(0)
static void attached_world_point(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-92;
 g_eax=MEM32(g_ecx+128);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);ROUTE_CALL(sub_001F8FC0,0x174566);g_esi=g_eax;g_esp+=4;
 if(g_esi){
  g_ecx=MEM32(frame+12);g_edx=MEM32(g_esi);g_eax=g_esp+16;
  {uint32_t s=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;ROUTE_VCALL(MEM32(g_edx+20),0x174583,s);}
  if(LO8(g_eax)){
   g_edx=MEM32(g_esi);g_eax=g_esp+32;{uint32_t s=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;ROUTE_VCALL(MEM32(g_edx+64),0x174593,s);}
   /* Local bone point to world space, in original column/operand order. */
   g_xmm1=XMM_MEM(g_eax+48);g_xmm3=XMM_MEM(g_eax+32);g_ecx=g_esp+16;g_xmm0=XMM_MEM(g_ecx);
   g_xmm2=g_xmm0;g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm3=XMM_MUL(g_xmm3,g_xmm2);g_xmm2=XMM_MEM(g_eax+16);g_xmm3=XMM_ADD(g_xmm3,g_xmm1);
   g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0x55);g_xmm2=XMM_MUL(g_xmm2,g_xmm1);g_xmm1=g_xmm0;g_xmm1=XMM_SHUFFLE(g_xmm1,g_xmm0,0);
   g_xmm0=XMM_MEM(g_eax);g_eax=MEM32(frame+8);g_xmm0=XMM_MUL(g_xmm0,g_xmm1);g_xmm0=XMM_ADD(g_xmm0,g_xmm2);g_xmm0=XMM_ADD(g_xmm0,g_xmm3);XMM_STORE(g_esp+16,g_xmm0);
   for(unsigned axis=0;axis<3;axis++){g_xmm0=XMM_SCALAR(MEMF(g_esp+16+4*axis));MEMF(g_eax+4*axis)=g_xmm0.f[0];}
   POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=0;g_esp+=12;return;
  }
 }
 g_eax=MEM32(frame+8);g_xmm0=XMM_ZERO();for(unsigned axis=0;axis<3;axis++)MEMF(g_eax+4*axis)=g_xmm0.f[0];POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_esp+=12;
}
static void emit_flare_particles(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0u)-72;
 g_xmm0=XMM_MEM(g_ecx+48);g_eax=MEM32(frame+8);g_eax=MEM32(g_eax);XMM_STORE(g_esp+8,g_xmm0);g_xmm0=XMM_MEM(g_ecx+32);XMM_STORE(g_esp+24,g_xmm0);g_xmm0=XMM_MEM(g_ecx+16);
 g_eax-=3;int z=g_eax==22;PUSH32(g_esp,g_esi);XMM_STORE(g_esp+44,g_xmm0);g_xmm0=XMM_MEM(g_ecx+64);PUSH32(g_esp,g_edi);XMM_STORE(g_esp+64,g_xmm0);
 if(g_eax<=22){g_ecx=MEM8(0x1871dc+g_eax);if(MEM32(0x1871d4+4*g_ecx)==0x18719a){
  g_ecx=MEM32(0x51c4dc);ROUTE_CALL(sub_00208C50,0x1871a5);g_esi=g_eax;g_edx=g_esp+12;PUSH32(g_esp,g_edx);g_eax=g_esp+20;MEM32(g_esp+16)=0x472d60;g_edi=MEM32(g_esi);PUSH32(g_esp,g_eax);ROUTE_CALL(sub_00219450,0x1871c0);
  g_esp+=4;{uint32_t s=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;ROUTE_VCALL(MEM32(g_edi+16),0x1871c9,s);}
 }}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=z;g_esp+=12;
}
int uc_migration_particles_visual_routes(unsigned entry){
 if(!route_enabled())return 0;uint32_t s=g_esp,p=g_ecx;
 if((s&3)||s<0x10100u||!route_span(s-256,272)||!route_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(entry==0x174550){
  if(!route_span(p,132)||!route_apart(p,132,s-256,272)||!route_span(MEM32(s+4),12)||!route_apart(MEM32(s+4),12,s-256,272))return 0;
  attached_world_point();return 1;
 }
 if(entry==0x187150){
  if(!route_span(p,80)||!route_apart(p,80,s-256,272)||!route_span(MEM32(s+4),4)||!route_apart(MEM32(s+4),4,s-256,272)||!route_span(0x1871d4,31)||!route_span(0x51c4dc,4))return 0;
  unsigned type=MEM32(MEM32(s+4))-3;if(type<=22){unsigned i=MEM8(0x1871dc+type);if(i>1)return 0;unsigned target=MEM32(0x1871d4+4*i);if(target!=0x18719a&&target!=0x1871c9)return 0;}
  emit_flare_particles();return 1;
 }
 return 0;
}
#undef ROUTE_CALL
#undef ROUTE_VCALL
