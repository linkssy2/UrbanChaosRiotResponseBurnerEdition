/* Original hinge/ragdoll data, ordered copies and motor reference ownership. */
#include "migration_physics_joint_data.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-64||(uint64_t)g_esp+8<=p);}
static void base_construct(int ragdoll){
 g_eax=g_ecx;g_xmm0=XMM_ZERO();if(ragdoll)g_ecx=1;MEM16(g_eax+6)=1;MEM32(g_eax)=ragdoll?0x49de88:0x49a9d4;
 if(ragdoll){const uint32_t v[]={0x3f800000,0xbccccccd,0x3ccccccd,0xbf060aa6,0x3f060aa6,0};for(unsigned i=0;i<6;i++)MEM32(g_eax+112+i*4)=v[i];}
 else{MEM32(g_eax+12)=0xc0490fdb;MEM32(g_eax+16)=0x40490fdb;MEM32(g_eax+20)=0;}
 for(unsigned o=ragdoll?16:32;o<=(ragdoll?96u:128u);o+=16)XMM_STORE(g_eax+o,g_xmm0);
 if(ragdoll)MEM8(g_eax+12)=LO8(g_ecx);g_esp+=4;
}
static void motor_construct(int ragdoll){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,ragdoll?0x32a688:0x2fe238);base_construct(ragdoll);
 if(ragdoll){MEM32(g_esi)=0x49e144;MEM8(g_esi+144)=1;g_eax=0;MEM32(g_esi+208)=g_eax;MEM32(g_esi+212)=g_eax;MEM32(g_esi+216)=g_eax;g_xmm0=XMM_ZERO();XMM_STORE(g_esi+160,g_xmm0);XMM_STORE(g_esi+176,g_xmm0);g_eax=0x3f800000;XMM_STORE(g_esi+192,g_xmm0);MEM32(g_esi+160)=g_eax;MEM32(g_esi+180)=g_eax;MEM32(g_esi+200)=g_eax;}
 else{SET_LO8(g_eax,1);MEM32(g_esi)=0x49ad60;MEM8(g_esi+144)=LO8(g_eax);MEM8(g_esi+145)=LO8(g_eax);g_eax=0;MEM32(g_esi+148)=g_eax;MEM32(g_esi+152)=g_eax;}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=(g_eax==0);g_esp+=4;
}
static void base_copy(int ragdoll){
 g_eax=g_ecx;g_ecx=MEM32(g_esp+4);SET_LO16(g_edx,MEM16(g_ecx+4));MEM16(g_eax+4)=LO16(g_edx);SET_LO16(g_edx,MEM16(g_ecx+6));MEM16(g_eax+6)=LO16(g_edx);g_edx=MEM32(g_ecx+8);MEM32(g_eax+8)=g_edx;
 if(ragdoll){SET_LO8(g_edx,MEM8(g_ecx+12));MEM8(g_eax+12)=LO8(g_edx);}else for(unsigned o=12;o<=20;o+=4){g_edx=MEM32(g_ecx+o);MEM32(g_eax+o)=g_edx;}
 for(unsigned o=ragdoll?16:32;o<=(ragdoll?96u:128u);o+=16){g_xmm0=XMM_MEM(g_ecx+o);XMM_STORE(g_eax+o,g_xmm0);}
 if(ragdoll){for(unsigned o=112;o<=128;o+=4){g_edx=MEM32(g_ecx+o);MEM32(g_eax+o)=g_edx;}g_ecx=MEM32(g_ecx+132);MEM32(g_eax+132)=g_ecx;}g_esp+=8;
}
static void motor_copy(int ragdoll){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_esi=g_ecx;PUSH32(g_esp,ragdoll?0x2f123e:0x2f11ee);base_copy(ragdoll);SET_LO8(g_eax,MEM8(g_edi+144));MEM8(g_esi+144)=LO8(g_eax);
 if(ragdoll){for(unsigned o=160;o<=192;o+=16){g_xmm0=XMM_MEM(g_edi+o);XMM_STORE(g_esi+o,g_xmm0);}g_ecx=MEM32(g_edi+208);MEM32(g_esi+208)=g_ecx;g_edx=MEM32(g_edi+212);MEM32(g_esi+212)=g_edx;g_eax=MEM32(g_edi+216);MEM32(g_esi+216)=g_eax;}
 else{SET_LO8(g_ecx,MEM8(g_edi+145));MEM8(g_esi+145)=LO8(g_ecx);g_edx=MEM32(g_edi+148);MEM32(g_esi+148)=g_edx;g_eax=MEM32(g_edi+152);MEM32(g_esi+152)=g_eax;}
 POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;
}
static void set_motor(unsigned offset,unsigned ret){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=g_ecx;
 if(g_esi&&MEM16(g_esi+4))++MEM16(g_esi+6);
 g_ecx=MEM32(g_edi+offset);if(g_ecx&&MEM16(g_ecx+4)){--MEM16(g_ecx+6);if(!MEM16(g_ecx+6)){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}}
 MEM32(g_edi+offset)=g_esi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
int uc_migration_physics_joint_data(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_JOINT_DATA");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-64,80))return 0;
 unsigned size=0,offset=0,ret=0;int copy=0;
 switch(entry){case 0x2f48a0:size=144;break;case 0x31e3f0:size=136;break;case 0x2fe230:size=160;break;case 0x32a680:size=224;break;case 0x2f10e0:size=144;copy=1;break;case 0x2f1150:size=136;copy=1;break;case 0x2f11e0:size=160;copy=1;break;case 0x2f1230:size=224;copy=1;break;
 case 0x2fe260:offset=152;ret=0x2fe299;break;case 0x32a560:offset=208;ret=0x32a599;break;case 0x32a5c0:offset=212;ret=0x32a5f9;break;case 0x32a620:offset=216;ret=0x32a659;break;case 0x32a550:offset=208;break;case 0x32a5b0:offset=212;break;case 0x32a610:offset=216;break;default:return 0;}
 if(!safe(g_ecx,offset?offset+4:size)||(copy&&!safe(MEM32(g_esp+4),size)))return 0;
 if(ret){uint32_t incoming=MEM32(g_esp+4),old=MEM32(g_ecx+offset);if((incoming&&!safe(incoming,8))||(old&&(!safe(old,8)||!safe(MEM32(old),4))))return 0;set_motor(offset,ret);}
 else if(offset){g_eax=MEM32(g_ecx+offset);g_esp+=4;}
 else switch(entry){case 0x2f48a0:base_construct(0);break;case 0x31e3f0:base_construct(1);break;case 0x2fe230:motor_construct(0);break;case 0x32a680:motor_construct(1);break;case 0x2f10e0:base_copy(0);break;case 0x2f1150:base_copy(1);break;case 0x2f11e0:motor_copy(0);break;case 0x2f1230:motor_copy(1);break;}
 return 1;
}
