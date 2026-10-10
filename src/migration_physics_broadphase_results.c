/* Original overlap result owner: scratch bitmap, packed tests and ordered pairs. */
#include "migration_physics_broadphase_results.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_00324100(void);void sub_002E0150(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-512||(uint64_t)g_esp+32<=p);}
int uc_migration_physics_broadphase_results(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_RESULTS");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x3251e0||!span(g_esp-512,544)||!safe(g_ecx,0x7c)||!span(0x51e6f4,4))return 0;
 uint32_t n=MEM32(g_ecx+0x44),id=MEM32(g_esp+4),out=MEM32(g_esp+8),alloc=MEM32(0x51e6f4);if(!n||n>16384||!safe(id,4)||MEM32(id)>=n||!safe(MEM32(g_ecx+0x40),n*16)||!safe(MEM32(g_ecx+0x4c),n*8)||!safe(out,12)||!safe(alloc,48)||!safe(MEM32(alloc),56))return 0;
 uint32_t count=MEM32(out+4),cap=MEM32(out+8)&0x3fffffff,pool=MEM32(alloc+32),end=MEM32(alloc+44);if(cap>65535||count>cap||(cap&&!safe(MEM32(out),cap*8))||pool>end||!safe(pool,end-pool))return 0;
 uint32_t node=MEM32(g_ecx+0x40)+MEM32(id)*16;if(MEM16(node+8)>=n*2)return 0;
 uint32_t frame=g_seh_ebp;g_esp-=20;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ebx=MEM32(g_esi+0x44);g_ecx=MEM32(0x51e6f4);frame=MEM32(g_ecx+44);g_eax=(uint32_t)((int32_t)g_ebx>>5)*4+48;PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+32);g_eax&=~15u;g_edx=g_edi+g_eax;
 if(g_edx<=frame){MEM32(g_ecx+32)=g_edx;MEM32(g_esp+24)=g_edi;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x32521d);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esp+24)=g_eax;g_edi=g_eax;}
 g_eax=MEM32(g_esp+40);g_eax=MEM32(g_eax);g_ecx=MEM32(g_esi+0x40);g_edx=MEM32(g_esi+0x4c);frame=g_eax*16+g_ecx;g_ecx=MEM16(frame+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);g_eax=MEM16(g_edx+g_ecx*4);PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);g_ecx=g_esi;MEM32(g_esp+40)=g_edi;MEM32(g_esp+48)=frame;CALL(0x325252,sub_00324100);
 g_ecx=MEM32(g_esi+0x40);g_edx=(uint32_t)((int32_t)MEM32(g_esi+0x44)>>5);MEM32(g_esp+16)=g_ecx;g_ecx=g_edi+g_edx*4+4;MEM32(g_esp+32)=g_ecx;
 if(g_edi<g_ecx){g_edi=MEM32(g_esp+44);do{
  g_eax=MEM32(g_esp+20);g_ebx=MEM32(g_eax);g_esi=MEM32(g_esp+16);
  if(g_ebx){do{
   if(!(uint8_t)g_ebx){g_esi+=128;g_ebx>>=8;}
   else{if(g_ebx&1){g_edx=MEM32(g_esi);g_ecx=MEM32(frame+4);g_eax=MEM32(frame);g_ecx-=g_edx;g_edx=MEM32(g_esi+4)-g_eax;g_ecx|=g_edx;
     if(!(g_ecx&0x80008000)){g_eax=MEM32(g_esi+12);MEM32(g_esp+44)=g_eax;if(!(g_eax&1)){g_ecx=MEM32(g_edi+8)&0x3fffffff;g_edx=MEM32(g_edi+4);if(g_edx==g_ecx){PUSH32(g_esp,8);PUSH32(g_esp,g_edi);CALL(0x3252d1,sub_002E0150);g_eax=MEM32(g_esp+52);g_esp+=8;}g_edx=MEM32(g_edi+4);g_ecx=MEM32(g_edi);frame=MEM32(g_esp+40);MEM32(g_ecx+g_edx*8)=frame;frame=MEM32(g_esp+28);MEM32(g_ecx+g_edx*8+4)=g_eax;++MEM32(g_edi+4);}}
    }g_esi+=16;g_ebx>>=1;}
  }while(g_ebx);g_ecx=MEM32(g_esp+32);}
  g_eax=MEM32(g_esp+20);g_ebx=MEM32(g_esp+16);g_eax+=4;g_ebx+=512;MEM32(g_esp+16)=g_ebx;MEM32(g_esp+20)=g_eax;
 }while(g_eax<g_ecx);g_edi=MEM32(g_esp+24);}
 g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx+40);MEM32(g_ecx+32)=g_edi;if(!g_eax||g_edi==g_eax){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_edx+52);PUSH32(g_esp,0x32533b);RECOMP_ICALL_SAFE(target,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
#undef CALL
