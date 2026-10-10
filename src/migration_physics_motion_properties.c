/* Original mass, inertia and velocity operations for the original motion classes. */
#include "migration_physics_motion_properties.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_00388A00(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
static void exchange_fp(void){unsigned next=(g_fp_top+1)&7;double v=g_fp_stack[g_fp_top];g_fp_stack[g_fp_top]=g_fp_stack[next];g_fp_stack[next]=v;}
static unsigned status(unsigned mask){unsigned s=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);SET_LO16(g_eax,s);return ((g_eax>>8)&255)&mask;}
static void set_mass(int box){
 if(box){push_fp(MEMF(0x44bf94));g_eax=MEM32(g_ecx);g_fp_stack[g_fp_top]/=MEMF(g_esp+4);uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];pop_fp();uint32_t target=MEM32(g_eax+36);PUSH32(g_esp,0x2f4e03);RECOMP_ICALL_SAFE(target,saved);g_esp+=8;}
 else{uint32_t frame=g_seh_ebp;push_fp(MEMF(0x44c174));push_fp(MEMF(g_esp+4));g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);pop_fp();pop_fp();unsigned flags=status(0x44);if(RECOMP_PARITY8(flags)){push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_esp+4);MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];pop_fp();}else MEM32(g_esp+4)=0;g_edx=MEM32(g_esp+4);g_eax=MEM32(g_ecx);MEM32(g_esp+4)=g_edx;g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+36));}
}
static void export_inertia(unsigned kind){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;
 if(kind==2){g_eax=MEM32(frame+8);g_xmm0=XMM_ZERO();XMM_STORE(g_eax,g_xmm0);XMM_STORE(g_eax+16,g_xmm0);g_esp&=~15u;XMM_STORE(g_eax+32,g_xmm0);}
 else{if(kind){g_esp=(g_esp&~15u)-16;}push_fp(MEMF(0x44bf94));g_eax=MEM32(frame+8);g_fp_stack[g_fp_top]/=MEMF(g_ecx+(kind?240:196));g_xmm0=XMM_ZERO();if(kind){push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_ecx+244);push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_ecx+248);}XMM_STORE(g_eax,g_xmm0);XMM_STORE(g_eax+16,g_xmm0);XMM_STORE(g_eax+32,g_xmm0);
  if(kind){MEMF(g_esp+12)=(float)g_fp_stack[g_fp_top];pop_fp();exchange_fp();MEMF(g_eax)=(float)g_fp_stack[g_fp_top];pop_fp();g_ecx=MEM32(g_esp+12);MEMF(g_eax+20)=(float)g_fp_stack[g_fp_top];pop_fp();MEM32(g_eax+40)=g_ecx;}
  else{g_esp&=~15u;MEMF(g_eax)=(float)g_fp_stack[g_fp_top];MEMF(g_eax+20)=(float)g_fp_stack[g_fp_top];MEMF(g_eax+40)=(float)g_fp_stack[g_fp_top];pop_fp();}}
 g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void sphere_inertia(void){
 g_edx=MEM32(g_esp+4);push_fp(MEMF(g_edx+20));push_fp(MEMF(g_edx));MEMF(g_esp+4)=(float)g_fp_stack[g_fp_top];g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);pop_fp();if(!status(0x41)){pop_fp();push_fp(MEMF(g_esp+4));}g_eax=MEM32(g_edx+40);MEM32(g_esp+4)=g_eax;g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(g_esp+4));if(status(0x41)){pop_fp();push_fp(MEMF(g_esp+4));}g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],MEMF(0x44c174));unsigned flags=status(0x41);
 if(!flags){push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=g_fp_stack[(g_fp_top+1)&7];MEMF(g_ecx+196)=(float)g_fp_stack[g_fp_top];pop_fp();pop_fp();}else{MEM32(g_ecx+196)=0;pop_fp();}g_recomp_zf=flags==0;g_esp+=8;
}
static void box_inertia(void){
 g_eax=MEM32(g_esp+4);push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_eax+20);g_edx=MEM32(g_ecx+252);MEM32(g_esp+4)=g_edx;push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_eax+40);push_fp(MEMF(0x44bf94));g_fp_stack[g_fp_top]/=MEMF(g_eax);g_eax=g_edx;MEMF(g_ecx+240)=(float)g_fp_stack[g_fp_top];pop_fp();exchange_fp();MEMF(g_ecx+244)=(float)g_fp_stack[g_fp_top];pop_fp();MEMF(g_ecx+248)=(float)g_fp_stack[g_fp_top];pop_fp();MEM32(g_ecx+252)=g_eax;g_esp+=8;
}
int uc_migration_physics_motion_properties(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_MOTION_PROPERTIES");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,144)||!span(0x44bf94,4)||!span(0x44c174,4))return 0;
 if(entry==0x2f33d0||entry==0x2f4e10||entry==0x32a670||entry==0x302660||entry==0x31e3a0){g_eax=entry==0x2f33d0?3:entry==0x2f4e10?4:entry==0x32a670?5:entry==0x302660?6:7;g_esp+=4;return 1;}
 if(entry!=0x2ec430&&entry!=0x2f4df0&&entry!=0x2ec4a0&&entry!=0x2f4fd0&&entry!=0x2f3a30&&entry!=0x2f3a70&&entry!=0x2f4e40&&entry!=0x2f50a0&&entry!=0x2fe920&&entry!=0x2ec4b0&&entry!=0x2ec5c0&&entry!=0x2ec5e0)return 0;
 if(!span(g_ecx,256))return 0;uint32_t arg=MEM32(g_esp+4);
 if(entry==0x2ec430||entry==0x2f4df0){if(!span(MEM32(g_ecx),40))return 0;set_mass(entry==0x2f4df0);}
 else if(entry==0x2ec4a0||entry==0x2f4fd0){g_eax=arg;if(entry==0x2f4fd0)g_edx=g_eax;MEM32(g_ecx+192)=g_eax;if(entry==0x2f4fd0)MEM32(g_ecx+252)=g_edx;g_esp+=8;}
 else if(entry==0x2ec5c0||entry==0x2ec5e0){if(!span(arg,16))return 0;g_eax=arg;g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_ecx+(entry==0x2ec5c0?208:224),g_xmm0);g_esp+=8;}
 else if(entry==0x2ec4b0){if(!span(arg,16))return 0;g_eax=arg;g_ecx+=16;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2ec4be);sub_00388A00();g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;}
 else{if(!span(arg,48))return 0;if(entry==0x2f3a70)sphere_inertia();else if(entry==0x2f50a0)box_inertia();else export_inertia(entry==0x2f3a30?0:entry==0x2f4e40?1:2);}return 1;
}
