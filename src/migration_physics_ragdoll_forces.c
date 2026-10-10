/* Ragdoll force application and frame ownership with original SSE ordering. */
#include "migration_physics_ragdoll_forces.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_0029E500(void);void sub_002ECAA0(void);void sub_002A51C0(void);void sub_002A55A0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-512||(uint64_t)g_esp+32<=p);}
static unsigned fp_flags(float a,float b){double x=a,y=b;return x!=x||y!=y?0x47:x<y?3:x>y?2:0x42;}
static void apply_force(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-68;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x74);g_ebx=0;PUSH32(g_esp,g_edi);MEM32(g_esp+28)=g_esi;
 if((int32_t)g_eax>0){g_edi=MEM32(frame+8);do{
  g_ecx=MEM32(g_esi+0x70);g_edx=MEM32(g_ecx+g_ebx*4);g_ecx=MEM32(g_edx+0x50);g_eax=MEM32(g_edi);g_ecx+=64;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+52;PUSH32(g_esp,g_edx);g_ecx=g_edi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x2a5c9b);RECOMP_ICALL_SAFE(target,saved);
  g_xmm0=XMM_MEM(g_esp+48);g_xmm0=XMM_MUL(g_xmm0,g_xmm0);g_xmm1=XMM_SHUFFLE(g_xmm0,g_xmm0,0x55);g_xmm2=g_xmm0;g_xmm1.f[0]+=g_xmm0.f[0];g_xmm2=XMM_SHUFFLE(g_xmm2,g_xmm0,0xaa);g_xmm0=g_xmm2;g_xmm0.f[0]+=g_xmm1.f[0];XMM_STORE(g_esp+32,g_xmm0);g_xmm0.f[0]=sqrtf(g_xmm0.f[0]);MEMF(g_esp+32)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esp+32);g_eax=g_esp+16;MEMF(g_eax)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+16));
  if(!(fp_flags(g_xmm0.f[0],MEMF(0x44c174))&0x41)){
   g_ecx=MEM32(g_esi+0x70);g_edx=MEM32(g_ecx+g_ebx*4);g_eax=MEM32(g_edx+0x50);g_edx=MEM32(g_edi);g_esi=g_ecx+g_ebx*4;g_eax+=64;saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+68;PUSH32(g_esp,g_eax);g_ecx=g_edi;target=MEM32(g_edx+4);PUSH32(g_esp,0x2a5d03);RECOMP_ICALL_SAFE(target,saved);
   MEM32(g_esp+20)=g_eax;g_ebp=frame;CALL(0x2a5d0c,sub_0029E500);g_ecx=MEM32(g_eax+8);g_esi=MEM32(g_esi);MEM32(g_esp+24)=g_ecx;g_ecx=g_esi;g_ebp=frame;CALL(0x2a5d1c,sub_002ECAA0);
   g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esi+0x50);g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);target=MEM32(g_edx+108);PUSH32(g_esp,0x2a5d2e);RECOMP_ICALL_SAFE(target,saved);g_esi=MEM32(g_esp+28);
  }
  g_eax=MEM32(g_esi+0x74);++g_ebx;
 }while((int32_t)g_ebx<(int32_t)g_eax);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=8;
}
static void update(void){
 g_eax=MEM32(g_esp+8);g_xmm0=XMM_MEM(g_eax);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;XMM_STORE(g_esi+0x30,g_xmm0);for(unsigned o=16;o<64;o+=16){g_xmm0=XMM_MEM(g_eax+o);XMM_STORE(g_esi+0x30+o,g_xmm0);}g_eax=MEM32(g_esi+0x90);PUSH32(g_esp,g_edi);g_edi=0;
 while((int32_t)g_edi<(int32_t)g_eax){g_eax=MEM32(g_esi+0x8c);g_ecx=MEM32(g_eax+g_edi*4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x2a5def);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+0x8c);g_ecx=MEM32(g_eax+g_edi*4);g_xmm0=XMM_SCALAR(MEMF(g_ecx+4));SET_HI8(g_eax,fp_flags(g_xmm0.f[0],MEMF(0x44c174)));
  if(!RECOMP_PARITY8(((g_eax>>8)&255)&0x44)){if(g_ecx){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);target=MEM32(g_edx);PUSH32(g_esp,0x2a5e14);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esi+0x90)-1;MEM32(g_esi+0x90)=g_eax;g_ecx=g_eax;g_eax=MEM32(g_esi+0x8c);g_ecx=MEM32(g_eax+g_ecx*4);MEM32(g_eax+g_edi*4)=g_ecx;}
  else{g_eax=MEM32(g_esi+0x1c);if(g_eax){PUSH32(g_esp,g_ecx);g_ecx=g_esi;CALL(0x2a5e40,apply_force);}++g_edi;}
  /* The original compares against live count without loading it into EAX. */
  if((int32_t)g_edi>=(int32_t)MEM32(g_esi+0x90))break;
  g_eax=MEM32(g_esi+0x90);
 }
 g_eax=MEM32(g_esi+0x1c);if(g_eax){SET_LO8(g_eax,MEM8(g_esi+0x24));if(!(uint8_t)g_eax){PUSH32(g_esp,g_ebx);g_ecx=g_esi;CALL(0x2a5e5f,sub_002A51C0);}}
 g_eax=MEM32(g_esi+0x1c);int inactive=g_eax==0;SET_LO8(g_edx,!inactive);MEM8(g_esi+0x24)=(uint8_t)g_edx;g_ecx=g_esi;if(inactive){PUSH32(g_esp,g_ebx);CALL(0x2a5e76,sub_002A51C0);}else CALL(0x2a5e81,sub_002A55A0);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=inactive;g_esp+=12;
}
int uc_migration_physics_ragdoll_forces(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_FORCES");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x2a5c60&&entry!=0x2a5da0)||!span(g_esp-512,544)||!safe(g_ecx,0x94)||!span(0x44c174,4))return 0;
 uint32_t p=g_ecx,n=MEM32(p+0x74);if(n>65535||(n&&!safe(MEM32(p+0x70),n*4)))return 0;for(unsigned i=0;i<n;i++){uint32_t b=MEM32(MEM32(p+0x70)+i*4);if(!safe(b,0x54)||!safe(MEM32(b+0x50),0x50)||!safe(MEM32(MEM32(b+0x50)),112))return 0;}
 if(entry==0x2a5c60){uint32_t f=MEM32(g_esp+4);if(!safe(f,8)||!safe(MEM32(f),12))return 0;apply_force();return 1;}
 if(!safe(MEM32(g_esp+8),64))return 0;n=MEM32(p+0x90);if(n>65535||(n&&!safe(MEM32(p+0x8c),n*4)))return 0;for(unsigned i=0;i<n;i++){uint32_t f=MEM32(MEM32(p+0x8c)+i*4);if(!safe(f,8)||!safe(MEM32(f),12))return 0;}update();return 1;
}
#undef CALL
