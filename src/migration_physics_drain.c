/* Deferred world operations: detach, process in original order, recursively
 * drain callback additions, release retained storage, restore the sole queue. */
#include "migration_physics_drain.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002ED570(void);void sub_002ED710(void);void sub_002ED8C0(void);void sub_002ED920(void);void sub_002ED980(void);void sub_002EDD50(void);void sub_002EEDC0(void);void sub_002EEFF0(void);void sub_002EF400(void);void sub_002EF910(void);void sub_002EFC70(void);void sub_002EFFB0(void);void sub_002F02E0(void);void sub_002F0AD0(void);void sub_002F0E40(void);void sub_002F1880(void);void sub_002F1AE0(void);void sub_002F6360(void);void sub_002F7490(void);void sub_002F7C10(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static const uint32_t targets[23]={0x2fa381,0x2fa39f,0x2fa6e4,0x2fa3be,0x2fa3da,0x2fa407,0x2fa456,0x2fa4a6,0x2fa4d8,0x2fa4f7,0x2fa588,0x2fa5a2,0x2fa600,0x2fa61a,0x2fa634,0x2fa686,0x2fa702,0x2fa747,0x2fa76b,0x2fa78a,0x2fa7a1,0x2fa7d3,0x2fa832};
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-4096,4100);}
static int object(uint32_t p){return safe(p,0x94)&&safe(MEM32(p),20);}
static int valid(void){
 if(!span(g_esp-4096,4100)||!safe(g_ecx,16)||!safe(0x2fa8e0,92)||memcmp((const void*)XBOX_PTR(0x2fa8e0),targets,92)||!safe(0x51e6f4,4))return 0;
 uint32_t n=MEM32(g_ecx+4),cap=MEM32(g_ecx+8)&0x3fffffff,p=MEM32(g_ecx),world=MEM32(g_ecx+12),alloc=MEM32(0x51e6f4);
 if(n>cap||cap>65536||n>1024||!safe(world,0x9c)||!apart(world,0x9c,g_ecx,16)||!safe(alloc,4)||!safe(MEM32(alloc),24)||(cap&&(!safe(p,cap*16)||!apart(p,cap*16,g_ecx,16)||!apart(p,cap*16,world,0x9c))))return 0;
 for(unsigned i=0;i<n;i++){
  uint32_t r=p+i*16,a=MEM32(r+4),b=MEM32(r+8);unsigned op=MEM8(r);
  if(!op||op>23||op==20)continue;
  if(op==6||op==7||op==15||op==16||op==22){unsigned count=MEM16(r+8);if(count>256||(count&&!safe(a,count*4)))return 0;for(unsigned j=0;j<count;j++)if(!object(MEM32(a+j*4)))return 0;if(op==22&&(!safe(MEM32(world+8),4)||!safe(MEM32(MEM32(world+8)),20)))return 0;}
  else{if(!object(a)||!apart(a,0x94,p,cap*16)||!apart(a,0x94,g_ecx,16)||!apart(a,0x94,world,0x9c))return 0;if((op==5||op==12)&&!object(b))return 0;if(op==8&&(!object(MEM32(a+16))||!object(MEM32(a+20))))return 0;if(op==17&&!safe(b,32))return 0;}
 }
 return 1;
}
static void release_ref(int use_edx,unsigned ret){
 if(MEM16(g_ecx+4)&&!--MEM16(g_ecx+6)){
  uint32_t table=MEM32(g_ecx);if(use_edx)g_edx=table;else g_eax=table;
  uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(table);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);
 }
}
static void drain(void){
 uint32_t frame=g_seh_ebp;g_esp-=0x120;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_eax=g_esp+40;frame=0;g_ebx=g_ecx;PUSH32(g_esp,g_esi);MEM32(g_esp+32)=g_eax;MEM32(g_esp+36)=frame;MEM32(g_esp+40)=0x80000010;g_ecx=16;
 do{MEM8(g_eax)=0;g_eax+=16;--g_ecx;}while(g_ecx);
 g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_ebx);g_edx=MEM32(g_ebx+4);MEM32(g_ebx)=g_eax;g_eax=MEM32(g_esp+36);MEM32(g_ebx+4)=g_eax;g_eax=MEM32(g_esp+40);MEM32(g_esp+32)=g_ecx;g_ecx=MEM32(g_ebx+8);MEM32(g_ebx+8)=g_eax;g_eax=MEM32(g_ebx+12);MEM32(g_esp+36)=g_edx;MEM32(g_esp+40)=g_ecx;++MEM32(g_eax+0x98);g_eax=MEM32(g_esp+36);g_esi=0;MEM32(g_esp+16)=0;
 if((int32_t)g_eax>0){PUSH32(g_esp,g_edi);do{
  g_edx=MEM32(g_esp+36);g_esi*=16;g_eax=MEM8(g_esi+g_edx);g_esi+=g_edx;--g_eax;unsigned op=g_eax+1;
  if(g_eax<=22){uint32_t target=MEM32(0x2fa8e0+g_eax*4);for(op=0;op<23&&targets[op]!=target;op++);if(op==23){g_seh_ebp=frame;RECOMP_ITAIL(target);return;}++op;}
  switch(op){
  case 1:g_eax=MEM32(g_esi+4);if(!MEM32(g_eax+8)){g_edx=MEM32(g_esi+8);g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);CALL(0x2fa39a,sub_002ED570);}goto release_eax;
  case 2:g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_ecx){PUSH32(g_esp,g_eax);g_edx=g_esp+22;PUSH32(g_esp,g_edx);CALL(0x2fa3b9,sub_002ED710);}goto release_eax;
  case 3:g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_ecx){PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);CALL(0x2fa6fa,sub_002F7490);g_esp+=8;}goto release_edx;
  case 4:g_ecx=MEM8(g_esi+10);g_edx=MEM8(g_esi+9);g_eax=MEM8(g_esi+8);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);CALL(0x2fa3d5,sub_002F1880);goto release_edx;
  case 5:case 23:{g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,op==5?0x2fa3e6:0x2fa83e);RECOMP_ICALL_SAFE(target,saved);if(op==23)goto release_eax;g_ecx=MEM32(g_esi+4);release_ref(0,0x2fa3ff);g_ecx=MEM32(g_esi+8);release_ref(1,0x2fa7ce);break;}
  case 6:g_eax=MEM8(g_esi+10);g_ecx=MEM16(g_esi+8);g_edx=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_edx);CALL(0x2fa41d,sub_002EF400);g_eax=MEM16(g_esi+8);g_edi=MEM32(g_esi+4);frame=g_edi+g_eax*4;goto array_edx;
  case 7:g_eax=MEM16(g_esi+8);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);CALL(0x2fa467,sub_002EF910);g_edx=MEM16(g_esi+8);g_edi=MEM32(g_esi+4);frame=g_edi+g_edx*4;goto array_eax;
  case 8:g_eax=MEM32(g_esi+4);if(!MEM32(g_eax+8)){g_edx=MEM32(g_eax+16);g_ecx=MEM32(g_ebx+12);if(MEM32(g_edx+8)==g_ecx){g_edx=MEM32(g_eax+20);if(MEM32(g_edx+8)==g_ecx){PUSH32(g_esp,g_eax);CALL(0x2fa4d3,sub_002ED8C0);}}}goto release_eax;
  case 9:g_eax=MEM32(g_esi+4);if(MEM32(g_eax+8)){PUSH32(g_esp,g_eax);g_ecx=g_esp+23;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);CALL(0x2fa4f2,sub_002ED920);}goto release_edx;
  case 10:
   g_eax=MEM32(g_esi+4);if(!MEM32(g_eax+8)){
    g_ecx=g_eax;g_eax=g_esp+24;MEM32(g_esp+24)=frame;MEM32(g_esp+28)=frame;MEM32(g_esp+32)=0x80000000;g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2fa51b);RECOMP_ICALL_SAFE(target,saved);}
    g_edi=MEM32(g_esp+28);g_eax=0;int add=1;if((int32_t)g_edi>(int32_t)frame){g_ecx=MEM32(g_ebx+12);g_edx=MEM32(g_esp+24);do{frame=MEM32(g_edx+g_eax*4);if(MEM32(frame+8)!=g_ecx){add=0;break;}++g_eax;}while((int32_t)g_eax<(int32_t)g_edi);}
    if(add){g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);CALL(0x2fa549,sub_002EFC70);g_edx=MEM32(g_esp+24);}
    g_eax=MEM32(g_esp+32);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edi=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax*=4;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_edi+20);PUSH32(g_esp,0x2fa56c);RECOMP_ICALL_SAFE(target,saved);}
   }g_ecx=MEM32(g_esi+4);release_ref(1,0x2fa7ce);break;
  case 11:g_eax=MEM32(g_esi+4);if(MEM32(g_eax+12)){g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_eax);CALL(0x2fa59d,sub_002F0E40);}goto release_eax;
  case 12:
   g_eax=MEM32(g_esi+4);g_edx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_edx){g_ecx=MEM32(g_esi+8);if(MEM32(g_ecx+8)==g_edx){SET_LO8(g_edx,MEM8(g_eax+0x91));if(!(uint8_t)g_edx){SET_LO8(g_edx,MEM8(g_ecx+0x91));if(!(uint8_t)g_edx){g_edx=MEM32(g_eax+0x54);if(g_edx!=MEM32(g_ecx+0x54)){PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_eax+8);PUSH32(g_esp,g_eax);CALL(0x2fa5dc,sub_002F7C10);g_esp+=12;}}}}}
   g_ecx=MEM32(g_esi+4);release_ref(1,0x2fa5f8);g_ecx=MEM32(g_esi+8);release_ref(0,0x2fa857);break;
  case 13:g_eax=MEM32(g_esi+4);if(!MEM32(g_eax+8)){g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_eax);CALL(0x2fa615,sub_002EDD50);}goto release_edx;
  case 14:g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_ecx){PUSH32(g_esp,g_eax);CALL(0x2fa62f,sub_002ED980);}goto release_eax;
  case 15:g_ecx=MEM16(g_esi+8);g_edx=MEM32(g_esi+4);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_edx);CALL(0x2fa645,sub_002EFFB0);g_eax=MEM16(g_esi+8);g_edi=MEM32(g_esi+4);frame=g_edi+g_eax*4;goto array_edx;
  case 16:g_eax=MEM16(g_esi+8);g_ecx=MEM32(g_esi+4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);CALL(0x2fa697,sub_002F02E0);g_edx=MEM16(g_esi+8);g_edi=MEM32(g_esi+4);frame=g_edi+g_edx*4;goto array_eax;
  case 17:
   g_ecx=MEM32(g_esi+4);g_eax=MEM32(g_ecx+8);if(g_eax==MEM32(g_ebx+12)){g_edx=MEM32(g_esi+8);PUSH32(g_esp,g_edx);CALL(0x2fa716,sub_002F6360);}g_ecx=MEM32(g_esi+4);release_ref(0,0x2fa72f);g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_esi+8);g_edx=MEM32(g_ecx);
   {uint32_t saved=g_esp;PUSH32(g_esp,0x24);PUSH32(g_esp,32);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2fa742);RECOMP_ICALL_SAFE(target,saved);}break;
  case 18:g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_ecx){g_edx=MEM8(g_esi+9);PUSH32(g_esp,g_edx);g_edx=MEM8(g_esi+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);CALL(0x2fa766,sub_002EEFF0);}goto release_eax;
  case 19:g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_ebx+12);if(MEM32(g_eax+8)==g_ecx){g_edx=MEM8(g_esi+8);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);CALL(0x2fa785,sub_002EEDC0);}goto release_eax;
  case 20:g_ecx=MEM8(g_esi+2);g_edx=MEM8(g_esi+1);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_ebx+12);PUSH32(g_esp,g_edx);CALL(0x2fa79c,sub_002F0AD0);break;
  case 21:g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);CALL(0x2fa7aa,sub_002F1AE0);g_esp+=4;goto release_edx;
  case 22:g_eax=MEM32(g_ebx+12);g_ecx=MEM32(g_eax+8);g_edx=MEM32(g_ecx);{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM16(g_esi+8);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+16);PUSH32(g_esp,0x2fa7e8);RECOMP_ICALL_SAFE(target,saved);}g_ecx=MEM16(g_esi+8);g_edi=MEM32(g_esi+4);frame=g_edi+g_ecx*4;goto array_edx;
  default:break;
  }goto next;
 release_eax:g_ecx=MEM32(g_esi+4);release_ref(0,0x2fa857);goto next;
 release_edx:g_ecx=MEM32(g_esi+4);release_ref(1,0x2fa7ce);goto next;
 array_eax:
  while(g_edi<frame){g_ecx=MEM32(g_edi);g_edi+=4;release_ref(0,op==7?0x2fa49d:0x2fa6c2);}
  g_eax=MEM16(g_esi+8);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);PUSH32(g_esp,4);g_eax*=4;{uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2fa6df);RECOMP_ICALL_SAFE(target,saved);}goto next;
 array_edx:
  while(g_edi<frame){g_ecx=MEM32(g_edi);g_edi+=4;release_ref(1,op==6?0x2fa44d:op==15?0x2fa67d:0x2fa813);}
  g_edx=MEM16(g_esi+8);g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);PUSH32(g_esp,4);g_edx*=4;{uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+4);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+20);PUSH32(g_esp,0x2fa830);RECOMP_ICALL_SAFE(target,saved);}
 next:
  g_eax=MEM32(g_ebx+4);frame=0;if(g_eax){g_ecx=g_ebx;CALL(0x2fa867,drain);}g_esi=MEM32(g_esp+20);g_eax=MEM32(g_esp+40);++g_esi;MEM32(g_esp+20)=g_esi;
 }while((int32_t)g_esi<(int32_t)g_eax);POP32(g_esp,g_edi);}
 g_eax=MEM32(g_esp+32);g_ecx=MEM32(g_ebx);g_edx=MEM32(g_ebx+4);MEM32(g_ebx)=g_eax;g_eax=MEM32(g_esp+40);MEM32(g_esp+32)=g_ecx;g_ecx=MEM32(g_ebx+8);MEM32(g_ebx+8)=g_eax;g_eax=MEM32(g_ebx+12);MEM32(g_ebx+4)=frame;MEM32(g_esp+36)=g_edx;MEM32(g_esp+40)=g_ecx;g_edx=MEM32(g_eax+0x98);--g_edx;POP32(g_esp,g_esi);MEM32(g_eax+0x98)=g_edx;g_eax=MEM32(g_esp+36);POP32(g_esp,frame);POP32(g_esp,g_ebx);
 if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,0x14);g_eax*=16;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2fa8d7);RECOMP_ICALL_SAFE(target,saved);}
 g_esp+=0x120;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_drain(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_DRAIN");enabled=v&&!strcmp(v,"1");}if(!enabled||!valid())return 0;drain();return 1;}
#undef CALL
