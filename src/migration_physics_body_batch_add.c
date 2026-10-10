/* Original batched body insertion and dynamic/fixed island ownership. */
#include "migration_physics_body_batch_add.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F9FB0(void);void sub_002FD0E0(void);void sub_002E00E0(void);void sub_002E0150(void);void sub_003889E0(void);void sub_002FCB70(void);void sub_00319050(void);void sub_002FAA60(void);void sub_002F8850(void);void sub_002ED490(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+16<=p);}
static void profile(unsigned tag,unsigned scratch,int low_edx){
 if(g_ecx<MEM32(0x52020c)){MEM32(g_ecx)=tag;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp+scratch)=g_eax;if(low_edx)g_edx=MEM32(g_esp+scratch);else g_eax=MEM32(g_esp+scratch);MEM32(g_ecx+4)=low_edx?g_edx:g_eax;g_ecx+=12;MEM32(0x520208)=g_ecx;}
}
static void release_temporary(unsigned restore,unsigned flags,unsigned pointer,unsigned shift,unsigned first_ret,unsigned free_ret){
 g_eax=MEM32(g_esp+restore);g_ecx=MEM32(0x51e6f4);g_edx=g_eax;MEM32(g_ecx+32)=g_eax;g_eax=MEM32(g_ecx+40);if(!g_eax||g_edx==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,first_ret);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esp+flags);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,20);g_eax<<=shift;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+pointer+8);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,free_ret);RECOMP_ICALL_SAFE(target,saved);}
}
static void add_batch(void){
 uint32_t frame=g_seh_ebp;g_esp-=72;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+84);PUSH32(g_esp,frame);frame=0;PUSH32(g_esp,g_edi);g_edi=g_ecx;
 if(g_ebx){if(MEM32(g_edi+136)){SET_LO8(g_ecx,MEM8(g_esp+96));g_eax=MEM32(g_esp+88);g_edx=g_esp+20;MEM8(g_esp+30)=(uint8_t)g_ecx;g_ecx=MEM32(g_edi+128);PUSH32(g_esp,g_edx);MEM8(g_esp+24)=6;MEM32(g_esp+28)=g_eax;MEM16(g_esp+32)=(uint16_t)g_ebx;CALL(0x2ef448,sub_002F9FB0);++MEM32(g_edi+132);POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=72;g_recomp_zf=g_esp==0;g_esp+=16;return;}
  g_ecx=MEM32(0x520208);profile(0x499048,16,0);PUSH32(g_esp,g_esi);++MEM32(g_edi+136);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+40)=MEM32(g_esp+44)=frame;MEM32(g_esp+48)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_ebx*4+16)&~15u;g_esi=g_edx+g_eax;
  if(g_esi<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=g_esi;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2ef4c0);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(0x51e6f4);MEM32(g_esp+40)=MEM32(g_esp+52)=g_eax;g_eax=g_ebx<<5;g_esi=g_ebx|0x80000000;MEM32(g_esp+56)=MEM32(g_esp+60)=frame;g_eax+=16;MEM32(g_esp+48)=g_esi;MEM32(g_esp+64)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax&=~15u;frame=g_edx+g_eax;
  if(frame<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=frame;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2ef50d);RECOMP_ICALL_SAFE(target,saved);}
  MEM32(g_esp+56)=MEM32(g_esp+68)=g_eax;SET_LO8(g_eax,MEM8(g_edi+156));MEM32(g_esp+64)=g_esi;MEM8(g_esp+19)=0;
  if((uint8_t)g_eax){g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,47);PUSH32(g_esp,108);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x2ef537);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_edi);g_ecx=g_eax;MEM16(g_eax+4)=108;CALL(0x2ef545,sub_002FD0E0);frame=g_eax;if(MEM32(g_esp+100)==1){SET_LO16(g_ecx,MEM16(g_edi+60));MEM16(frame+32)=(uint16_t)g_ecx;MEM8(frame+38)=MEM8(frame+41)=MEM8(frame+40)=1;}else{SET_LO16(g_edx,MEM16(g_edi+72));MEM16(frame+32)=(uint16_t)g_edx;MEM8(frame+38)=1;MEM8(frame+41)=MEM8(frame+40)=0;}}
  else{g_eax=MEM32(g_edi+56);frame=MEM32(g_eax);g_eax=MEM32(frame+56);g_ecx=MEM32(frame+60);g_edx=frame+52;g_eax+=g_ebx;g_ecx&=0x3fffffff;if((int32_t)g_ecx<(int32_t)g_eax){g_ecx*=2;if((int32_t)g_eax<(int32_t)g_ecx)g_eax=g_ecx;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);CALL(0x2ef5a5,sub_002E00E0);g_esp+=12;}}
  g_ecx=MEM32(g_edi+116);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_ecx+8);g_eax=MEM32(g_esp+96);g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);g_ebx=0;MEMF(g_esp+20)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;
  while((int32_t)g_ebx<(int32_t)g_eax){g_edx=MEM32(g_esp+92);g_esi=MEM32(g_edx+g_ebx*4);if(MEM16(g_esi+4))++MEM16(g_esi+6);g_eax=MEM32(g_esi+28);if(!g_eax){g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+12);PUSH32(g_esp,0x2ef5f0);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+28)=g_eax;}
   g_ecx=MEM32(g_esi+80)+16;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0);CALL(0x2ef603,sub_003889E0);SET_LO8(g_eax,MEM8(g_esi+145));g_esp+=12;MEM32(g_esi+8)=g_edi;PUSH32(g_esp,g_esi);if((uint8_t)g_eax)g_ecx=MEM32(g_edi+48);else{MEM8(g_esp+23)=1;g_ecx=frame;}CALL(0x2ef625,sub_002FCB70);g_eax=MEM32(g_esi+20);
   if(g_eax){g_ecx=MEM32(g_esp+60);g_eax=(g_ecx<<5)+MEM32(g_esp+56);++g_ecx;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+24);MEM32(g_esp+64)=g_ecx;g_ecx=MEM32(g_esi+20);g_edx=MEM32(g_ecx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+28);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2ef650);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esp+44);g_edx=MEM32(g_esp+40);g_esi+=40;MEM32(g_edx+g_ecx*4)=g_esi;++MEM32(g_esp+44);}g_eax=MEM32(g_esp+96);++g_ebx;
  }
  SET_LO8(g_eax,MEM8(g_edi+156));if((uint8_t)g_eax){SET_LO8(g_eax,MEM8(g_esp+19));if((uint8_t)g_eax){g_esi=g_edi+56;if(MEM32(g_esp+100)!=1)g_esi=g_edi+68;g_eax=MEM32(g_esi+8)&0x3fffffff;g_ecx=MEM32(g_esi+4);if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x2ef6a5,sub_002E0150);g_esp+=8;}g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);MEM32(g_edx+g_ecx*4)=frame;++MEM32(g_esi+4);}else{g_eax=MEM32(frame);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=frame;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2ef6be);RECOMP_ICALL_SAFE(target,saved);}}
  g_esi=MEM32(g_edi+660);g_ecx=MEM32(0x51e6f4);g_eax=0;MEM32(g_esp+72)=MEM32(g_esp+76)=g_eax;MEM32(g_esp+80)=0x80000000;g_edx=MEM32(g_ecx+32);g_ebx=MEM32(g_ecx+44);g_eax=(g_esi*8+16)&~15u;frame=g_edx+g_eax;
  if(frame<=g_ebx){MEM32(g_ecx+32)=frame;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2ef700);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(0x520208);MEM32(g_esp+72)=MEM32(g_esp+84)=g_eax;g_eax=MEM32(0x52020c);g_esi|=0x80000000;MEM32(g_esp+80)=g_esi;profile(0x498d1c,100,0);g_ecx=MEM32(g_edi+100);g_edx=MEM32(g_ecx);g_eax=g_esp+72;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+60;PUSH32(g_esp,g_eax);g_eax=g_esp+48;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2ef754);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(0x520208);profile(0x499038,100,1);g_eax=MEM32(g_edi+120);if(g_eax)g_eax+=8;else g_eax=0;g_ecx=MEM32(g_esp+72);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+80);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+104);CALL(0x2ef79f,sub_00319050);g_ecx=MEM32(0x520208);profile(0x49902c,100,1);g_ebx=MEM32(g_esp+96);g_esi=0;
  while((int32_t)g_esi<(int32_t)g_ebx){g_eax=MEM32(g_esp+92);frame=MEM32(g_eax+g_esi*4);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);CALL(0x2ef7e1,sub_002FAA60);PUSH32(g_esp,frame);CALL(0x2ef7e7,sub_002F8850);g_esp+=12;++g_esi;}
  --MEM32(g_edi+136);POP32(g_esp,g_esi);if(!MEM32(g_edi+136)){g_eax=MEM32(g_edi+132);if(g_eax){SET_LO8(g_eax,MEM8(g_edi+144));if(!(uint8_t)g_eax){g_ecx=g_edi;CALL(0x2ef813,sub_002ED490);}}}g_ecx=MEM32(0x520208);profile(0x4988f8,92,1);release_temporary(80,76,68,3,0x2ef85d,0x2ef880);release_temporary(64,60,52,5,0x2ef8a0,0x2ef8c3);release_temporary(48,44,36,2,0x2ef8e3,0x2ef906);
 }
 POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=72;g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_physics_body_batch_add(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_BATCH_ADD");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2ef400||!span(g_esp-256,276)||!safe(g_ecx,664))return 0;uint32_t count=MEM32(g_esp+8);if(count>65535)return 0;
 if(count){uint32_t list=MEM32(g_esp+4);if(!safe(list,count*4))return 0;for(uint32_t i=0;i<count;i++){uint32_t body=MEM32(list+i*4);if(!safe(body,148)||!safe(MEM32(body),16)||!safe(MEM32(body+80),32))return 0;uint32_t shape=MEM32(body+20);if(shape&&(!safe(shape,4)||!safe(MEM32(shape),16)))return 0;}
  if(!MEM32(g_ecx+136)){if(!span(0x51e6f4,4)||!span(0x520208,8)||!span(0x44c1f8,4))return 0;uint32_t allocator=MEM32(0x51e6f4),bp=MEM32(g_ecx+100),cursor=MEM32(0x520208),end=MEM32(0x52020c);if(!safe(allocator,48)||!safe(MEM32(allocator),56)||!safe(bp,4)||!safe(MEM32(bp),16)||!safe(MEM32(g_ecx+116),12)||MEM32(g_ecx+660)>65535||(cursor<end&&!safe(cursor,end-cursor+12)))return 0;uint32_t start=MEM32(allocator+32),limit=MEM32(allocator+44);if(start>limit||!safe(start,limit-start))return 0;if(!MEM8(g_ecx+156)){uint32_t islands=MEM32(g_ecx+56);if(!safe(islands,4)||!safe(MEM32(islands),64))return 0;}}
 }add_batch();return 1;
}
#undef CALL
