/* Original motion-type factory, including automatic inertia classification. */
#include "migration_physics_motion_factory.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F1430(void);void sub_002F1450(void);void sub_002F5050(void);void sub_002F36F0(void);void sub_002FE8D0(void);void sub_002FE7B0(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static void push_fp(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void add_pop(void){unsigned next=(g_fp_top+1)&7;g_fp_stack[next]+=g_fp_stack[g_fp_top];g_fp_top=(int)next;}
static unsigned auto_type(void){
 g_ecx=MEM32(g_edi+20);g_edx=MEM32(g_edi+40);g_eax=MEM32(g_edi);MEM32(g_esp+12)=g_ecx;push_fp(MEMF(g_esp+12));MEM32(g_esp+28)=g_edx;g_fp_stack[g_fp_top]-=MEMF(g_esp+28);MEM32(g_esp+16)=g_eax;g_ecx=MEM32(0x51e6f4);PUSH32(g_esp,43);g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);push_fp(MEMF(g_esp+20));g_fp_stack[g_fp_top]-=MEMF(g_esp+32);g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);add_pop();push_fp(MEMF(g_esp+20));g_fp_stack[g_fp_top]-=MEMF(g_esp+16);g_fp_stack[g_fp_top]=fabs(g_fp_stack[g_fp_top]);MEMF(g_esp+24)=(float)g_fp_stack[g_fp_top];add_pop();push_fp(MEMF(g_esp+32));g_fp_stack[g_fp_top]+=MEMF(g_esp+16);g_fp_stack[g_fp_top]+=MEMF(g_esp+20);g_fp_stack[g_fp_top]*=MEMF(0x44c538);g_fp_cmp=RECOMP_FCMP(g_fp_stack[g_fp_top],g_fp_stack[(g_fp_top+1)&7]);g_fp_top=(g_fp_top+2)&7;unsigned status=((g_fp_top&7)<<11)|(g_fp_cmp>=0x10000?(g_fp_cmp&0x4700):g_fp_cmp==2?0x4500:g_fp_cmp<0?0x100:g_fp_cmp>0?0:0x4000);SET_LO16(g_eax,status);unsigned flags=((g_eax>>8)&255)&0x41;g_eax=MEM32(g_ecx);return flags?4:2;
}
static void create_motion(void){
 g_esp-=12;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);PUSH32(g_esp,g_esi);g_eax=g_ebx-1;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+44);unsigned kind=g_eax<=5?g_ebx:0;uint32_t saved=g_esp;
 if(kind==1){kind=auto_type();saved=g_esp;}else{g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);PUSH32(g_esp,43);if(kind==2||kind==4)saved=g_esp;}
 unsigned bytes=(kind==2||kind==3)?240:256;unsigned allocation_ret=kind==2?0x2f1599:kind==3?0x2f14e0:kind==4?0x2f14b2:kind==5?0x2f150e:kind==6?0x2f15c4:0x2f15ef;PUSH32(g_esp,bytes);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,allocation_ret);RECOMP_ICALL_SAFE(target,saved);
 g_ecx=MEM32(g_esp+36);g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_eax;MEM16(g_eax+4)=(uint16_t)bytes;unsigned constructor_ret=kind==2?0x2f15b0:kind==3?0x2f14f7:kind==4?0x2f14c9:kind==5?0x2f1525:kind==6?0x2f15db:0x2f1606;PUSH32(g_esp,constructor_ret);
 switch(kind){case 2:sub_002F1430();break;case 3:sub_002F1450();break;case 4:sub_002F5050();break;case 5:sub_002F36F0();break;case 6:sub_002FE8D0();break;default:sub_002FE7B0();break;}
 g_esi=g_eax;if(g_ebx!=6){g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_edi);g_ecx=g_esi;target=MEM32(g_eax+48);PUSH32(g_esp,0x2f1615);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esp+48);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edx+64);PUSH32(g_esp,0x2f1621);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esp+40);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edx+32);PUSH32(g_esp,0x2f162d);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+52);g_edx=MEM32(g_esp+56);POP32(g_esp,g_edi);MEM16(g_esi+190)=20;MEM32(g_esi+180)=g_ecx;MEM32(g_esi+184)=g_edx;g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_motion_factory(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_MOTION_FACTORY");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2f1470||!span(g_esp-128,164)||!span(0x51e6f4,4)||!span(0x44c538,4)||!span(0x2f1654,24))return 0;static const unsigned table[]={0x2f152a,0x2f1491,0x2f14ce,0x2f14a0,0x2f14fc,0x2f15b2};for(unsigned i=0;i<6;i++)if(MEM32(0x2f1654+i*4)!=table[i])return 0;uint32_t allocator=MEM32(0x51e6f4);if(!span(allocator,4)||!span(MEM32(allocator),20)||!span(MEM32(g_esp+8),16)||!span(MEM32(g_esp+12),16)||!span(MEM32(g_esp+20),48)||!span(MEM32(g_esp+24),16))return 0;create_motion();return 1;
}
