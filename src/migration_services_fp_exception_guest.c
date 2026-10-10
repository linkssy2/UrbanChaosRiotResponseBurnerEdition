#include "migration_services_fp_exception_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002DAA77(void),sub_002DAA82(void),sub_00086A0A(void);
static int fe_span(unsigned p,unsigned n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void fe_copy_double(unsigned to,unsigned from){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMD(from);MEMD(to)=g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
int uc_migration_services_fp_exception(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FP_EXCEPTION");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||entry!=0x2da2b6||MEM8(0x51c178)||!fe_span(s-256,288)||!fe_span(MEM32(s+4),100)||!fe_span(MEM32(s+8),4)||!fe_span(MEM32(s+20),8)||!fe_span(MEM32(s+24),8))return 0;
 PUSH32(g_esp,g_ebp);unsigned f=g_esp;g_ebp=f;g_ecx=MEM32(f+8);g_eax=0;MEM32(g_ecx+4)=0;g_ecx=MEM32(f+8);MEM32(g_ecx+8)=0;g_ecx=MEM32(f+8);PUSH32(g_esp,g_ebx);g_ebx=0;MEM32(g_ecx+12)=0;SET_LO8(g_ecx,MEM8(f+16));PUSH32(g_esp,g_esi);g_ebx=1;PUSH32(g_esp,g_edi);
 const unsigned flags[]={16,2,1,4,8},codes[]={0xc000008f,0xc0000093,0xc0000091,0xc000008e,0xc0000090};
 for(unsigned i=0;i<5;i++)if(LO8(g_ecx)&flags[i]){g_eax=MEM32(f+8);MEM32(g_eax+4)|=1u<<i;MEM32(f+16)=codes[i];}
 g_esi=MEM32(f+12);g_ecx=MEM32(g_esi);g_eax=MEM32(f+8);g_ecx=~(g_ecx<<4);g_ecx^=MEM32(g_eax+8);PUSH32(g_esp,2);g_ecx&=16;MEM32(g_eax+8)^=g_ecx;
 g_ecx=MEM32(g_esi);g_eax=MEM32(f+8);g_ecx=~(g_ecx<<1);g_ecx^=MEM32(g_eax+8);POP32(g_esp,g_edi);g_ecx&=8;MEM32(g_eax+8)^=g_ecx;
 const unsigned shifts[]={1,3,5},bits[]={4,2,1};for(unsigned i=0;i<3;i++){g_ecx=MEM32(g_esi);g_eax=MEM32(f+8);g_ecx=~(g_ecx>>shifts[i]);g_ecx^=MEM32(g_eax+8);g_ecx&=bits[i];MEM32(g_eax+8)^=g_ecx;}
 g_ebp=f;PUSH32(g_esp,0x2da399);sub_002DAA77();
 const unsigned status[]={1,4,8,16};for(unsigned i=0;i<4;i++)if(LO8(g_eax)&status[i]){g_ecx=MEM32(f+8);MEM32(g_ecx+12)|=16u>>i;}if(LO8(g_eax)&32){g_eax=MEM32(f+8);MEM32(g_eax+12)|=g_ebx;}
 g_eax=MEM32(g_esi)&0xc00;g_ecx=0xc00;
 if(g_eax==0){g_eax=MEM32(f+8);MEM32(g_eax)&=~3u;}
 else if(g_eax==0xc00){g_eax=MEM32(f+8);MEM32(g_eax)|=3;}
 else{unsigned bit=g_eax==0x400?1:2;g_eax=MEM32(f+8);g_ecx=(MEM32(g_eax)&~(bit==1?2u:1u))|bit;MEM32(g_eax)=g_ecx;}
 g_eax=MEM32(g_esi)&0x300;g_ecx=0x300;
 if(g_eax==0x300){g_eax=MEM32(f+8);MEM32(g_eax)&=0xffffffe3;}
 else if(g_eax!=0x100){unsigned value=g_eax;g_eax=MEM32(f+8);g_ecx=(MEM32(g_eax)&(value?0xffffffe7:0xffffffeb))|(value?4:8);MEM32(g_eax)=g_ecx;}
 g_eax=MEM32(f+8);g_ecx=((MEM32(f+20)<<5)^MEM32(g_eax))&0x1ffe0;MEM32(g_eax)^=g_ecx;g_eax=MEM32(f+8);MEM32(g_eax+32)|=g_ebx;g_eax=MEM32(f+8);g_ecx=(MEM32(g_eax+32)&0xffffffe3)|g_edi;MEM32(g_eax+32)=g_ecx;
 g_eax=MEM32(f+24);unsigned value=g_eax;g_eax=MEM32(f+8);fe_copy_double(g_eax+16,value);g_eax=MEM32(f+8);MEM32(g_eax+96)|=g_ebx;g_eax=MEM32(f+8);g_ecx=(MEM32(g_eax+96)&0xffffffe3)|g_edi;g_edi=MEM32(f+28);MEM32(g_eax+96)=g_ecx;g_eax=MEM32(f+8);fe_copy_double(g_eax+80,g_edi);
 g_ebp=f;PUSH32(g_esp,0x2da4a1);sub_002DAA82();g_eax=f+8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0);PUSH32(g_esp,MEM32(f+16));g_ebp=f;PUSH32(g_esp,0x2da4b0);sub_00086A0A();
 g_eax=MEM32(f+8);const unsigned masks[]={1,4,8,16,32};for(unsigned i=0;i<5;i++)if(MEM8(g_eax+8)&(16u>>i))MEM32(g_esi)&=~masks[i];
 g_ecx=MEM32(g_eax)&3;g_edx=0xfffff3ff;
 if(!g_ecx)MEM32(g_esi)&=g_edx;
 else{unsigned rounding=g_ecx;g_ecx=0;if(rounding==3)MEM8(g_esi+1)|=12;else{g_ecx=(MEM32(g_esi)&(rounding==1?0xfffff7ff:0xfffffbff))|(rounding==1?0x400:0x800);MEM32(g_esi)=g_ecx;}}
 g_ecx=(MEM32(g_eax)>>2)&7;
 if(!g_ecx){g_ecx=(MEM32(g_esi)&g_edx)|0x300;MEM32(g_esi)=g_ecx;}
 else if(!--g_ecx){g_ecx=(MEM32(g_esi)&g_edx)|0x200;MEM32(g_esi)=g_ecx;}
 else if(!--g_ecx)MEM32(g_esi)&=g_edx;
 fe_copy_double(g_edi,g_eax+80);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=8;return 1;
}
