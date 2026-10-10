/* Original body broadphase insertion/removal owns scratch pairs and delivery. */
#include "migration_physics_body_broadphase.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_00319050(void);void sub_003190C0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-1024,1056);}
static int valid(unsigned entry){
 if(!span(g_esp-1024,1056)||(unsigned)g_fp_top>7)return 0;uint32_t w=MEM32(g_esp+4),b=MEM32(g_esp+8);if(!safe(b,0x30)||!safe(w,0x298))return 0;
 if(entry==0x2f76e0&&!MEM32(b+20))return 1;
 if(MEM32(w+0x294)>65536||!span(0x51e6f4,4))return 0;uint32_t a=MEM32(0x51e6f4),broad=MEM32(w+0x64);if(!safe(a,48)||!safe(MEM32(a),56)||!safe(broad,4)||!safe(MEM32(broad),20)||!safe(MEM32(w+0x68),256))return 0;
 uint32_t p=MEM32(a+32),end=MEM32(a+44);if(p>end||!safe(p,end-p))return 0;
 if(entry==0x2f76e0){uint32_t shape=MEM32(b+20);if(!safe(shape,4)||!safe(MEM32(shape),16)||!safe(MEM32(w+0x74),12)||!span(0x44c1f8,4))return 0;}return 1;
}
static void add_body(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0)-52;PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+12);g_ecx=MEM32(g_ebx+20);g_eax=0;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(g_ecx){
  g_edi=MEM32(frame+8);g_esi=MEM32(g_edi+0x294);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+16)=g_eax;MEM32(g_esp+20)=g_eax;MEM32(g_esp+24)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;MEM32(g_esp+12)=g_edx;g_edx+=g_eax;
  if(g_edx<=MEM32(g_ecx+44)){g_eax=MEM32(g_esp+12);MEM32(g_ecx+32)=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f7742);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_ebx+20);g_edx=g_esp+32;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi+0x74);MEM32(g_esp+20)=g_eax;g_esi|=0x80000000;MEM32(g_esp+28)=g_esi;MEM32(g_esp+32)=g_eax;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_edx+8);g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);g_edx=MEM32(g_ebx+28);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x2f7775);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_edi+0x64);g_eax=MEM32(g_ecx);g_edx=g_esp+16;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_ebx+=40;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,0x2f778b);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esp+20);if((int32_t)g_eax>0){g_ecx=MEM32(g_edi+0x78);if(g_ecx)g_ecx+=8;else g_ecx=0;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+0x68);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);g_ebp=frame;CALL(0x2f77b0,sub_00319050);}
  g_eax=MEM32(g_esp+28);g_ecx=MEM32(0x51e6f4);g_edx=g_eax;MEM32(g_ecx+32)=g_eax;g_eax=MEM32(g_ecx+40);
  if(!g_eax||g_edx==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,0x2f77d0);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esp+24);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax*=8;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2f77f3);RECOMP_ICALL_SAFE(target,saved);}
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
static void remove_body(void){
 g_esp-=16;g_ecx=MEM32(0x51e6f4);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx+0x294);MEM32(g_esp+8)=0;MEM32(g_esp+12)=0;MEM32(g_esp+16)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;PUSH32(g_esp,g_edi);g_edi=g_edx+g_eax;
 if(g_edi<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=g_edi;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f7950);RECOMP_ICALL_SAFE(target,saved);}
 MEM32(g_esp+12)=g_eax;MEM32(g_esp+24)=g_eax;g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_eax+20);g_esi|=0x80000000;MEM32(g_esp+20)=g_esi;
 if(g_ecx){
  g_ecx=MEM32(g_ebx+0x64);g_edx=MEM32(g_ecx);g_esi=g_esp+12;uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_eax+=40;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2f797e);RECOMP_ICALL_SAFE(target,saved);
  g_eax=MEM32(g_esp+16);if((int32_t)g_eax>0){g_ecx=MEM32(g_ebx+0x68);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+16);PUSH32(g_esp,g_eax);CALL(0x2f7994,sub_003190C0);}
 }
 g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx+40);g_edx=MEM32(g_esp+24);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(g_ecx+32)=g_edx;POP32(g_esp,g_ebx);
 if(!g_eax||g_edx==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,0x2f79b5);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esp+8);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax*=8;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2f79d8);RECOMP_ICALL_SAFE(target,saved);}
 g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_body_broadphase(unsigned entry){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_BROADPHASE");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x2f76e0&&entry!=0x2f7900)||!valid(entry))return 0;if(entry==0x2f76e0)add_body();else remove_body();return 1;}
#undef CALL
