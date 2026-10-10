#include "migration_interface_seek.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
extern void sub_00080D27(void);
static int seek_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SEEK");v=s&&!strcmp(s,"1");}return v;}
static int seek_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void seek_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void seek_set_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void seek_error(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));seek_call(0x44b27c,0x820f9,saved);g_edx=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);seek_set_error();g_eax=g_edx;g_esp+=8;}
static void seek_position(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp-=72;PUSH32(g_esp,g_ebx);g_ebx=MEM32(frame+16);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(g_ebx){g_edi=MEM32(g_ebx);g_esi=MEM32(frame+12);}else{g_eax=MEM32(frame+12);g_edx=(int32_t)g_eax<0?UINT32_MAX:0;g_esi=g_eax;g_edi=g_edx;}
 g_eax=MEM32(frame+20);uint32_t method=g_eax;int failed=0;
 if(method){
  g_eax=method==1?0:method-2;uint32_t saved=g_esp;PUSH32(g_esp,method==2?34:14);PUSH32(g_esp,method==2?56:8);g_eax=frame-(method==2?72:8);PUSH32(g_esp,g_eax);g_eax=frame-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));seek_call(0x44b3bc,method==2?0x80fb0:0x80fd6,saved);
  failed=(int32_t)g_eax<0;
  if(failed){PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x80fe0);seek_error();g_eax=UINT32_MAX;}
  else{uint64_t low=(uint64_t)MEM32(frame-(method==2?32:8))+g_esi;MEM32(frame-8)=(uint32_t)low;g_eax=MEM32(frame-(method==2?28:4))+g_edi+(uint32_t)(low>>32);}
 }else{MEM32(frame-8)=g_esi;g_eax=g_edi;}
 if(!failed){
  MEM32(frame-4)=g_eax;
  if((int32_t)g_eax<0||(!g_ebx&&(g_eax&0x7fffffff))){PUSH32(g_esp,(int32_t)g_eax<0?131:87);g_ebp=frame;PUSH32(g_esp,0x8101c);seek_set_error();g_eax=UINT32_MAX;}
  else{
   uint32_t saved=g_esp;PUSH32(g_esp,14);PUSH32(g_esp,8);g_eax=frame-8;PUSH32(g_esp,g_eax);g_eax=frame-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));seek_call(0x44b34c,0x81033,saved);
   if((int32_t)g_eax<0){PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,0x81058);seek_error();g_eax=UINT32_MAX;if(g_ebx)MEM32(g_ebx)=g_eax;}
   else{if(g_ebx){g_eax=MEM32(frame-4);MEM32(g_ebx)=g_eax;}if(MEM32(frame-8)==UINT32_MAX){PUSH32(g_esp,0);g_ebp=frame;PUSH32(g_esp,0x8104d);seek_set_error();}g_eax=MEM32(frame-8);}
  }
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+24;
}
int migration_interface_seek(unsigned op){
 if(!seek_enabled()||op>1)return 0;uint32_t s=g_esp;if(s<0x10200||(s&3)||!seek_span(s-512,536)||!seek_span(0x519c50,4))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!seek_span(tls,4)||!seek_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 const uint32_t slots[]={0x44b27c,0x44b3bc,0x44b34c};for(unsigned i=0;i<3;i++){uint32_t target=MEM32(slots[i]);if(target<0xfe000000||target>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(target&3))return 0;}
 if(!op){uint32_t high=MEM32(s+12);if(high&&(!seek_span(high,4)||!((uint64_t)high+4<=s-512||(uint64_t)s+24<=high)))return 0;seek_position();return 1;}
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=0;
 if(MEM32(frame+12)){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;PUSH32(g_esp,0x817ae);seek_position();}
 PUSH32(g_esp,g_esi);g_eax=frame-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,2);g_eax=frame+14;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));g_ebp=frame;PUSH32(g_esp,0x817c1);sub_00080D27();
 g_eax=(g_eax&&MEM32(frame-4)==2)||MEM16(frame+14)==0xfeff;POP32(g_esp,g_esi);g_esp=frame+16;return 1;
}
