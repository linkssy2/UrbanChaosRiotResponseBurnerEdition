#include "migration_graphics_shader_create.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00082528(void),sub_003E66F0(void),sub_003E6550(void),sub_003E64E0(void);
static int create_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SHADER_CREATE");flag=v&&!strcmp(v,"1");}return flag;}
static int create_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define CCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
int uc_migration_graphics_shader_create(uint32_t entry){
 if(!create_enabled()||(entry!=0x3e67f0&&entry!=0x3ee8b0))return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!create_span(s-128,148)||!create_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t data=MEM32(s+4),program=MEM32(s+8);
 if(entry==0x3ee8b0){if(!create_span(data,240)||!create_span(program,4))return 0;
  PUSH32(esp,0x24800000);PUSH32(esp,252);CCALL(sub_00082528,0x3ee8bf);
  if(!eax){eax=0x8007000e;g_recomp_zf=1;esp+=12;return 1;}
  PUSH32(esp,esi);esi=MEM32(esp+8);PUSH32(esp,edi);edi=eax+12;MEM32(eax+8)=edi;MEM32(eax)=1;MEM32(eax+4)=1;ecx=60;memcpy((void*)XBOX_PTR(edi),(const void*)XBOX_PTR(esi),ecx*4);esi+=ecx*4;edi+=ecx*4;ecx=0;ecx=MEM32(esp+16);POP32(esp,edi);MEM32(ecx)=eax;eax=0;POP32(esp,esi);g_recomp_zf=1;esp+=12;return 1;
 }
 if((data&&!create_span(data,4))||!create_span(MEM32(s+12),4)||(program&&(!create_span(program,4)||MEM16(program+2)>1024||!create_span(program,4+(uint64_t)MEM16(program+2)*16))))return 0;
 uint32_t frame=g_seh_ebp;PUSH32(esp,ecx);PUSH32(esp,ebx);PUSH32(esp,frame);frame=MEM32(esp+20);PUSH32(esp,esi);
 if(frame){eax=MEM16(frame);ecx=MEM16(frame+2);MEM32(esp+12)=eax;MEM32(esp+24)=ecx;}
 esi=0;if(frame){eax=MEM32(esp+24)<<4;edx=(eax+127)>>7;esi=eax+edx*4;}ecx=MEM32(esp+20);eax=0;if(ecx){PUSH32(esp,eax);PUSH32(esp,ecx);CCALL(sub_003E66F0,0x3e6833);eax<<=2;}
 ebx=eax+esi;PUSH32(esp,0x24800000);eax=ebx+0x118;PUSH32(esp,eax);CCALL(sub_00082528,0x3e684a);esi=eax;
 if(!esi){POP32(esp,esi);POP32(esp,frame);eax=0x8007000e;POP32(esp,ebx);POP32(esp,ecx);g_recomp_zf=1;esp+=20;return 1;}
 PUSH32(esp,edi);eax=0;ecx=70;edi=esi;for(unsigned i=0;i<ecx;i++)MEM32(edi+i*4)=eax;edi+=ecx*4;ecx=0;ecx=MEM32(esp+36);MEM32(esi+4)=ecx;MEM32(esi)=1;eax=esi+28;ecx=16;do{MEM32(eax)=2;eax+=16;ecx--;}while(ecx);
 edi=MEM32(esp+24);if(edi){edx=frame==0;PUSH32(esp,edx);PUSH32(esp,edi);PUSH32(esp,esi);CCALL(sub_003E6550,0x3e68a3);}
 eax=0;if(frame){edi=MEM32(esi+4);ecx=MEM32(esp+16);edi|=16;MEM32(esi+4)=edi;eax=edi;if(ecx!=0x2078){eax|=1;MEM32(esi+4)=eax;}if(ecx==0x7378)MEM32(esi+4)|=8;eax=MEM32(esp+28);MEM32(esi+8)=eax;eax<<=2;PUSH32(esp,eax);frame+=4;PUSH32(esp,frame);edi=esi+0x114;PUSH32(esp,edi);CCALL(sub_003E64E0,0x3e68ed);eax=(uint32_t)((int32_t)(eax-edi)>>2);edi=MEM32(esp+24);}
 if(edi){ecx=esi+eax*4+0x114;PUSH32(esp,ecx);PUSH32(esp,edi);CCALL(sub_003E66F0,0x3e6908);}
 edx=MEM32(esp+32);ebx>>=2;eax=ebx;MEM32(esi+12)=eax;MEM32(esi+eax*4+0x114)=0;esi|=1;POP32(esp,edi);MEM32(edx)=esi;POP32(esp,esi);POP32(esp,frame);eax=0;POP32(esp,ebx);POP32(esp,ecx);g_recomp_zf=1;esp+=20;return 1;
}
#undef CCALL
