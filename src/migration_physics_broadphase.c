/* Unpartitioned sweep-axis candidate bitmap (324100). This consumes the live
 * endpoint order: XOR endpoints before the selected minimum, then XOR only
 * minima inside its interval. It does not sort, deduplicate or choose policy. */
#include "migration_physics_broadphase.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int broad_span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int broad_apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
int uc_migration_physics_broadphase(void){
 static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE");flag=v&&!strcmp(v,"1");}
 if(!flag||(g_esp&3)||g_esp<0x10100||!broad_span(g_esp-128,152)||!broad_span(g_ecx,0x7c)||MEM32(g_ecx+0x70))return 0;
 uint32_t n=MEM32(g_esp+4),node=MEM32(g_esp+12),bits=MEM32(g_esp+20),axis=MEM32(g_ecx+0x4c);
 /* Original endpoint/node indices are 16 bit, with signed packed comparisons
  * elsewhere in this owner. Keep the established 16383-body admission. */
 if(!n||n>16384||!broad_span(node,16)||!broad_span(axis,n*8))return 0;
 uint32_t size=((n>>7)+1)*16,lo=MEM16(node+8),hi=MEM16(node+10);
 if(!lo||lo>=hi||hi>=n*2-1||(bits&15)||!broad_span(bits,size))return 0;
 if(!broad_apart(bits,size,axis,n*8)||!broad_apart(bits,size,node,16)||!broad_apart(bits,size,g_ecx,0x7c)||!broad_apart(bits,size,g_esp-128,152))return 0;
 if(!broad_apart(axis,n*8,g_esp-128,152)||!broad_apart(node,16,g_esp-128,152)||!broad_apart(g_ecx,0x7c,g_esp-128,152))return 0;
 for(uint32_t i=1;i<hi;i++)if(MEM16(axis+i*4+2)>=n)return 0;
 uint32_t frame=g_ebp;
 PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-20;
 g_eax=MEM32(frame+16);g_edx=MEM16(g_eax+10);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;
 g_ecx=MEM16(g_eax+8);g_eax=MEM32(frame+8);MEM32(g_esp+20)=g_edx;g_edx=MEM32(frame+24);g_eax>>=7;
 PUSH32(g_esp,g_esi);MEM32(g_esp+20)=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_ebx;
 g_xmm0=XMM_ZERO();g_ecx=g_edx;++g_eax;
 do{g_esi=g_ecx;g_ecx+=16;--g_eax;XMM_STORE(g_esi,g_xmm0);}while(g_eax);
 g_eax=MEM32(g_ebx+0x4c);g_ecx=MEM32(g_ebx+0x70);g_eax+=4;
 g_esi=MEM32(g_esp+24);g_ecx=MEM32(g_ebx+0x4c)+g_esi*4;MEM32(g_esp+24)=g_ecx;
 int equal=g_eax==g_ecx;
 while(g_eax<MEM32(g_esp+24)){
  g_ecx=MEM16(g_eax+2);g_esi=g_ecx;g_ecx&=31;g_edi=1u<<g_ecx;g_esi>>=5;
  g_ecx=MEM32(g_edx+g_esi*4);g_eax+=4;g_ecx^=g_edi;MEM32(g_edx+g_esi*4)=g_ecx;
  equal=g_eax==MEM32(g_esp+24);
 }
 g_ecx=MEM32(g_ebx+0x4c);g_esi=MEM32(g_esp+28);g_eax+=4;g_edi=g_ecx+g_esi*4;equal=g_eax==g_edi;
 while(g_eax<g_edi){
  if(!(MEM8(g_eax)&1)){
   g_ecx=MEM16(g_eax+2);g_esi=g_ecx;g_ecx&=31;g_ebx=1;g_esi>>=5;g_ebx<<=g_ecx;MEM32(g_edx+g_esi*4)^=g_ebx;
  }
  g_eax+=4;equal=g_eax==g_edi;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);
 g_recomp_zf=equal;g_esp+=24;return 1;
}
