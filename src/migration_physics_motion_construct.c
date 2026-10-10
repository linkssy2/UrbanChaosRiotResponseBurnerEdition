/* Original six motion classes and shared motion lifetime initialization. */
#include "migration_physics_motion_construct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_003894C0(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void motion_base(void){uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-12;g_eax=MEM32(frame+12);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(frame+8);PUSH32(g_esp,g_eax);g_xmm0=XMM_ZERO();PUSH32(g_esp,g_ecx);g_ecx=g_esi+16;MEM16(g_esi+6)=1;MEM32(g_esi+8)=0;MEM32(g_esi)=0x498b90;XMM_STORE(g_esi+208,g_xmm0);XMM_STORE(g_esi+224,g_xmm0);g_ebp=frame;PUSH32(g_esp,0x2ec5a0);sub_003894C0();MEM32(g_esi+200)=MEM32(g_esi+204)=0;g_eax=g_esi;POP32(g_esp,g_esi);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=12;}
static void construct_motion(unsigned entry){
 if(entry==0x2ec560){motion_base();return;}g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esp+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_esi;
 unsigned ret=entry==0x2f1430?0x2f1444:entry==0x2f1450?0x2f1464:entry==0x2f5050?0x2f5064:entry==0x2f36f0?0x2f3704:entry==0x2fe7b0?0x2fe7c4:0x2fe8e4;PUSH32(g_esp,ret);if(entry==0x2f36f0)construct_motion(0x2f5050);else if(entry==0x2fe7b0)construct_motion(0x2fe8d0);else motion_base();
 unsigned vt=entry==0x2f1430?0x4990f0:entry==0x2f1450?0x499168:entry==0x2f5050?0x49a8b8:entry==0x2f36f0?0x49a930:entry==0x2fe7b0?0x49add8:0x49ae58;
 if(entry==0x2f5050){g_eax=0x3f800000;MEM32(g_esi)=vt;for(unsigned o=240;o<=252;o+=4)MEM32(g_esi+o)=g_eax;MEM32(g_esi+196)=0xbf800000;}else if(entry==0x2fe8d0){g_eax=0;MEM32(g_esi+240)=MEM32(g_esi+196)=MEM32(g_esi+192)=0;MEM32(g_esi)=vt;}else MEM32(g_esi)=vt;
 g_eax=g_esi;POP32(g_esp,g_esi);if(entry==0x2fe8d0)g_recomp_zf=g_eax==0;g_esp+=12;
}
int uc_migration_physics_motion_construct(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_MOTION_CONSTRUCT");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x2ec560&&entry!=0x2f1430&&entry!=0x2f1450&&entry!=0x2f5050&&entry!=0x2f36f0&&entry!=0x2fe7b0&&entry!=0x2fe8d0)||!span(g_esp-128,144)||!span(g_ecx,(entry==0x2ec560||entry==0x2f1430||entry==0x2f1450)?240:256)||!span(MEM32(g_esp+4),16)||!span(MEM32(g_esp+8),16))return 0;construct_motion(entry);return 1;
}
