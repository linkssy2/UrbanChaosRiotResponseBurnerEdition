/* Collision-agent ownership stays in the live guest island streams and body
 * backlinks. Removing an entry repairs both independent ownership structures. */
#include "migration_physics_agents.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E01D0(void);void sub_002440D0(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-1024,1040);}
static int island_valid(uint32_t island){
 if(!safe(island,0x5c))return 0;
 uint32_t count=MEM32(island+0x48),cap=MEM32(island+0x4c)&0x3fffffff,array=MEM32(island+0x44),size=MEM16(island+0x5a);
 if(count>cap||cap>65536||!size||(cap&&!safe(array,cap*4)))return 0;
 for(uint32_t i=0;i<count;i++)if(!safe(MEM32(array+i*4),size))return 0;
 return 1;
}
static uint32_t owner(uint32_t agent,uint32_t a,uint32_t b){
 uint32_t scan=MEM32(a+0x48)<=MEM32(b+0x48)?a:b;
 for(uint32_t i=0;i<MEM32(scan+0x48);i++){
  uint32_t start=MEM32(MEM32(scan+0x44)+i*4);
  if(agent>=start&&agent<start+MEM16(scan+0x5a))return scan;
 }
 return scan==a?b:a;
}
static int links_valid(uint32_t agent){
 if(!safe(agent,32))return 0;
 for(unsigned side=0;side<2;side++){
  uint32_t body=MEM32(agent+20+side*4);if(!safe(body,48))return 0;
  uint32_t count=MEM32(body+40),cap=MEM32(body+44)&0x3fffffff,array=MEM32(body+36),index=MEM16(agent+12+side*2);
  if(!count||count>cap||cap>65536||index>=count||!safe(array,cap*8)||MEM32(array+index*8)!=agent)return 0;
  uint32_t tail=MEM32(array+(count-1)*8);if(!safe(tail,32))return 0;
  unsigned which=MEM32(tail+20)!=body;
  if(MEM32(tail+20+which*4)!=body||MEM16(tail+12+which*2)!=count-1)return 0;
 }
 return MEM32(agent+20)!=MEM32(agent+24);
}
static int store_valid(uint32_t store,uint32_t agent,int linked){
 if(!safe(store,24))return 0;
 uint32_t array=MEM32(store),count=MEM32(store+4),cap=MEM32(store+8)&0x3fffffff;
 uint32_t used=MEM32(store+16),stride=MEM16(store+20),size=MEM16(store+22);
 if(!count||count>cap||cap>65536||stride<32||!size||size%stride||!used||used>size||used%stride||!safe(array,cap*4))return 0;
 int found=0;
 for(uint32_t i=0;i<count;i++){
  uint32_t block=MEM32(array+i*4),bytes=i+1==count?used:size;if(!safe(block,size))return 0;
  if(agent>=block&&agent-block<bytes&&(agent-block)%stride==0)found=1;
 }
 uint32_t last=MEM32(array+(count-1)*4)+used-stride;
 return found&&(!linked||links_valid(agent))&&(last==agent||links_valid(last))&&safe(0x51e6f4,4)&&safe(MEM32(0x51e6f4),4)&&safe(MEM32(MEM32(0x51e6f4)),24);
}
static int destroy_valid(uint32_t agent,uint32_t dispatcher){
 unsigned tag=MEM8(agent);
 return (tag!=2&&tag!=4&&tag!=6)||safe(dispatcher,0x169c+MEM8(agent+1)*0x34);
}
static void select_owner(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=8;g_ecx=MEM32(g_esp+16);g_eax=MEM32(g_ecx+0x48);PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+24);PUSH32(g_esp,g_esi);
 SET_LO8(g_eax,(int32_t)g_eax<=(int32_t)MEM32(g_ebx+0x48));PUSH32(g_esp,g_edi);MEM8(g_esp+16)=(uint8_t)g_eax;if((uint8_t)g_eax)g_ebx=g_ecx;
 g_edi=MEM32(g_ebx+0x48);SET_LO8(g_eax,0);g_edx=0;MEM8(g_esp+12)=0;
 if((int32_t)g_edi>0){
  g_esi=MEM32(g_ebx+0x44);PUSH32(g_esp,frame);frame=MEM32(g_esp+28);int found=0;
  while((int32_t)g_edx<(int32_t)g_edi){
   g_ecx=MEM32(g_esi);if(g_ecx<=frame){g_eax=MEM16(g_ebx+0x5a)+g_ecx;if(frame<g_eax){found=1;break;}}
   ++g_edx;g_esi+=4;
  }
  SET_LO8(g_eax,found?1:MEM8(g_esp+16));POP32(g_esp,frame);
 }
 SET_LO8(g_edx,MEM8(g_esp+16));int found=(uint8_t)g_eax!=0;g_eax=MEM32(g_esp+32);SET_LO8(g_ecx,found);POP32(g_esp,g_edi);SET_LO8(g_edx,(uint8_t)g_edx!=0);SET_LO8(g_ecx,(uint8_t)g_ecx^(uint8_t)g_edx);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
 if(!(uint8_t)g_ecx)g_eax=MEM32(g_esp+16);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void compact(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_ecx=MEM16(g_esi+20);g_eax=MEM32(g_esi+4);g_edx=MEM32(g_esi);g_eax=MEM32(g_edx+g_eax*4-4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+16);g_eax=g_eax-g_ecx+g_edi;g_edi=MEM32(g_esp+16);
 if(g_edi!=g_eax){
  PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x32989a);sub_002440D0();
  g_ecx=MEM16(g_edi+12);g_edx=MEM32(g_edi+20);g_edx=MEM32(g_edx+36);SET_LO16(g_eax,MEM16(g_edi+14));MEM32(g_edx+g_ecx*8)=g_edi;
  g_ecx=MEM32(g_edi+24);g_edx=MEM32(g_ecx+36);g_eax=(uint16_t)g_eax;g_esp+=12;MEM32(g_edx+g_eax*8)=g_edi;
 }
 g_eax=MEM16(g_esi+20);MEM32(g_esi+16)-=g_eax;
 if(!MEM32(g_esi+16)){
  g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi);g_edi=MEM32(g_ecx+g_eax*4-4);g_ecx=g_eax-1;g_eax=MEM32(g_esi+8);MEM32(g_esi+4)=g_ecx;
  if(!(g_eax&0x80000000)){
   ++g_ecx;int shrink=(int32_t)g_ecx<1;
   if(!shrink){g_eax&=0x3fffffff;g_edx=g_ecx*2;shrink=(int32_t)g_edx<(int32_t)g_eax;}
   if(shrink){PUSH32(g_esp,1);g_eax=g_esi+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,4);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3298f9);sub_002E01D0();g_esp+=16;}
  }
  g_eax=MEM16(g_esi+22);g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;
  PUSH32(g_esp,0x24);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x32990f);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM16(g_esi+22);MEM32(g_esi+16)=g_ecx;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static void remove_record(void){
 uint32_t frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+24);g_eax=(unsigned)MEM8(frame)-2;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 unsigned tag=MEM8(frame);if(tag!=2){g_eax-=2;if(tag!=4)g_eax-=2;}
 if(tag==2||tag==6){
  g_edx=MEM8(frame+1);g_eax=MEM32(frame+16);g_edx*=0x34;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+40);g_ecx=frame+32;PUSH32(g_esp,g_ecx);PUSH32(g_esp,frame);uint32_t target=MEM32(g_edx+g_eax+0x1698);PUSH32(g_esp,0x32a37d);RECOMP_ICALL_SAFE(target,saved);g_esp+=12;
 }else if(tag==4){
  g_eax=MEM8(frame+1);g_ecx=MEM32(frame+16);g_eax*=0x34;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+40);g_edx=frame+48;PUSH32(g_esp,g_edx);PUSH32(g_esp,frame);uint32_t target=MEM32(g_eax+g_ecx+0x1698);PUSH32(g_esp,0x32a441);RECOMP_ICALL_SAFE(target,saved);g_esp+=12;
 }
 g_ecx=frame+12;MEM32(g_esp+16)=g_ecx;frame+=20;MEM32(g_esp+20)=2;
 do{
  g_esi=MEM32(frame);g_edi=MEM32(g_esi+40);g_edx=MEM32(g_esp+16);g_ecx=MEM16(g_edx);g_edx=MEM32(g_esi+36);g_eax=g_esi+36;--g_edi;MEM32(g_eax+4)=g_edi;
  g_ebx=MEM32(g_edx+g_edi*8);MEM32(g_edx+g_ecx*8)=g_ebx;g_edi=MEM32(g_edx+g_edi*8+4);MEM32(g_edx+g_ecx*8+4)=g_edi;
  if((int32_t)g_ecx<(int32_t)MEM32(g_esi+40)){g_edx=MEM32(g_eax);g_edi=MEM32(g_edx+g_ecx*8);g_ebx=MEM32(g_edi+20);g_edx=g_ebx!=g_esi;MEM16(g_edi+g_edx*2+12)=(uint16_t)g_ecx;}
  g_ecx=MEM32(g_eax+4);g_edx=g_ecx*2+8;g_ecx=MEM32(g_eax+8)&0x3fffffff;
  if((int32_t)g_edx<=(int32_t)g_ecx){PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x32a3f0);sub_002E01D0();g_esp+=16;}
  g_ecx=MEM32(g_esp+16);g_eax=MEM32(g_esp+20);g_ecx+=2;frame+=4;--g_eax;MEM32(g_esp+16)=g_ecx;MEM32(g_esp+20)=g_eax;
 }while(g_eax);
 g_edx=MEM32(g_esp+32);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x32a41b);compact();g_esp+=8;
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void remove_owner(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);g_ecx=MEM32(g_ebx+20);g_eax=MEM32(g_ecx+16)+g_ecx;g_ecx=MEM32(g_ebx+24);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+16)+g_ecx;g_ecx=MEM32(g_esi+0x54);PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax+0x54);
 if(g_edi==g_ecx){g_eax=g_edi;MEM8(g_edi+0x26)=1;}
 else{
  SET_LO8(g_eax,MEM8(g_eax+0x91));if((uint8_t)g_eax)g_eax=g_ecx;
  else{SET_LO8(g_eax,MEM8(g_esi+0x91));if((uint8_t)g_eax)g_eax=g_edi;
   else{PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2fe1dd);select_owner();SET_LO8(g_ecx,1);MEM8(g_edi+0x26)=(uint8_t)g_ecx;g_edx=MEM32(g_esi+0x54);g_esp+=12;MEM8(g_edx+0x26)=(uint8_t)g_ecx;}
  }
 }
 g_ecx=MEM32(g_eax+0x1c);g_edx=MEM32(g_ecx+0x7c);g_esi=MEM32(g_ebx+16);PUSH32(g_esp,g_edx);g_eax+=0x44;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2fe1ff);remove_record();
 g_eax=MEM32(g_esi);g_esp+=12;POP32(g_esp,g_edi);g_ecx=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_seh_ebp=frame;RECOMP_ITAIL(MEM32(g_eax+24));
}
int uc_migration_physics_agents(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_AGENTS");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(g_esp&3)||g_esp<0x10400||!span(g_esp-1024,1040))return 0;
 if(entry==0x2fdfb0){if(!island_valid(MEM32(g_esp+8))||!island_valid(MEM32(g_esp+12)))return 0;select_owner();return 1;}
 if(entry==0x329870||entry==0x32a340){uint32_t store=MEM32(g_esp+4),agent=MEM32(g_esp+8);if(!store_valid(store,agent,entry==0x32a340)||(entry==0x32a340&&!destroy_valid(agent,MEM32(g_esp+12))))return 0;if(entry==0x329870)compact();else remove_record();return 1;}
 if(entry!=0x2fe190)return 0;
 uint32_t agent=MEM32(g_esp+4);if(!links_valid(agent))return 0;
 uint32_t a=MEM32(agent+20),b=MEM32(agent+24);a+=MEM32(a+16);b+=MEM32(b+16);if(!safe(a,0x94)||!safe(b,0x94))return 0;
 uint32_t ia=MEM32(a+0x54),ib=MEM32(b+0x54);if(!island_valid(ia)||!island_valid(ib))return 0;
 uint32_t chosen=ia==ib?ia:MEM8(a+0x91)?ib:MEM8(b+0x91)?ia:owner(agent,ia,ib);
 if(!store_valid(chosen+0x44,agent,1)||!safe(MEM32(chosen+0x1c),0x80)||!destroy_valid(agent,MEM32(MEM32(chosen+0x1c)+0x7c)))return 0;
 uint32_t manager=MEM32(agent+16);if(!safe(manager,4)||!safe(MEM32(manager),28))return 0;
 remove_owner();return 1;
}
