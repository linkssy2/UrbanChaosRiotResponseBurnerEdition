/* Original motion replacement and ordered swept-state copy. */
#include "migration_physics_motion_replace.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002EC470(void);void sub_002F1470(void);void sub_002FE7B0(void);void sub_002FE8D0(void);void sub_00389070(void);
#define CALL(ret,fn) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+20<=p);}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void pop_fp(void){g_fp_top=(g_fp_top+1)&7;}
#define TOP g_fp_stack[g_fp_top]
static void copy_swept(void){g_eax=g_ecx;g_ecx=MEM32(g_esp+4);for(unsigned i=0;i<160;i+=16){g_xmm0=XMM_MEM(g_ecx+i);XMM_STORE(g_eax+i,g_xmm0);}for(unsigned i=160;i<172;i+=4){g_edx=MEM32(g_ecx+i);MEM32(g_eax+i)=g_edx;}SET_LO16(g_edx,MEM16(g_ecx+172));MEM16(g_eax+172)=(uint16_t)g_edx;SET_LO16(g_ecx,MEM16(g_ecx+174));MEM16(g_eax+174)=(uint16_t)g_ecx;g_esp+=8;}
static void transfer_motion(void){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=g_esi+16;PUSH32(g_esp,g_eax);g_ecx=g_edi+16;PUSH32(g_esp,0x2ec9c4);copy_swept();g_xmm0=XMM_MEM(g_esi+208);XMM_STORE(g_edi+208,g_xmm0);g_xmm0=XMM_MEM(g_esi+224);XMM_STORE(g_edi+224,g_xmm0);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;}
static void destroy_motion(int fixed){
 int release=0;if(!fixed)release=MEM8(g_esp+4)&1;PUSH32(g_esp,g_esi);g_esi=g_ecx;
 if(fixed){PUSH32(g_esp,0x2fe7e8);MEM32(g_ecx)=0x4564ec;g_esp+=4;release=MEM8(g_esp+8)&1;}else MEM32(g_esi)=0x4564ec;
 if(release){g_edx=MEM16(g_esi+4);g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,43);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,fixed?0x2fe802:0x2ec663);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!release;g_esp+=8;
}
static void replace_motion(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-52;SET_LO8(g_eax,MEM8(frame+16));PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);
 if((uint8_t)g_eax){SET_LO8(g_eax,MEM8(frame+20));if(!(uint8_t)g_eax){g_esi=MEM32(g_edi+80);g_eax=MEM32(g_esi+240);MEM32(g_edi+80)=g_eax;SET_LO16(g_ecx,MEM16(g_esi+244));MEM16(g_edi+46)=(uint16_t)g_ecx;g_edx=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;uint32_t target=MEM32(g_edx+116);PUSH32(g_esp,0x2f6d60);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+240)=0;g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_edx);PUSH32(g_esp,0x2f6d72);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_edi+80);g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x2f6d7a);RECOMP_ICALL_SAFE(target,saved);g_ebx=MEM32(frame+12);
  if(g_eax!=g_ebx&&g_ebx!=1){g_esi=MEM32(g_edi+80);g_edx=MEM32(g_esi);g_eax=g_esp+16;saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edx+40);PUSH32(g_esp,0x2f6d9d);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+184);g_edx=MEM32(g_esi+180);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_eax=g_esi+144;PUSH32(g_esp,g_eax);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_ecx=g_esi;CALL(0x2f6dbe,sub_002EC470);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)TOP;pop_fp();g_edx=g_esi+128;PUSH32(g_esp,g_edx);g_eax=g_esi+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x2f6dd3,sub_002F1470);g_edx=MEM32(g_esi);g_esp+=32;g_ebx=g_eax;saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_esi;target=MEM32(g_edx+116);PUSH32(g_esp,0x2f6de0);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+200);MEM32(g_ebx+200)=g_eax;g_ecx=MEM32(g_esi+204);MEM32(g_ebx+204)=g_ecx;MEM32(g_edi+80)=g_ebx;g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;target=MEM32(g_edx);PUSH32(g_esp,0x2f6e03);RECOMP_ICALL_SAFE(target,saved);g_ebx=MEM32(frame+12);}
 }else{g_eax=MEM32(frame+12);g_ecx=MEM32(0x51e6f4);g_ebx=MEM32(g_edi+80);PUSH32(g_esp,43);int fixed=g_eax==7;g_eax=MEM32(g_ecx);PUSH32(g_esp,256);uint32_t saved=g_esp,target=MEM32(g_eax+16);PUSH32(g_esp,fixed?0x2f6e25:0x2f6e73);RECOMP_ICALL_SAFE(target,saved);MEM16(g_eax+4)=256;g_ecx=MEM32(g_edi+80);g_edx=g_ecx+128;g_ecx+=64;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);g_ecx=g_eax;
  if(fixed){CALL(0x2f6e40,sub_002FE7B0);g_esi=g_eax;g_eax=g_ebx+16;g_ecx=g_esi+16;PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x2f6e4e);copy_swept();push_fp(MEMF(0x44c174));push_fp(MEMF(g_esi+108));g_fp_cmp=RECOMP_FCMP(TOP,g_fp_stack[(g_fp_top+1)&7]);pop_fp();pop_fp();SET_LO16(g_eax,((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000));if(RECOMP_PARITY8(((g_eax>>8)&255)&0x44)){g_eax=MEM32(g_edi+8);if(g_eax){g_ecx=MEM32(g_eax+12);MEM32(g_esp+12)=g_ecx;}else{push_fp(MEMF(0x44bf94));TOP/=MEMF(g_esi+108);TOP+=MEMF(g_esi+92);MEMF(g_esp+12)=(float)TOP;pop_fp();}g_edx=MEM32(g_esp+12);g_eax=g_esi+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);CALL(0x2f6ed2,sub_00389070);g_esp+=8;}}
  else{CALL(0x2f6e8e,sub_002FE8D0);g_esi=g_eax;g_eax=MEM32(g_ebx);saved=g_esp;PUSH32(g_esp,g_esi);g_ecx=g_ebx;target=MEM32(g_eax+116);PUSH32(g_esp,0x2f6e98);RECOMP_ICALL_SAFE(target,saved);}
  SET_LO8(g_eax,MEM8(frame+20));if((uint8_t)g_eax){MEM32(g_esi+240)=g_ebx;g_ecx=MEM16(g_edi+46);MEM32(g_esi+244)=g_ecx;MEM32(g_edi+80)=g_esi;}
  else{g_edx=MEM32(g_ebx+240);MEM32(g_esi+240)=g_edx;g_eax=MEM32(g_ebx+244);MEM32(g_esi+244)=g_eax;MEM32(g_edi+80)=g_esi;g_edx=MEM32(g_ebx);saved=g_esp;PUSH32(g_esp,1);g_ecx=g_ebx;target=MEM32(g_edx);PUSH32(g_esp,0x2f6efa);RECOMP_ICALL_SAFE(target,saved);}
  g_edx=MEM32(frame+12);g_eax=g_edx!=7;g_eax++;MEM16(g_edi+46)=(uint16_t)g_eax;g_ebx=MEM32(frame+12);
 }
 g_ecx=MEM32(g_edi+80)+16;SET_LO8(g_edx,g_ebx==7);MEM32(g_edi+28)=g_ecx;MEM8(g_edi+145)=(uint8_t)g_edx;int nondynamic=g_ebx==7||g_ebx==6;SET_LO8(g_eax,nondynamic?1:0);MEM8(g_edi+146)=(uint8_t)g_eax;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);if(!nondynamic)g_recomp_zf=1;g_esp+=4;
}
int uc_migration_physics_motion_replace(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_MOTION_REPLACE");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-256,276))return 0;
 if(entry==0x2ec900){if(!safe(g_ecx,176)||!safe(MEM32(g_esp+4),176))return 0;copy_swept();return 1;}
 if(entry==0x2ec9b0){if(!safe(g_ecx,240)||!safe(MEM32(g_esp+4),240))return 0;transfer_motion();return 1;}
 if(entry==0x2fe910){if(!safe(g_ecx,4))return 0;MEM32(g_ecx)=0x4564ec;g_esp+=4;return 1;}
 if(entry==0x2ec640||entry==0x2fe7e0){if(!safe(g_ecx,8)||!span(0x51e6f4,4)||!span(MEM32(0x51e6f4),4)||!span(MEM32(MEM32(0x51e6f4)),24))return 0;destroy_motion(entry==0x2fe7e0);return 1;}
 if(entry!=0x2f6d20||!span(0x51e6f4,4)||!span(0x44c174,4)||!span(0x44bf94,4))return 0;uint32_t body=MEM32(g_esp+4);if(!safe(body,148))return 0;uint32_t motion=MEM32(body+80),allocator=MEM32(0x51e6f4);if(!safe(motion,256)||!span(MEM32(motion),120)||!safe(allocator,4)||!span(MEM32(allocator),20))return 0;
 if(MEM8(g_esp+12)&&!MEM8(g_esp+16)){uint32_t saved=MEM32(motion+240);if(!safe(saved,256)||!span(MEM32(saved),120))return 0;}if(MEM32(body+8)&&!safe(MEM32(body+8),16))return 0;
 replace_motion();return 1;
}
#undef TOP
#undef CALL
