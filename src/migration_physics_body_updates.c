/* World-locked broadphase updates own temporary pair arrays through delivery
 * and deferred draining. Preserve original allocator rollback/callback order. */
#include "migration_physics_body_updates.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002ED500(void);void sub_002ED490(void);void sub_002F5F40(void);void sub_003191A0(void);void sub_003190C0(void);void sub_00319050(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-1024,1056);}
static int valid(unsigned entry){
 if(!span(g_esp-1024,1056)||(unsigned)g_fp_top>7)return 0;
 uint32_t world,body;if(entry==0x2f6360){body=g_ecx;if(!safe(body,0x94)||!safe(MEM32(g_esp+4),32))return 0;world=MEM32(body+8);if(!world)return 1;}
 else{world=MEM32(g_esp+4);body=MEM32(g_esp+8);if(!safe(body,0x30))return 0;}
 if(!safe(world,0x29c))return 0;
 if(MEM32(world+0x88)||(entry==0x2f6360&&MEM32(world+0x8c)))return safe(MEM32(world+0x80),16);
 if(entry==0x2f7490&&!MEM32(body+20))return 1;
 if(MEM32(world+(entry==0x2f6360?0x298:0x294))>65536||!safe(0x51e6f4,4))return 0;
 uint32_t allocator=MEM32(0x51e6f4),broad=MEM32(world+0x64);
 if(!safe(allocator,48)||!safe(MEM32(allocator),56)||!safe(broad,4)||!safe(MEM32(broad),28)||!safe(MEM32(world+0x68),256))return 0;
 uint32_t cursor=MEM32(allocator+32),end=MEM32(allocator+44);if(cursor>end||!safe(cursor,end-cursor))return 0;
 if(entry==0x2f7490){uint32_t shape=MEM32(body+20),config=MEM32(world+0x74);if(!safe(shape,4)||!safe(MEM32(shape),16)||!safe(config,12)||!safe(0x44c1f8,4))return 0;}
 return 1;
}
static void release_array(unsigned offset,unsigned rollback_ret,unsigned free_ret){
 g_eax=MEM32(g_esp+offset+12);g_ecx=MEM32(0x51e6f4);g_edx=g_eax;MEM32(g_ecx+32)=g_eax;g_eax=MEM32(g_ecx+40);
 if(!g_eax||g_edx==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,rollback_ret);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esp+offset+8);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax*=8;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+offset+8);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,free_ret);RECOMP_ICALL_SAFE(target,saved);}
}
static void aabb_update(void){
 uint32_t frame=g_seh_ebp;g_esp-=48;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_edi+8);g_ebx=0;
 if(g_ecx){g_eax=MEM32(g_ecx+0x8c)+MEM32(g_ecx+0x88);
  if(g_eax){g_edx=MEM32(g_esp+60);g_eax=g_esp+12;PUSH32(g_esp,g_eax);MEM8(g_esp+16)=17;MEM32(g_esp+20)=g_edi;MEM32(g_esp+24)=g_edx;CALL(0x2f639d,sub_002ED500);}
  else{
   PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);++MEM32(g_ecx+0x88);g_ecx=MEM32(g_edi+8);g_esi=MEM32(g_ecx+0x298);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+48)=g_ebx;MEM32(g_esp+52)=g_ebx;MEM32(g_esp+56)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;frame=g_edx+g_eax;
   if(frame<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=frame;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f63ee);RECOMP_ICALL_SAFE(target,saved);}
   g_ecx=MEM32(0x51e6f4);MEM32(g_esp+48)=g_eax;MEM32(g_esp+60)=g_eax;g_eax=MEM32(g_edi+8);g_esi|=0x80000000;MEM32(g_esp+56)=g_esi;g_esi=MEM32(g_eax+0x298);MEM32(g_esp+32)=g_ebx;MEM32(g_esp+36)=g_ebx;MEM32(g_esp+40)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;frame=g_edx+g_eax;
   if(frame<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=frame;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f6441);RECOMP_ICALL_SAFE(target,saved);}
   g_ecx=MEM32(g_edi+8);MEM32(g_esp+32)=g_eax;MEM32(g_esp+44)=g_eax;g_eax=g_edi+40;MEM32(g_esp+16)=g_eax;g_eax=g_esp+32;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=g_esp+52;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+76);PUSH32(g_esp,1);g_esi|=0x80000000;MEM32(g_esp+52)=g_esi;g_ecx=MEM32(g_ecx+0x64);g_edx=MEM32(g_ecx);PUSH32(g_esp,g_eax);g_eax=g_esp+32;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+24);PUSH32(g_esp,0x2f647b);RECOMP_ICALL_SAFE(target,saved);}
   int changes=MEM32(g_esp+52)!=g_ebx;POP32(g_esp,g_esi);POP32(g_esp,frame);
   if(changes||MEM32(g_esp+28)!=g_ebx){g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+44;PUSH32(g_esp,g_edx);CALL(0x2f6498,sub_003191A0);g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_esp+32);g_edx=MEM32(g_edi+8);g_esp+=8;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edx+0x68);CALL(0x2f64b0,sub_003190C0);g_ecx=MEM32(g_edi+8);g_eax=MEM32(g_ecx+0x78);if(g_eax)g_eax+=8;else g_eax=0;g_edx=MEM32(g_esp+40);g_ecx=MEM32(g_ecx+0x68);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);CALL(0x2f64d4,sub_00319050);g_eax=g_edi+80;CALL(0x2f64dc,sub_002F5F40);}
   g_ecx=MEM32(g_edi+8);--MEM32(g_ecx+0x88);if(!MEM32(g_ecx+0x88)&&MEM32(g_ecx+0x84)!=g_ebx&&MEM8(g_ecx+0x90)==(uint8_t)g_ebx){CALL(0x2f64fc,sub_002ED490);}
   release_array(24,0x2f651c,0x2f653f);release_array(40,0x2f655f,0x2f6582);
  }
 }POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_esp+=48;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void shape_update(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&0xfffffff0)-84;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(frame+8);g_eax=MEM32(g_edi+0x88);
 if(g_eax){g_eax=MEM32(frame+12);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM8(g_esp+28)=3;MEM32(g_esp+32)=g_eax;g_ebp=frame;CALL(0x2f74c1,sub_002ED500);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=0;g_esp+=4;return;}
 g_edx=MEM32(frame+12);g_eax=MEM32(g_edx+20);MEM32(g_edi+0x88)=1;
 if(g_eax){
  g_esi=MEM32(g_edi+0x294);g_ecx=MEM32(0x51e6f4);MEM32(g_esp+48)=0;MEM32(g_esp+52)=0;MEM32(g_esp+56)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;g_ebx=g_edx+g_eax;
  if(g_ebx<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=g_ebx;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f7526);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(0x51e6f4);MEM32(g_esp+48)=g_eax;MEM32(g_esp+60)=g_eax;g_esi|=0x80000000;g_eax=0;MEM32(g_esp+56)=g_esi;g_esi=MEM32(g_edi+0x294);MEM32(g_esp+32)=g_eax;MEM32(g_esp+36)=g_eax;MEM32(g_esp+40)=0x80000000;g_edx=MEM32(g_ecx+32);g_eax=(g_esi*8+16)&0xfffffff0;g_ebx=g_edx+g_eax;
  if(g_ebx<=MEM32(g_ecx+44)){MEM32(g_ecx+32)=g_ebx;g_eax=g_edx;}else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x2f7578);RECOMP_ICALL_SAFE(target,saved);}
  g_edx=g_esp+64;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_edi+0x74);g_esi|=0x80000000;MEM32(g_esp+44)=g_esi;g_esi=MEM32(frame+12);g_ecx=MEM32(g_esi+20);MEM32(g_esp+36)=g_eax;MEM32(g_esp+48)=g_eax;g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=MEMF(g_edx+8);g_fp_stack[g_fp_top]*=MEMF(0x44c1f8);g_edx=MEM32(g_esi+28);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_ecx);MEMF(g_esp)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x2f75ae);RECOMP_ICALL_SAFE(target,saved);}
  g_ecx=MEM32(g_edi+0x64);g_edx=g_esp+32;
  {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=g_esp+52;PUSH32(g_esp,g_edx);PUSH32(g_esp,1);g_edx=g_esp+76;PUSH32(g_esp,g_edx);g_esi+=40;g_edx=g_esp+40;MEM32(g_esp+40)=g_esi;g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x2f75d3);RECOMP_ICALL_SAFE(target,saved);}
  g_eax=MEM32(g_esp+52);g_ecx=MEM32(g_esp+36)+g_eax;
  if((int32_t)g_ecx>0){g_edx=g_esp+32;PUSH32(g_esp,g_edx);g_eax=g_esp+52;PUSH32(g_esp,g_eax);g_ebp=frame;CALL(0x2f75f0,sub_003191A0);g_ecx=MEM32(g_esp+44);g_edx=MEM32(g_esp+40);g_esp+=8;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+0x68);PUSH32(g_esp,g_edx);g_ebp=frame;CALL(0x2f7605,sub_003190C0);g_eax=MEM32(g_edi+0x78);if(g_eax)g_eax+=8;else g_eax=0;g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+56);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_edi+0x68);g_ebp=frame;CALL(0x2f7626,sub_00319050);}
  release_array(32,0x2f7646,0x2f7669);release_array(48,0x2f7689,0x2f76ac);
 }
 --MEM32(g_edi+0x88);if(!MEM32(g_edi+0x88)){g_eax=MEM32(g_edi+0x84);if(g_eax){SET_LO8(g_eax,MEM8(g_edi+0x90));if(!(uint8_t)g_eax){g_ecx=g_edi;g_ebp=frame;CALL(0x2f76cf,sub_002ED490);}}}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=4;
}
int uc_migration_physics_body_updates(unsigned entry){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_UPDATES");enabled=v&&!strcmp(v,"1");}if(!enabled||(entry!=0x2f6360&&entry!=0x2f7490)||!valid(entry))return 0;if(entry==0x2f6360)aabb_update();else shape_update();return 1;}
#undef CALL
