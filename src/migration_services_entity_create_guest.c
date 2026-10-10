#include "migration_services_entity_create_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00208150(void),sub_0020A850(void);
static int entity_create_span(uint32_t p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
int uc_migration_services_entity_create(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_ENTITY_CREATE");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x20bdc0||MEM8(0x51c178))return 0;
 uint32_t s=g_esp,p=g_ecx,context=MEM32(0x51c3e0);
 if(!entity_create_span(s-96,100)||!entity_create_span(p,0x7c)||!entity_create_span(context,28))return 0;
 uint32_t placement=MEM32(context+16);if(placement&&!entity_create_span(placement,0x5c))return 0;
 if((uint64_t)p<s+4u&&(uint64_t)p+0x7c>s-96u)return 0;
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-24;
 g_xmm0=XMM_ZERO();g_xmm1=XMM_SCALAR(MEMF(0x44bf94));PUSH32(g_esp,g_esi);g_esi=g_ecx;
 for(unsigned i=0;i<4;i++)MEMF(g_esp+12+i*4)=i?0:g_xmm1.f[0];g_xmm2=XMM_MEM(g_esp+12);
 PUSH32(g_esp,g_edi);MEM32(g_esi)=0x4805f8;MEM32(g_esi+0x5c)=8;g_edi=0;MEM32(g_esi+0x74)=0;MEM32(g_esi+0x60)=0;XMM_STORE(g_esi+16,g_xmm2);
 /* Build each row in the original scratch slot; the final scratch and SIMD values are guest-visible. */
 for(unsigned row=1;row<4;row++){
  for(unsigned i=0;i<4;i++)MEMF(g_esp+16+i*4)=i==row?g_xmm1.f[0]:0;
  if(row<3){g_xmm2=XMM_MEM(g_esp+16);if(row==1)XMM_STORE(g_esi+32,g_xmm2);}else g_xmm0=XMM_MEM(g_esp+16);
 }
 XMM_STORE(g_esi+48,g_xmm2);XMM_STORE(g_esi+64,g_xmm0);
 PUSH32(g_esp,0x20be76);MEM32(g_ecx+0x78)=0;g_esp+=4;
 g_ecx=MEM32(0x51c4dc);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x20be82);sub_00208150();
 MEM32(g_esi+0x54)=g_eax;for(unsigned i=0;i<3;i++)MEM32(g_esi+0x64+i*4)=g_edi;
 g_ecx=MEM32(0x51c3e0);g_eax=MEM32(g_ecx+16);int absent=g_eax==g_edi;
 if(!absent){
  MEM32(g_esi+0x5c)|=0x1000;g_ecx=MEM32(0x51c3e0);g_edx=MEM32(g_ecx+20);MEM32(g_esi+0x50)=g_edx;
  for(unsigned row=0;row<4;row++){g_xmm0=XMM_MEM(g_eax+row*16);XMM_STORE(g_esi+16+row*16,g_xmm0);}
  if(MEM32(g_eax+0x4c)!=g_edi){g_ecx=MEM32(g_eax+0x58);if(g_ecx&1)MEM32(g_esi+0x60)|=2;if(g_ecx&4)MEM32(g_esi+0x60)|=16;MEM32(g_esi+0x5c)|=0x20000;}
  g_eax=MEM32(g_eax+0x44);MEM32(g_esi+0x70)=g_eax;
 }else{
  g_ecx=MEM32(g_ecx+24);MEM32(g_esi+0x70)=g_ecx;g_ecx=g_esi;g_ebp=frame;PUSH32(g_esp,0x20bf09);sub_0020A850();
 }
 g_eax=g_esi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp=frame+8;if(absent)g_recomp_zf=1;return 1;
}
