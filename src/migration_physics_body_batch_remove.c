/* Original batched body removal: deferred operation, temporary arrays and world lifetime. */
#include "migration_physics_body_batch_remove.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F9FB0(void);void sub_003190C0(void);void sub_00320860(void);void sub_002FAAF0(void);void sub_002F8B20(void);void sub_002F6F50(void);void sub_002ED490(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+12<=p);}
static void profile(unsigned tag,unsigned scratch,int low_edx){
 if(g_ecx<MEM32(0x52020c)){MEM32(g_ecx)=tag;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp+scratch)=g_eax;if(low_edx)g_edx=MEM32(g_esp+scratch);else g_eax=MEM32(g_esp+scratch);MEM32(g_ecx+4)=low_edx?g_edx:g_eax;g_ecx+=12;MEM32(0x520208)=g_ecx;}
}
static void release_temporary(unsigned restore,unsigned flags,unsigned pointer,unsigned shift,unsigned first_ret,unsigned free_ret){
 g_eax=MEM32(g_esp+restore);g_ecx=MEM32(0x51e6f4);g_edx=g_eax;MEM32(g_ecx+32)=g_eax;g_eax=MEM32(g_ecx+40);
 if(!g_eax||g_edx==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,first_ret);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esp+flags);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,20);g_eax<<=shift;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+pointer+8);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,free_ret);RECOMP_ICALL_SAFE(target,saved);}
}
static void remove_batch(void){
 uint32_t frame=g_seh_ebp;g_esp-=36;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+52);g_ebx=g_ecx;
 if((int32_t)g_edi>=1){g_eax=MEM32(g_ebx+136);PUSH32(g_esp,frame);frame=0;
  if(g_eax){g_eax=MEM32(g_esp+52);g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+128);MEM8(g_esp+20)=7;MEM32(g_esp+24)=g_eax;MEM16(g_esp+28)=(uint16_t)g_edi;CALL(0x2ef953,sub_002F9FB0);g_eax=MEM32(g_ebx+132);POP32(g_esp,frame);++g_eax;POP32(g_esp,g_edi);MEM32(g_ebx+132)=g_eax;POP32(g_esp,g_ebx);g_esp+=36;g_recomp_zf=g_esp==0;g_esp+=12;return;}
  MEM32(g_ebx+136)=1;g_ecx=MEM32(0x520208);profile(0x499084,12,1);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+32)=MEM32(g_esp+36)=frame;MEM32(g_esp+40)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_edi*4+16)&~15u;PUSH32(g_esp,g_esi);g_esi=g_edx+g_eax;
  if(g_esi<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=g_esi;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2ef9d6);RECOMP_ICALL_SAFE(target,saved);}
  g_esi=MEM32(g_esp+56);g_ecx=g_edi|0x80000000;g_edx=g_esi+g_edi*4;MEM32(g_esp+44)=g_ecx;MEM32(g_esp+36)=MEM32(g_esp+48)=g_eax;g_ecx=g_esi;MEM32(g_esp+16)=g_edx;
  while(g_ecx!=g_edx){g_eax=MEM32(g_ecx);g_edi=MEM32(g_eax+20);g_eax+=20;if(g_edi!=frame){g_esi=MEM32(g_esp+36);g_edi=MEM32(g_esp+40);g_eax+=20;MEM32(g_esi+g_edi*4)=g_eax;g_eax=MEM32(g_esp+40);g_esi=MEM32(g_esp+56);++g_eax;MEM32(g_esp+40)=g_eax;}g_ecx+=4;}
  g_ecx=MEM32(0x520208);profile(0x499074,56,1);g_edi=MEM32(g_ebx+660);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+20)=MEM32(g_esp+24)=frame;MEM32(g_esp+28)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_edi*8+16)&~15u;frame=g_edx+g_eax;
  if(frame<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=frame;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2efa96);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_ebx+100);g_edx=g_esp+20;uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edi|=0x80000000;g_edx=g_esp+40;MEM32(g_esp+24)=g_eax;MEM32(g_esp+32)=g_edi;MEM32(g_esp+36)=g_eax;g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x2efaba);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(0x520208);profile(0x499068,56,0);g_ecx=MEM32(g_esp+24);g_edx=MEM32(g_esp+20);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+104);PUSH32(g_esp,g_edx);CALL(0x2efaf6,sub_003190C0);release_temporary(32,28,20,3,0x2efb16,0x2efb39);
  if((int32_t)MEM32(g_ebx+172)>=4){g_ecx=MEM32(g_esp+60);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+8);PUSH32(g_esp,g_esi);CALL(0x2efb50,sub_00320860);}
  g_ecx=MEM32(0x520208);profile(0x49905c,56,1);g_edi=MEM32(g_esp+16);
  if(g_esi!=g_edi){do{g_eax=MEM32(g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x2efb8b,sub_002FAAF0);g_ecx=MEM32(g_esi);PUSH32(g_esp,g_ecx);CALL(0x2efb93,sub_002F8B20);g_edx=MEM32(g_esi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);CALL(0x2efb9c,sub_002F6F50);g_ecx=MEM32(g_esi);g_esp+=20;
    if(!MEM16(g_ecx+4)){g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+16);PUSH32(g_esp,0x2efbad);RECOMP_ICALL_SAFE(target,saved);}g_ecx=MEM32(g_esi);if(MEM16(g_ecx+4)){--MEM16(g_ecx+6);if(!MEM16(g_ecx+6)){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);target=MEM32(g_edx);PUSH32(g_esp,0x2efbc7);RECOMP_ICALL_SAFE(target,saved);}}g_esi+=4;
   }while(g_esi!=g_edi);g_ecx=MEM32(0x520208);}
  POP32(g_esp,g_esi);profile(0x4988f8,52,0);--MEM32(g_ebx+136);if(!MEM32(g_ebx+136)){g_eax=MEM32(g_ebx+132);if(g_eax){SET_LO8(g_eax,MEM8(g_ebx+144));if(!(uint8_t)g_eax){g_ecx=g_ebx;CALL(0x2efc1c,sub_002ED490);}}}
  release_temporary(44,40,32,2,0x2efc3c,0x2efc5f);POP32(g_esp,frame);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_esp+=36;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_physics_body_batch_remove(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_BATCH_REMOVE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2ef910||!span(g_esp-256,272)||!safe(g_ecx,664))return 0;int32_t count=(int32_t)MEM32(g_esp+8);if(count>65535)return 0;
 if(count>0){uint32_t list=MEM32(g_esp+4);if(!safe(list,(uint32_t)count*4))return 0;for(int32_t i=0;i<count;i++){uint32_t body=MEM32(list+i*4);if(!safe(body,44)||!safe(MEM32(body),20))return 0;}
  if(!MEM32(g_ecx+136)){if(!span(0x51e6f4,4)||!span(0x520208,8))return 0;uint32_t allocator=MEM32(0x51e6f4),broadphase=MEM32(g_ecx+100),cursor=MEM32(0x520208),end=MEM32(0x52020c);if(!safe(allocator,48)||!safe(MEM32(allocator),56)||!safe(broadphase,4)||!safe(MEM32(broadphase),24)||MEM32(g_ecx+660)>65535||(cursor<end&&!safe(cursor,end-cursor+12)))return 0;uint32_t start=MEM32(allocator+32),limit=MEM32(allocator+44);if(start>limit||!safe(start,limit-start))return 0;}
 }remove_batch();return 1;
}
#undef CALL
