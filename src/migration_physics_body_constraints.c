/* Original reverse-order extraction of a body's master and slave constraints. */
#include "migration_physics_body_constraints.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002FEEF0(void);void sub_002ED490(void);void sub_002E0150(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+12<=p);}
static int array(uint32_t p,unsigned stride){if(!safe(p,12))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff;return n<=cap&&cap<=65535&&(!cap||safe(MEM32(p),cap*stride));}
static void extract(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+8);g_eax=MEM32(g_ecx+108)-1;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+24);PUSH32(g_esp,g_edi);g_edi=MEM32(g_ecx+8);
 if((int32_t)g_eax>=0){frame=g_eax*28;++g_eax;MEM32(g_esp+16)=g_eax;do{g_eax=MEM32(g_ecx+104);g_ebx=MEM32(g_eax+frame);if(MEM16(g_ebx+4))++MEM16(g_ebx+6);g_eax=MEM32(g_edi+136)+1;PUSH32(g_esp,g_ebx);MEM32(g_edi+136)=g_eax;CALL(0x2f69d8,sub_002FEEF0);g_eax=MEM32(g_edi+136);g_esp+=4;--g_eax;MEM32(g_edi+136)=g_eax;if(!g_eax){g_eax=MEM32(g_edi+132);if(g_eax){SET_LO8(g_eax,MEM8(g_edi+144));if(!(uint8_t)g_eax){g_ecx=g_edi;CALL(0x2f6a05,sub_002ED490);}}}g_ecx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);if(g_eax==g_ecx){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x2f6a1d,sub_002E0150);g_esp+=8;}g_edx=MEM32(g_esi+4);g_eax=MEM32(g_esi);g_ecx=MEM32(g_esp+24);MEM32(g_eax+g_edx*4)=g_ebx;g_edx=MEM32(g_esi+4);g_eax=MEM32(g_esp+16);++g_edx;frame-=28;--g_eax;MEM32(g_esi+4)=g_edx;MEM32(g_esp+16)=g_eax;}while(g_eax);}
 frame=MEM32(g_ecx+120)-1;if((int32_t)frame>=0){for(;;){g_ecx=MEM32(g_ecx+116);g_ebx=MEM32(g_ecx+frame*4);if(MEM16(g_ebx+4))++MEM16(g_ebx+6);g_eax=MEM32(g_edi+136)+1;PUSH32(g_esp,g_ebx);MEM32(g_edi+136)=g_eax;CALL(0x2f6a79,sub_002FEEF0);g_eax=MEM32(g_edi+136);g_esp+=4;--g_eax;MEM32(g_edi+136)=g_eax;if(!g_eax){g_eax=MEM32(g_edi+132);if(g_eax){SET_LO8(g_eax,MEM8(g_edi+144));if(!(uint8_t)g_eax){g_ecx=g_edi;CALL(0x2f6aa6,sub_002ED490);}}}g_edx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);if(g_eax==g_edx){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x2f6abe,sub_002E0150);g_esp+=8;}g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);MEM32(g_ecx+g_eax*4)=g_ebx;g_ecx=MEM32(g_esi+4)+1;--frame;MEM32(g_esi+4)=g_ecx;if((int32_t)frame<0)break;g_ecx=MEM32(g_esp+24);}}
 g_recomp_zf=frame==0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=4;
}
int uc_migration_physics_body_constraints(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_CONSTRAINTS");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2f6990||!span(g_esp-128,140))return 0;uint32_t body=MEM32(g_esp+4),out=MEM32(g_esp+8);if(!safe(body,128)||!safe(MEM32(body+8),145)||!array(body+104,28)||!array(body+116,4)||!array(out,4))return 0;
 for(unsigned k=0;k<2;k++){uint32_t h=body+104+k*12,p=MEM32(h),n=MEM32(h+4);for(uint32_t i=0;i<n;i++)if(!safe(MEM32(p+i*(k?4:28)),8))return 0;}
 extract();return 1;
}
#undef CALL
