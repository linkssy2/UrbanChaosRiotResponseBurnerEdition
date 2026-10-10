/* Original ragdoll reference release, array ownership and normal SEH lifetime. */
#include "migration_physics_ragdoll_destroy.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002A5F00(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+32<=p);}
static void release_members(unsigned offset,unsigned ret,int counted){
 g_eax=MEM32(g_esi+offset+4);g_edi=0;
 while((int32_t)g_edi<(int32_t)g_eax){
  g_eax=MEM32(g_esi+offset);g_ecx=MEM32(g_eax+g_edi*4);int destroy=g_ecx!=0;
  if(counted){destroy=0;if(MEM16(g_ecx+4)){MEM16(g_ecx+6)=(uint16_t)(MEM16(g_ecx+6)-1);destroy=MEM16(g_ecx+6)==0;}}
  if(destroy){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esi+offset+4);++g_edi;
 }
}
static void free_array(unsigned offset,unsigned ret){
 g_edx=MEM32(g_esi+offset);g_ecx=MEM32(0x51e6f4);g_edi=MEM32(g_ecx);g_eax&=0x3fffffff;uint32_t saved=g_esp;PUSH32(g_esp,20);g_eax<<=2;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_edi+20);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);
}
static void destroy_ragdoll(void){
 PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a1762);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+8)=g_esi;MEM32(g_esi)=0x48dd60;for(unsigned i=0;i<6;i++)PUSH32(g_esp,0);MEM32(g_esp+44)=5;PUSH32(g_esp,0x2a65cd);sub_002A5F00();
 release_members(124,0x2a65fe,1);release_members(112,0x2a6630,1);release_members(140,0x2a6657,0);
 g_eax=MEM32(g_esi+148);if((int32_t)g_eax>=0)free_array(140,0x2a6689);
 g_ecx=(MEM32(g_esi+148)&0xc0000000)|0x80000000;g_eax=g_ecx;MEM32(g_esi+140)=0;MEM32(g_esi+144)=0;MEM32(g_esi+148)=g_ecx;MEM8(g_esp+20)=4;
 const unsigned offsets[]={124,112,16},returns[]={0x2a6706,0x2a672c,0x2a6752};
 for(unsigned i=0;i<3;i++){g_eax=MEM32(g_esi+offsets[i]+8);MEM8(g_esp+20)=(uint8_t)(3-i);if((int32_t)g_eax>=0)free_array(offsets[i],returns[i]);}
 g_eax=MEM32(g_esi+12);MEM8(g_esp+20)=0;if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;uint32_t saved=g_esp;PUSH32(g_esp,20);g_eax<<=2;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2a6778);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+12);MEM32(g_esi)=0x48dd38;POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_ragdoll_destroy(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_DESTROY");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2a6590||!span(g_esp-256,288)||!safe(g_ecx,156)||!span(0x51e6f4,4))return 0;
 uint32_t p=g_ecx,allocator=MEM32(0x51e6f4);if(!safe(allocator,4)||!safe(MEM32(allocator),24))return 0;const unsigned offsets[]={124,112,140};
 for(unsigned k=0;k<3;k++){uint32_t n=MEM32(p+offsets[k]+4),array=MEM32(p+offsets[k]);if(n>65535||(n&&!safe(array,n*4)))return 0;for(unsigned i=0;i<n;i++){uint32_t item=MEM32(array+i*4);if(k==2&&!item)continue;if(!safe(item,k==2?4:8)||!safe(MEM32(item),4))return 0;}}
 destroy_ragdoll();return 1;
}
