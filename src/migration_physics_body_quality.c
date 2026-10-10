/* Original collision-quality object lifetime and reciprocal mass export. */
#include "migration_physics_body_quality.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+8<=p);}
static void mass(void){
 double inverse=MEMF(g_ecx+192);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=inverse;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44c174);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=inverse;g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;unsigned status=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);SET_LO16(g_eax,status);unsigned flags=((g_eax>>8)&255)&0x44;
 if(RECOMP_PARITY8(flags))g_fp_stack[g_fp_top]=MEMF(0x44bf94)/g_fp_stack[g_fp_top];else{g_fp_top=(g_fp_top+1)&7;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44c174);}g_recomp_zf=flags==0;g_esp+=4;
}
static void construct_quality(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-16;g_eax=g_ecx;MEM32(g_esp+12)=0x7f7fffff;g_xmm0=XMM_SCALAR(MEMF(g_esp+12));g_xmm0=XMM_SHUFFLE(g_xmm0,g_xmm0,0);XMM_STORE(g_eax+16,g_xmm0);XMM_STORE(g_eax+48,g_xmm0);MEM16(g_eax+6)=1;MEM32(g_eax)=0x49a9b0;const unsigned v[]={0xbf800000,0x3c23d70a,0x3ba3d70a,0x3dcccccd,0x3e4ccccd};for(unsigned i=0;i<5;i++)MEM32(g_eax+80+i*4)=v[i];g_xmm0=XMM_ZERO();XMM_STORE(g_eax+32,g_xmm0);g_ecx=0x3f800000;MEM32(g_eax+44)=g_ecx;XMM_STORE(g_eax+64,g_xmm0);MEM32(g_eax+76)=g_ecx;g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}
static void replace_quality(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;if(g_esi&&MEM16(g_esi+4))++MEM16(g_esi+6);g_ecx=MEM32(g_edi+100);
 if(g_ecx&&MEM16(g_ecx+4)){--MEM16(g_ecx+6);if(!MEM16(g_ecx+6)){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2eca26);RECOMP_ICALL_SAFE(target,saved);}}
 MEM32(g_edi+100)=g_esi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
static void select_quality(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_edi+100);int same=0;
 if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+16);PUSH32(g_esp,0x2f1a74);RECOMP_ICALL_SAFE(target,saved);same=g_eax==g_esi;}
 if(!same){g_eax=g_esi-1;if(!g_eax){PUSH32(g_esp,0x505a64);g_ecx=g_edi;PUSH32(g_esp,0x2f1ad1);replace_quality();}
 else if(!--g_eax){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,40);PUSH32(g_esp,112);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2f1a8f);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_eax;MEM16(g_eax+4)=112;PUSH32(g_esp,0x2f1a9c);construct_quality();g_esi=g_eax;PUSH32(g_esp,g_esi);g_ecx=g_edi;PUSH32(g_esp,0x2f1aa6);replace_quality();if(MEM16(g_esi+4)){--MEM16(g_esi+6);if(!MEM16(g_esi+6)){g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_eax);PUSH32(g_esp,0x2f1ac0);RECOMP_ICALL_SAFE(target,saved);g_recomp_zf=1;}}}}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_physics_body_quality(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_QUALITY");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,144))return 0;
 if(entry==0x2ec470){if(!safe(g_ecx,196)||!span(0x44c174,4)||!span(0x44bf94,4))return 0;mass();return 1;}
 if(entry==0x2f4450){if(!safe(g_ecx,112))return 0;construct_quality();return 1;}
 if((entry!=0x2ec9f0&&entry!=0x2f1a60)||!safe(g_ecx,104))return 0;uint32_t old=MEM32(g_ecx+100);if(old&&(!safe(old,8)||!safe(MEM32(old),20)))return 0;
 if(entry==0x2ec9f0){uint32_t incoming=MEM32(g_esp+4);if(incoming&&!safe(incoming,8))return 0;replace_quality();}
 else{if(!span(0x51e6f4,4)||!safe(0x505a64,8))return 0;uint32_t allocator=MEM32(0x51e6f4);if(!safe(allocator,4)||!safe(MEM32(allocator),20))return 0;select_quality();}return 1;
}
