/* Original joint ownership: stable priority records, packed runtime bytes and
 * reciprocal body links. Guest storage remains the sole source of truth. */
#include "migration_physics_install.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E00E0(void);void sub_002E0150(void);void sub_002E0DA0(void);void sub_002F7C10(void);
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-1024,1048);}
static int array_valid(uint32_t header,uint32_t stride){
 if(!safe(header,12))return 0;uint32_t n=MEM32(header+4),cap=MEM32(header+8)&0x3fffffff;
 return n<=cap&&cap<=65536&&(!cap||safe(MEM32(header),cap*stride));
}
static int body_valid(uint32_t body){
 if(!safe(body,0x94)||!array_valid(body+0x68,28)||!array_valid(body+0x74,4)||!array_valid(body+0x80,1))return 0;
 uint32_t island=MEM32(body+0x54);if(!safe(island,4)||!safe(MEM32(island),16))return 0;
 uint32_t base=MEM32(body+0x80),end=base+MEM32(body+0x84),used=base;
 for(uint32_t i=0;i<MEM32(body+0x6c);i++){
  uint32_t r=MEM32(body+0x68)+i*28,p=MEM32(r+24),bytes=MEM16(r+20);
  if(!safe(MEM32(r),40)||MEM32(MEM32(r)+36)!=r)return 0;
  if(p){if(p!=used||bytes>end-used)return 0;used+=bytes;}
 }
 return used==end;
}
static void construct(void){
 g_eax=g_ecx;g_ecx=MEM32(g_esp+12);g_edx=0;MEM32(g_eax+12)=g_ecx;SET_LO8(g_ecx,MEM8(g_esp+16));
 MEM16(g_eax+6)=1;MEM32(g_eax)=0x4990bc;MEM32(g_eax+8)=0;MEM8(g_eax+24)=(uint8_t)g_ecx;MEM8(g_eax+25)=0;g_ecx=MEM32(g_eax+12);
 MEM32(g_eax+28)=MEM32(g_eax+32)=MEM32(g_eax+36)=0;if(MEM16(g_ecx+4))++MEM16(g_ecx+6);
 g_ecx=MEM32(g_esp+4);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);MEM32(g_eax+20)=g_esi;MEM32(g_eax+16)=g_ecx;
 int retain=MEM16(g_ecx+4)!=0;POP32(g_esp,g_esi);if(retain)++MEM16(g_ecx+6);g_ecx=MEM32(g_eax+20);if(MEM16(g_ecx+4))++MEM16(g_ecx+6);g_esp+=20;
}
static void priority(void){SET_LO8(g_eax,MEM8(g_esp+4));MEM8(g_ecx+24)=(uint8_t)g_eax;g_ecx=MEM32(g_ecx+36);if(g_ecx)MEM8(g_ecx+16)=(uint8_t)g_eax;g_recomp_zf=g_ecx==0;g_esp+=8;}
static void insert_records(void){
 uint32_t frame=g_seh_ebp;g_esp-=12;g_eax=MEM32(g_esp+20);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=g_ecx;PUSH32(g_esp,g_esi);
 g_esi=MEM32(g_eax+4);g_eax=MEM32(frame+4);g_ecx=g_eax+g_esi;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+32);g_eax-=g_edi;MEM32(g_esp+32)=g_eax;
 g_eax=MEM32(frame+8)&0x3fffffff;MEM32(g_esp+24)=g_esi;MEM32(g_esp+16)=g_ecx;
 if((int32_t)g_eax<(int32_t)g_ecx){g_eax*=2;if((int32_t)g_ecx>=(int32_t)g_eax)g_eax=g_ecx;PUSH32(g_esp,28);PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);PUSH32(g_esp,0x2ff065);sub_002E00E0();g_esp+=12;}
 g_ecx=MEM32(frame);g_ebx=g_edi;g_eax=(g_esi+g_edi)*28;g_ebx*=28;g_edx=g_ecx+g_ebx;g_eax+=g_ecx;g_ecx=MEM32(g_esp+32)-1;MEM32(g_esp+20)=g_edx;
 if((int32_t)g_ecx>=0){
  g_esi=MEM32(g_esp+20);g_edx=g_ecx*28+g_eax;g_esi-=g_eax;MEM32(g_esp+32)=g_esi;g_eax=g_ecx+1;
  do{g_esi=MEM32(g_esp+32)+g_edx;g_edi=g_edx;g_ecx=7;g_edx-=28;--g_eax;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),28);g_esi+=28;g_edi+=28;g_ecx=0;}while(g_eax);
  g_esi=MEM32(g_esp+24);
 }
 g_ecx=MEM32(frame);g_eax=MEM32(g_esp+36);g_edi=MEM32(g_eax);g_edx=g_esi-1;g_ecx+=g_ebx;
 if((int32_t)g_edx>=0){
  g_eax=g_edx*28+g_ecx;g_ebx=g_edi-g_ecx;++g_edx;
  do{g_esi=g_ebx+g_eax;g_edi=g_eax;g_ecx=7;g_eax-=28;--g_edx;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),28);g_esi+=28;g_edi+=28;g_ecx=0;}while(g_edx);
  g_ecx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(frame+4)=g_ecx;
 }else{g_edx=MEM32(g_esp+16);POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM32(frame+4)=g_edx;}
 POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void install(void){
 uint32_t frame=g_seh_ebp;g_esp-=0x3c;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+0x48);PUSH32(g_esp,g_esi);if(MEM16(g_ebx+4))++MEM16(g_ebx+6);
 g_eax=MEM32(g_ebx+16);SET_LO8(g_ecx,MEM8(g_ebx+24));g_esi=MEM32(g_ebx+20);PUSH32(g_esp,frame);frame=MEM32(g_ebx+12);
 MEM32(g_esp+44)=g_ebx;MEM32(g_esp+48)=g_eax;MEM32(g_esp+52)=g_esi;MEM32(g_esp+56)=frame;MEM8(g_esp+60)=(uint8_t)g_ecx;SET_LO8(g_ecx,MEM8(g_eax+0x91));PUSH32(g_esp,g_edi);
 if(!(uint8_t)g_ecx){
  SET_LO8(g_ecx,MEM8(g_esi+0x91));if(!(uint8_t)g_ecx){g_edx=MEM32(g_eax+0x54);if(g_edx!=MEM32(g_esi+0x54)){
   PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);g_eax=MEM32(g_eax+8);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2ff16d);sub_002F7C10();frame=MEM32(g_esp+72);g_esi=MEM32(g_esp+68);g_eax=MEM32(g_esp+64);g_esp+=12;
  }}
 }
 SET_LO8(g_edx,MEM8(g_esi+0x91));SET_LO8(g_ecx,MEM8(g_eax+0x91));g_edx=(uint8_t)g_edx!=0;g_edi=(uint32_t)(int32_t)(int8_t)g_ecx;
 if(g_edi==g_edx){g_eax=MEM32(g_eax+0x6c);g_esi=MEM32(g_esi+0x6c);g_ecx=g_esi+g_eax;SET_LO8(g_eax,(int32_t)g_ecx<8?(int32_t)g_eax>=(int32_t)g_esi:(int32_t)g_eax<=(int32_t)g_esi);}
 else SET_LO8(g_eax,(uint8_t)g_ecx!=0);
 g_edx=(uint8_t)g_eax;MEM8(g_esp+65)=(uint8_t)g_eax;g_edi=MEM32(g_esp+g_edx*4+52);g_eax=MEM32(g_edi+0x54);MEM32(g_ebx+8)=g_eax;g_edx=MEM32(frame);g_eax=g_esp+32;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=frame;MEM32(g_esp+28)=g_edi;uint32_t target=MEM32(g_edx+32);PUSH32(g_esp,0x2ff1da);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_ebx+8);g_edx=MEM32(g_ecx);g_eax=g_esp+32;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2ff1e8);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_edi+0x6c);g_edi+=0x68;frame=0;
 if((int32_t)g_eax>0){g_esi=MEM32(g_edi);SET_LO8(g_ecx,MEM8(g_esp+64));g_esi+=16;while(MEM8(g_esi)<=(uint8_t)g_ecx){++frame;g_esi+=28;if((int32_t)frame>=(int32_t)g_eax)break;}}
 g_ecx=MEM32(g_edi+8)&0x3fffffff;g_edx=(int32_t)g_eax>=(int32_t)g_ecx;g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_eax=g_esp+52;PUSH32(g_esp,frame);g_ecx=g_edi;MEM32(g_esp+40)=g_eax;
 g_edx=(g_edx-1)&frame;g_esi=g_edx;MEM32(g_esp+44)=1;MEM32(g_esp+48)=0x80000001;PUSH32(g_esp,0x2ff246);insert_records();
 g_ecx=g_esi;
 if((int32_t)g_esi<(int32_t)MEM32(g_edi+4)){g_esi*=28;do{g_eax=MEM32(g_edi);g_edx=MEM32(g_eax+g_esi);g_eax+=g_esi;MEM32(g_edx+36)=g_eax;g_eax=MEM32(g_edi+4);++g_ecx;g_esi+=28;}while((int32_t)g_ecx<(int32_t)g_eax);}
 g_esi=MEM32(g_ebx+36);g_ecx=MEM32(g_esi+12);g_eax=MEM32(g_ecx);g_edx=g_esp+32;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM8(g_ebx+25);MEM32(g_esp+24)=g_esi;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x2ff27f);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esp+32);MEM16(g_esi+20)=(uint16_t)g_eax;
 if(g_eax){
  g_ebx=MEM32(g_esp+24);g_ecx=MEM32(g_ebx+0x80);g_esi=MEM32(g_ebx+0x84);g_edx=MEM32(g_ebx+0x88);g_ebx+=0x80;MEM32(g_esp+24)=g_ecx;g_ecx=g_esi+g_eax;g_edx&=0x3fffffff;
  if((int32_t)g_edx<(int32_t)g_ecx){PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x2ff2c5);sub_002E00E0();g_eax=MEM32(g_esp+44);g_esp+=12;}
  g_esi+=g_eax;g_eax=MEM32(g_esp+24);MEM32(g_ebx+4)=g_esi;g_esi=MEM32(g_ebx);g_edx=g_esi-g_eax;g_eax=0;MEM32(g_esp+28)=g_edx;MEM32(g_esp+24)=0;
  if((int32_t)frame>0){MEM32(g_esp+16)=0;MEM32(g_esp+24)=frame;do{
   g_ecx=MEM32(g_esp+16);g_eax=MEM32(g_edi)+g_ecx;g_ecx=MEM32(g_eax+24);
   if(g_ecx){g_esi=MEM16(g_eax+20);g_ecx+=g_edx;MEM32(g_eax+24)=g_ecx;g_esi+=g_ecx;}
   g_ecx=MEM32(g_esp+16)+28;--frame;MEM32(g_esp+16)=g_ecx;
  }while(frame);}
  frame=MEM32(g_esp+20);g_eax=MEM16(frame+20);g_ecx=MEM32(g_ebx+4);g_edx=MEM32(g_ebx);g_ecx=g_ecx-g_eax-g_esi+g_edx;PUSH32(g_esp,g_ecx);g_eax+=g_esi;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2ff336);sub_002E0DA0();
  g_edx=MEM16(frame+20);g_ecx=MEM32(g_esp+36);g_ebx=MEM32(g_esp+40);g_esp+=12;MEM32(frame+24)=g_esi;g_eax=MEM32(g_edi+4);++g_ecx;g_ebx+=g_edx;
  if((int32_t)g_ecx<(int32_t)g_eax){g_esi=g_ecx*28;do{g_eax=MEM32(g_edi);g_edx=MEM32(g_eax+g_esi+24);g_eax+=g_esi;if(g_edx)g_edx+=g_ebx;MEM32(g_eax+24)=g_edx;g_eax=MEM32(g_edi+4);++g_ecx;g_esi+=28;}while((int32_t)g_ecx<(int32_t)g_eax);}
  g_ebx=MEM32(g_esp+84);g_esi=MEM32(g_esp+20);
 }else MEM32(g_esi+24)=0;
 g_edx=MEM16(g_esi+20);g_ecx=MEM32(g_esi+12);g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esi+24);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_eax+24);PUSH32(g_esp,0x2ff391);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM8(g_esi+17);g_ecx=2-g_eax;g_eax=MEM32(g_esi+g_ecx*4);SET_LO16(g_edx,MEM16(g_eax+0x78));MEM16(g_esi+18)=(uint16_t)g_edx;g_ecx=MEM32(g_eax+0x78);g_esi=g_eax+0x74;g_eax=MEM32(g_esi+8)&0x3fffffff;
 int grow=g_ecx==g_eax;POP32(g_esp,g_edi);POP32(g_esp,frame);
 if(grow){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2ff3c3);sub_002E0150();g_esp+=8;}
 g_ecx=MEM32(g_esi+4);g_edx=MEM32(g_esi);MEM32(g_edx+g_ecx*4)=g_ebx;++MEM32(g_esi+4);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=0x3c;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_install(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_INSTALL");enabled=v&&!strcmp(v,"1");}
 if(!enabled||(g_esp&3)||g_esp<0x10400||!span(g_esp-1024,1048))return 0;
 if(entry==0x2f0fc0){if(!safe(g_ecx,40)||!safe(MEM32(g_esp+4),8)||!safe(MEM32(g_esp+8),8)||!safe(MEM32(g_esp+12),8))return 0;construct();return 1;}
 if(entry==0x2f1030){if(!safe(g_ecx,40)||(MEM32(g_ecx+36)&&!safe(MEM32(g_ecx+36),17)))return 0;priority();return 1;}
 if(entry==0x2ff020){uint32_t src=MEM32(g_esp+8);if(!array_valid(g_ecx,28)||!array_valid(src,28)||MEM32(g_esp+4)>MEM32(g_ecx+4)||MEM32(g_ecx+4)+MEM32(src+4)>65536)return 0;
  if(MEM32(src+4)&&!apart(MEM32(g_ecx),(MEM32(g_ecx+8)&0x3fffffff)*28,MEM32(src),MEM32(src+4)*28))return 0;insert_records();return 1;}
 if(entry!=0x2ff110)return 0;
 uint32_t joint=MEM32(g_esp+8);if(!safe(joint,40)||MEM32(joint+36))return 0;
 uint32_t a=MEM32(joint+16),b=MEM32(joint+20),data=MEM32(joint+12);
 if(a==b||!body_valid(a)||!body_valid(b)||!safe(data,8)||!safe(MEM32(data),36))return 0;
 install();return 1;
}
