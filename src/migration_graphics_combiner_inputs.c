#include "migration_graphics_combiner_inputs.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_graphics_combiner_inputs(uint32_t entry){
 static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_COMBINER_INPUTS");flag=v&&!strcmp(v,"1");}if(!flag||entry!=0x3f11e0)return 0;
 uint32_t s=g_esp;if((s&3)||s<0x10100||(uint64_t)s+8>g_xbox_total_ram||g_xbox_total_ram<=0x51c178||MEM8(0x51c178)||(g_edi&15)>5)return 0;
 uint32_t target=MEM32(0x3f1264+4*(g_edi&15));
 if(target!=0x3f11ed&&target!=0x3f1207&&target!=0x3f120e&&target!=0x3f1222&&target!=0x3f1229&&target!=0x3f1233)return 0;
 g_eax=g_edi;PUSH32(g_esp,g_esi);g_eax&=15;
 switch(target){
 case 0x3f11ed:g_eax=g_edx&3;g_esi=MEM32(0x3f51c8+4*g_eax)?g_eax+8:0xffffffff;break;
 case 0x3f1207:g_esi=1;break;
 case 0x3f120e:g_eax=MEM32(0x3f4248)|64;MEM32(0x3f4248)=g_eax;g_esi=5;break;
 case 0x3f1222:g_esi=13;break;
 case 0x3f1229:g_esi=g_edx&16?4:12;break;
 default:g_esi=4;break;
 }
 /* Source selection, alpha replication/complement, mapping and operand slot. */
 g_eax=((g_ecx|g_edx|g_edi)>>1)&16;g_eax|=((g_ecx^g_edi)&16)<<1;g_edx=g_ecx&64;g_eax|=g_edx;g_ecx=(g_ecx>>13)&0x78;g_eax=(g_eax|g_esi)<<(g_ecx&31);
 POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=4;return 1;
}
