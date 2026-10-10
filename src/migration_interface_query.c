#include "migration_interface_query.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
static int query_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static unsigned query_compare(void){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(0x44c174);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;SET_HI8(g_eax,g_fp_cmp==2?0x47:g_fp_cmp<0?3:g_fp_cmp>0?2:0x42);return HI8(g_eax)&0x44;}
static void query_input(unsigned mode,unsigned ret,int edx_table){g_ecx=MEM32(0x51c4dc);if(edx_table)g_edx=MEM32(g_ecx);else g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,mode);PUSH32(g_esp,g_esi);PUSH32(g_esp,0);uint32_t target=MEM32((edx_table?g_edx:g_eax)+8);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
int migration_interface_query(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_QUERY");enabled=s&&!strcmp(s,"1");}if(!enabled||op)return 0;uint32_t s=g_esp;if(s<0x10100||(s&3)||!query_span(s-256,264)||!query_span(0x51d2bc,16)||!query_span(0x51c4dc,4))return 0;uint32_t owner=MEM32(0x51c4dc);if(!query_span(owner,4)||!query_span(MEM32(owner),12)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t action=MEM32(s+4),index=action-22,target=0x27461b;if(index<=11){if(!query_span(0x274710+index*4,4))return 0;target=MEM32(0x274710+index*4);const unsigned known[]={0x274607,0x27461b,0x274654,0x27466d,0x274686,0x274696,0x27469e,0x2746a6};unsigned i=0;for(;i<8;i++)if(target==known[i])break;if(i==8)return 0;}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=action;g_eax=index;SET_LO8(g_ebx,0);
 if(target==0x274696||target==0x27469e){SET_LO8(g_eax,MEM8(target==0x274696?0x51d2c8:0x51d2bd));goto finish;}
 if(target==0x274607||target==0x274686){SET_LO8(g_eax,MEM8(0x51d2bc));if(LO8(g_eax)){SET_LO8(g_eax,MEM8(0x51d2be));if(target==0x274607?LO8(g_eax)!=0:LO8(g_eax)==0){SET_LO8(g_ebx,1);goto result;}}}
 if(target==0x274654||target==0x27466d){SET_LO8(g_eax,MEM8(0x51d2c9));if(LO8(g_eax)){SET_LO8(g_eax,MEM8(0x51d2c0));if(target==0x274654?LO8(g_eax)==0:LO8(g_eax)!=0){SET_LO8(g_ebx,1);if(target==0x27466d)g_recomp_zf=0;goto result;}}}
 if(target==0x2746a6){query_input(1,0x2746b6,0);unsigned bits=query_compare();if(!RECOMP_PARITY8(bits)){SET_LO8(g_eax,0);g_recomp_zf=1;}else{SET_LO8(g_eax,1);g_recomp_zf=bits==0;}goto finish;}
 SET_LO8(g_ebx,0);query_input(1,0x27462b,1);unsigned bits=query_compare();if(RECOMP_PARITY8(bits)){g_xmm0=XMM_SCALAR(MEMF(0x44c5f0));MEMF(0x51d2c4)=g_xmm0.f[0];SET_LO8(g_eax,1);g_recomp_zf=bits==0;goto finish;}
 g_xmm0=XMM_ZERO();double a=g_xmm0.f[0],b=MEMF(0x51d2c4);if(a!=a||b!=b||a<b)goto result;
 query_input(0,0x2746e7,0);bits=query_compare();if(!RECOMP_PARITY8(bits))goto result;g_xmm0=XMM_SCALAR(MEMF(0x44c538));MEMF(0x51d2c4)=g_xmm0.f[0];SET_LO8(g_eax,1);g_recomp_zf=bits==0;goto finish;
result:SET_LO8(g_eax,LO8(g_ebx));
finish:POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;return 1;
}
