#include "migration_services_ftol_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_services_ftol(unsigned entry){
 static int enabled=-1;
 if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_FTOL");enabled=p&&!strcmp(p,"1");}
 if(!enabled||entry!=0x2d6780)return 0;
 uint32_t s=g_esp;
 if(s<0x10040||s>=0x80000000u||(uint64_t)s+4>g_xbox_total_ram)return 0;
 uint32_t f=s-4,scratch=(f-32)&~15u;unsigned top=g_fp_top&7u,duplicate=(top+7)&7u;
 MEM32(f)=g_ebp;g_ebp=f;
 double input=g_fp_stack[top];g_fp_stack[duplicate]=input;
 MEMF(scratch+24)=(float)input;
 SMEM64(scratch+16)=(int64_t)llrint(input);
 g_fp_stack[duplicate]=(double)SMEM64(scratch+16);
 g_edx=MEM32(scratch+24);g_eax=MEM32(scratch+16);
 if(!g_eax)g_edx=MEM32(scratch+20);
 if(!g_eax&&!(g_edx&0x7fffffffu)){
  MEMF(scratch+24)=(float)g_fp_stack[duplicate];MEMF(scratch+24)=(float)g_fp_stack[top];
 }else{
  g_fp_stack[top]-=g_fp_stack[duplicate];MEMF(scratch)=(float)g_fp_stack[top];
  uint32_t bits=MEM32(scratch);int negative=(int32_t)g_edx<0;
  if(negative)bits^=0x80000000u;
  uint64_t correction=(uint64_t)bits+0x7fffffffu;g_ecx=(uint32_t)correction;
  uint64_t value=(uint64_t)SMEM64(scratch+16);
  value=negative?value+(correction>>32):value-(correction>>32);
  g_eax=(uint32_t)value;g_edx=(uint32_t)(value>>32);
 }
 g_fp_top=(top+1)&7u;g_esp=s+4;return 1;
}
