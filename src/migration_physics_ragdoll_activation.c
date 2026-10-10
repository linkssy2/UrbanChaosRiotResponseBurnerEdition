/* Original ragdoll activation/deactivation order and owned force-object list. */
#include "migration_physics_ragdoll_activation.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);void sub_002ED920(void);void sub_002EF910(void);void sub_002A3190(void);void sub_0021AB20(void);void sub_002A2910(void);void sub_002ECAA0(void);void sub_002EF400(void);void sub_002ED8C0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
#define FRAME_CALL(ret,fn) do{g_ebp=frame;CALL(ret,fn);}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-512||(uint64_t)g_esp+48<=p);}
static int list_valid(uint32_t h){uint32_t n=MEM32(h+4),cap=MEM32(h+8)&0x3fffffff;return n<=cap&&cap<=65535&&(!cap||safe(MEM32(h),cap*4));}
static void append_force(void){
 PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+8);if(g_edi){g_eax=MEM32(g_ecx+0x94);PUSH32(g_esp,g_esi);g_esi=g_ecx+0x8c;g_ecx=MEM32(g_esi+4);g_eax&=0x3fffffff;if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x2a5eba,sub_002E0150);g_esp+=8;}g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);MEM32(g_edx+g_ecx*4)=g_edi;++MEM32(g_esi+4);POP32(g_esp,g_esi);}POP32(g_esp,g_edi);g_esp+=8;
}
static void release_list(void){
 g_edx=g_ecx;g_eax=MEM32(g_edx+8);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax<<=2;PUSH32(g_esp,g_eax);g_eax=MEM32(g_edx);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_esi+20);PUSH32(g_esp,0x2a5ef3);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);}g_esp+=4;
}
static void activate(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x90);PUSH32(g_esp,g_edi);g_ebx=0;g_edi=0;
 while((int32_t)g_edi<(int32_t)g_eax){g_eax=MEM32(g_esi+0x8c);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x2a5f33);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esi+0x90);++g_edi;}
 g_eax=MEM32(g_esi+0x94);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;uint32_t saved=g_esp;PUSH32(g_esp,0x14);g_eax<<=2;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+0x8c);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2a5f65);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=(MEM32(g_esi+0x94)&0xc0000000)|0x80000000;MEM32(g_esi+0x8c)=g_ebx;MEM32(g_esi+0x90)=g_ebx;MEM32(g_esi+0x94)=g_ecx;
 if(MEM32(g_esi+0x20)!=g_ebx){g_eax=MEM32(g_esi+0x80);g_edi=0;MEM32(g_esi+0x20)=g_ebx;MEM32(g_esi+0x1c)=g_ebx;
  while((int32_t)g_edi<(int32_t)g_eax){g_edx=MEM32(g_esi+0x7c);g_eax=MEM32(g_edx+g_edi*4);PUSH32(g_esp,g_eax);g_ecx=g_esp+19;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+0x88);FRAME_CALL(0x2a5fb7,sub_002ED920);g_eax=MEM32(g_esi+0x80);++g_edi;}
  g_edx=MEM32(g_esi+0x74);g_eax=MEM32(g_esi+0x70);g_ecx=MEM32(g_esi+0x88);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);FRAME_CALL(0x2a5fd5,sub_002EF910);g_eax=MEM32(g_esi+0x74);
  if((int32_t)g_eax>0){do{g_ecx=MEM32(g_esi+0x70);g_edi=MEM32(g_ecx+g_ebx*4);PUSH32(g_esp,0);PUSH32(g_esp,0x21);PUSH32(g_esp,g_edi);FRAME_CALL(0x2a5ff0,sub_002A3190);PUSH32(g_esp,0);PUSH32(g_esp,0x2d);PUSH32(g_esp,g_edi);FRAME_CALL(0x2a5ffa,sub_002A3190);g_eax=MEM32(g_esi+0x74);g_esp+=24;++g_ebx;}while((int32_t)g_ebx<(int32_t)g_eax);g_ebx=0;}
 }
 g_eax=MEM32(frame+16);MEM8(g_esi+0x24)=0;if(g_eax!=g_ebx)for(unsigned o=0;o<64;o+=16){g_xmm0=XMM_MEM(g_eax+o);XMM_STORE(g_esi+0x30+o,g_xmm0);}
 g_ebx=MEM32(frame+8);if(g_ebx){g_ecx=MEM32(g_ebx);g_edx=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_edx+36);PUSH32(g_esp,0x2a6043);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_ebx;MEM32(g_esi+0x20)=g_eax;FRAME_CALL(0x2a604d,sub_0021AB20);
  if(g_eax){g_ecx=MEM32(g_esi+0x20);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+32);PUSH32(g_esp,0x2a6059);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+0x20);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x2a6061);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(frame+20);PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);FRAME_CALL(0x2a606f,sub_002A2910);g_eax=MEM32(g_esi+0x74);g_esp+=8;g_edi=0;
  if((int32_t)g_eax>0){do{g_edx=MEM32(g_esi+0x70);g_ebx=MEM32(g_edx+g_edi*4);g_ecx=g_ebx;FRAME_CALL(0x2a608d,sub_002ECAA0);g_ecx=MEM32(g_ebx+0x50);g_eax=MEM32(g_ecx);g_edx=g_esp+16;saved=g_esp;PUSH32(g_esp,g_edx);target=MEM32(g_eax+84);PUSH32(g_esp,0x2a609a);RECOMP_ICALL_SAFE(target,saved);
   g_eax=MEM32(frame+12);g_ecx=MEM32(g_esi+0x70);g_edx=MEM32(g_ecx+g_edi*4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2d);PUSH32(g_esp,g_edx);FRAME_CALL(0x2a60ac,sub_002A3190);g_eax=MEM32(frame+28);g_ecx=MEM32(g_esi+0x70);g_edx=MEM32(g_ecx+g_edi*4);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x21);PUSH32(g_esp,g_edx);FRAME_CALL(0x2a60be,sub_002A3190);g_eax=MEM32(g_esi+0x74);g_esp+=24;++g_edi;
  }while((int32_t)g_edi<(int32_t)g_eax);g_ebx=MEM32(frame+8);}
  g_eax=MEM32(g_esi+0x74);g_ecx=MEM32(g_esi+0x70);PUSH32(g_esp,1);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+0x88);FRAME_CALL(0x2a60e1,sub_002EF400);g_eax=MEM32(g_esi+0x80);g_edi=0;
  while((int32_t)g_edi<(int32_t)g_eax){g_edx=MEM32(g_esi+0x7c);g_eax=MEM32(g_edx+g_edi*4);g_ecx=MEM32(g_esi+0x88);PUSH32(g_esp,g_eax);FRAME_CALL(0x2a6102,sub_002ED8C0);g_eax=MEM32(g_esi+0x80);++g_edi;}
  SET_LO8(g_ecx,MEM8(frame+24));MEM32(g_esi+0x1c)=g_ebx;MEM8(g_esi+0x98)=(uint8_t)g_ecx;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=28;
}
int uc_migration_physics_ragdoll_activation(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_ACTIVATION");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-512,560))return 0;
 if(entry==0x2a5ed0){if(!safe(g_ecx,12)||!list_valid(g_ecx)||!span(0x51e6f4,4))return 0;uint32_t a=MEM32(0x51e6f4);if(!safe(a,4)||!safe(MEM32(a),24))return 0;release_list();return 1;}
 if(entry!=0x2a5e90&&entry!=0x2a5f00)return 0;if(!safe(g_ecx,0x9c)||!list_valid(g_ecx+0x8c))return 0;
 if(entry==0x2a5e90){append_force();return 1;}
 uint32_t p=g_ecx,a=MEM32(0x51e6f4),actor=MEM32(g_esp+4),matrix=MEM32(g_esp+12);if(!safe(a,4)||!safe(MEM32(a),24)||!list_valid(p+0x70)||!list_valid(p+0x7c)||(matrix&&!safe(matrix,64)))return 0;
 if(actor&&(!safe(actor,4)||!safe(MEM32(actor),4)||!safe(MEM32(MEM32(actor)),40)))return 0;
 for(unsigned i=0;i<MEM32(p+0x90);i++){uint32_t q=MEM32(MEM32(p+0x8c)+i*4);if(q&&(!safe(q,4)||!safe(MEM32(q),4)))return 0;}
 if(actor)for(unsigned i=0;i<MEM32(p+0x74);i++){uint32_t b=MEM32(MEM32(p+0x70)+i*4);if(!safe(b,0x54)||!safe(MEM32(b+0x50),4)||!safe(MEM32(MEM32(b+0x50)),88))return 0;}
 activate();return 1;
}
#undef FRAME_CALL
#undef CALL
