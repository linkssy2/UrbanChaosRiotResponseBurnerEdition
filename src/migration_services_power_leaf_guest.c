#include "migration_services_power_leaf_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned power_status(void){return ((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?g_fp_cmp&0x4700:g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);}
int uc_migration_services_power_leaf(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*p=getenv("UCRR_NATIVE_SERVICES_POWER_LEAF");enabled=p&&!strcmp(p,"1");}
 unsigned s=g_esp;if(!enabled||MEM8(0x51c178)||s<0x10080||s>=0x80000000u||(uint64_t)s+16>g_xbox_total_ram)return 0;
 switch(entry){
 case 0x2dafbd:g_edx=(MEM32(s+4)&0x300)|0x7f;MEM16(s+6)=LO16(g_edx);g_fp_control_word=MEM16(s+6);g_recomp_zf=0;g_esp+=4;return 1;
 case 0x2db046:{int same=MEM16(s)==0x27f;if(!same)g_fp_control_word=MEM16(s);POP32(g_esp,g_edx);g_recomp_zf=same;g_esp+=4;return 1;}
 case 0x2dafed:{
  if(g_edx<0x10000||g_edx>=0x80000000u||(uint64_t)g_edx+8>g_xbox_total_ram)return 0;
  g_eax=MEM32(g_edx+4)&0x7ff00000;g_fp_top=(g_fp_top+7)&7;
  if(g_eax!=0x7ff00000){g_fp_stack[g_fp_top]=MEMD(g_edx);g_recomp_zf=0;}
  else{g_eax=MEM32(g_edx+4);g_esp-=10;g_eax|=0x7fff0000;MEM32(g_esp+6)=g_eax;g_eax=MEM32(g_edx+4);g_ecx=MEM32(g_edx);g_eax=(g_eax<<11)|(g_ecx>>21);g_ecx<<=11;MEM32(g_esp+4)=g_eax;MEM32(g_esp)=g_ecx;
   /* Preserve the active runtime's MEMF approximation of retail FLDT; correction needs a separate oracle. */
   g_fp_stack[g_fp_top]=MEMF(g_esp);g_esp+=10;g_eax=MEM32(g_edx+4);g_recomp_zf=1;
  }g_esp+=4;return 1;
 }
 case 0x2dafa8:{
  unsigned top=g_fp_top,dup=(top+7)&7,one=(top+6)&7;double x=g_fp_stack[top];g_fp_top=dup;g_fp_stack[dup]=RECOMP_X87_ROUND(x);g_fp_stack[top]=g_fp_stack[dup]-g_fp_stack[top];
  double rounded=g_fp_stack[dup];g_fp_stack[dup]=-g_fp_stack[top];g_fp_stack[top]=rounded;g_fp_stack[dup]=exp2(g_fp_stack[dup])-1.0;g_fp_top=one;g_fp_stack[one]=1.0;g_fp_stack[dup]=g_fp_stack[dup]+g_fp_stack[one];g_fp_top=dup;g_fp_stack[dup]=ldexp(g_fp_stack[dup],(int)g_fp_stack[top]);g_fp_stack[top]=g_fp_stack[dup];g_fp_top=top;g_esp+=4;return 1;
 }
 case 0x2d7099:{
  unsigned top=g_fp_top,dup=(top+7)&7;g_fp_top=dup;g_fp_stack[dup]=RECOMP_X87_ROUND(g_fp_stack[top]);g_fp_cmp=RECOMP_FCMP(g_fp_stack[dup],g_fp_stack[top]);g_fp_top=top;SET_LO8(g_ecx,0);SET_LO16(g_eax,power_status());
  if(g_fp_cmp){g_fp_top=(top+1)&7;g_recomp_zf=0;}
  else{g_fp_stack[top]*=MEMD(0x5048a8);SET_LO8(g_ecx,1);g_fp_top=dup;g_fp_stack[dup]=RECOMP_X87_ROUND(g_fp_stack[top]);g_fp_cmp=RECOMP_FCMP(g_fp_stack[dup],g_fp_stack[top]);g_fp_top=(top+1)&7;SET_LO16(g_eax,power_status());if(!g_fp_cmp)SET_LO8(g_ecx,2);}
  g_esp+=4;return 1;
 }
 default:return 0;
 }
}
