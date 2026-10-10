#include "hybrid_particle_parser_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0023EEA0(void),sub_0023F750(void),sub_002D6A1B(void),sub_002D589F(void),sub_002D631D(void);
static int parser_enabled(void){static int value=-1;if(value<0){const char*v=getenv("UCRR_NATIVE_PARTICLE_PARSER");value=v&&!strcmp(v,"1");}return value;}
static int parser_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}

/* Keep the source property objects in guest scratch. Their vtables are read
 * live, and a property callback may alter the descriptor, stack or registers. */
static void parser_property(void){
 uint32_t frame=g_seh_ebp;
 PUSH32(g_esp, 0xFFFFFFFFu);PUSH32(g_esp, 0x39E4EA);g_eax = MEM32(0);
 PUSH32(g_esp, g_eax);MEM32(0) = g_esp;g_esp = g_esp - 0x320;
 PUSH32(g_esp, g_ebx);g_ecx = g_esp + 0x28;PUSH32(g_esp, frame);
 MEM32(g_esp + 0x70) = g_ecx;PUSH32(g_esp, g_esi);g_eax = 0x4831F4;
 g_ecx = 2;PUSH32(g_esp, g_edi);MEM32(g_esp + 0x34) = 0x4837EC;
 MEM32(g_esp + 0x38) = 0x4837E4;MEM32(g_esp + 0x74) = g_eax;MEM32(g_esp + 0x7C) = g_ecx;
 g_edx = 0;g_esi = g_esp + 0x80;MEM32(g_esp + 0x338) = g_edx;
 MEM32(g_esp + 0x80) = 0x4837DC;MEM32(g_esp + 0x84) = 0x4837D4;MEM32(g_esp + 0x88) = 0x4837CC;
 MEM32(g_esp + 0x68) = g_eax;MEM32(g_esp + 0x6C) = g_esi;MEM32(g_esp + 0x70) = 3;
 g_esi = g_esp + 0x8C;MEM32(g_esp + 0x8C) = 0x4837C4;MEM32(g_esp + 0x90) = 0x4837B0;
 MEM32(g_esp + 0x94) = 0x4837A0;MEM32(g_esp + 0x98) = 0x483790;MEM32(g_esp + 0x9C) = 0x46B7C8;
 MEM32(g_esp + 0xA0) = 0x483780;MEM32(g_esp + 0x44) = g_eax;MEM32(g_esp + 0x48) = g_esi;
 MEM32(g_esp + 0x4C) = 6;g_esi = g_esp + 0xA4;MEM32(g_esp + 0xA4) = 0x483778;
 MEM32(g_esp + 0xA8) = 0x483774;MEM32(g_esp + 0xAC) = 0x48376C;MEM32(g_esp + 0xB0) = g_edx;
 MEM32(g_esp + 0xB4) = 0x483760;MEM32(g_esp + 0xB8) = 0x483754;MEM32(g_esp + 0xBC) = 0x483744;
 MEM32(g_esp + 0x50) = g_eax;MEM32(g_esp + 0x54) = g_esi;MEM32(g_esp + 0x58) = 7;
 MEM32(g_esp + 0x5C) = g_eax;g_eax = g_esp + 0x3C;MEM32(g_esp + 0x3C) = 0x48373C;
 MEM32(g_esp + 0x40) = 0x483730;MEM32(g_esp + 0x60) = g_eax;MEM32(g_esp + 0x64) = g_ecx;
 MEM32(g_esp + 0x10) = 0x483164;MEM32(g_esp + 0x28) = 0x483170;MEM32(g_esp + 0x24) = 0x483178;
 MEM32(g_esp + 0x30) = 0x483180;MEM32(g_esp + 0x20) = 0x483190;MEM32(g_esp + 0x14) = 0x4831A4;
 MEM32(g_esp + 0x18) = 0x4831AC;MEM32(g_esp + 0x2C) = 0x4831B4;MEM32(g_esp + 0x1C) = 0x4831EC;
 g_ecx = g_esp + 0x74;g_eax = g_esp + 0x28;MEM8(g_esp + 0x338) = 0xD;
 MEM32(g_esp + 0xC0) = 0x483720;MEM32(g_esp + 0xC4) = g_edx;MEM32(g_esp + 0xC8) = g_ecx;
 MEM32(g_esp + 0xCC) = 0x483708;MEM32(g_esp + 0xD0) = 4;MEM32(g_esp + 0xD4) = g_eax;
 MEM32(g_esp + 0xD8) = 0x4836EC;g_ecx = g_esp + 0x10;MEM32(g_esp + 0xE0) = g_ecx;
 g_eax = g_esp + 0x24;MEM32(g_esp + 0xEC) = g_eax;g_ecx = g_esp + 0x68;
 MEM32(g_esp + 0xF8) = g_ecx;g_ecx = g_esp + 0x10;g_eax = g_esp + 0x20;
 MEM32(g_esp + 0x104) = g_eax;g_eax = g_ecx;g_esi = g_esp + 0x10;
 MEM32(g_esp + 0x110) = g_ecx;MEM32(g_esp + 0x11C) = g_eax;MEM32(g_esp + 0x128) = g_ecx;
 MEM32(g_esp + 0x134) = g_eax;g_ecx = g_esp + 0x24;g_eax = g_esp + 0x2C;
 MEM32(g_esp + 0x140) = g_ecx;MEM32(g_esp + 0x14C) = g_eax;g_ecx = g_esp + 0x44;
 g_eax = g_esp + 0x10;MEM32(g_esp + 0x158) = g_ecx;MEM32(g_esp + 0x164) = g_eax;
 g_ecx = 0xE0;g_eax = 0xE4;MEM32(g_esp + 0x170) = g_esi;
 MEM32(g_esp + 0x17C) = g_esi;MEM32(g_esp + 0x188) = g_esi;MEM32(g_esp + 0x194) = g_esi;
 MEM32(g_esp + 0x1A0) = g_esi;MEM32(g_esp + 0x1AC) = g_esi;MEM32(g_esp + 0x1B8) = g_esi;
 MEM32(g_esp + 0x1C4) = g_esi;MEM32(g_esp + 0x1D0) = g_esi;MEM32(g_esp + 0x1DC) = g_esi;
 MEM32(g_esp + 0xDC) = 8;MEM32(g_esp + 0xE4) = 0x4836D8;MEM32(g_esp + 0xE8) = 0xC;
 MEM32(g_esp + 0xF0) = 0x4836C8;MEM32(g_esp + 0xF4) = 0x10;MEM32(g_esp + 0xFC) = 0x4836B8;
 MEM32(g_esp + 0x100) = 0x14;MEM32(g_esp + 0x108) = 0x4836A4;MEM32(g_esp + 0x10C) = 0x2C;
 MEM32(g_esp + 0x114) = 0x483690;MEM32(g_esp + 0x118) = 0x30;MEM32(g_esp + 0x120) = 0x48367C;
 MEM32(g_esp + 0x124) = 0x34;MEM32(g_esp + 0x12C) = 0x483668;MEM32(g_esp + 0x130) = 0x38;
 MEM32(g_esp + 0x138) = 0x483654;MEM32(g_esp + 0x13C) = 0x3C;MEM32(g_esp + 0x144) = 0x483640;
 MEM32(g_esp + 0x148) = 0xA4;MEM32(g_esp + 0x150) = 0x483630;MEM32(g_esp + 0x154) = 0xD0;
 MEM32(g_esp + 0x15C) = 0x483614;MEM32(g_esp + 0x160) = g_ecx;MEM32(g_esp + 0x168) = 0x4835F8;
 MEM32(g_esp + 0x16C) = g_eax;MEM32(g_esp + 0x174) = 0x4835DC;MEM32(g_esp + 0x178) = 0xE8;
 MEM32(g_esp + 0x180) = 0x4835C0;MEM32(g_esp + 0x184) = 0xEC;MEM32(g_esp + 0x18C) = 0x483594;
 MEM32(g_esp + 0x190) = 0xDC;MEM32(g_esp + 0x198) = 0x48356C;MEM32(g_esp + 0x19C) = 0xF8;
 MEM32(g_esp + 0x1A4) = 0x483548;MEM32(g_esp + 0x1A8) = 0xFC;MEM32(g_esp + 0x1B0) = 0x483524;
 MEM32(g_esp + 0x1B4) = 0x104;MEM32(g_esp + 0x1BC) = 0x483500;MEM32(g_esp + 0x1C0) = 0x108;
 MEM32(g_esp + 0x1C8) = 0x4834DC;MEM32(g_esp + 0x1CC) = 0x100;MEM32(g_esp + 0x1D4) = 0x4834BC;
 MEM32(g_esp + 0x1D8) = 0x10C;MEM32(g_esp + 0x1E0) = 0x483498;MEM32(g_esp + 0x1E4) = 0xF0;
 g_esi = g_esp + 0x30;MEM32(g_esp + 0x1E8) = g_esi;g_esi = g_esp + 0x20;
 MEM32(g_esp + 0x1F4) = g_esi;MEM32(g_esp + 0x200) = g_esi;g_esi = g_esp + 0x10;
 MEM32(g_esp + 0x20C) = g_esi;g_esi = g_esp + 0x14;MEM32(g_esp + 0x218) = g_esi;
 g_esi = g_esp + 0x18;MEM32(g_esp + 0x224) = g_esi;g_esi = g_esp + 0x14;
 MEM32(g_esp + 0x230) = g_esi;g_esi = g_esp + 0x18;MEM32(g_esp + 0x23C) = g_esi;
 g_esi = g_esp + 0x14;MEM32(g_esp + 0x248) = g_esi;g_esi = g_esp + 0x18;
 MEM32(g_esp + 0x254) = g_esi;g_esi = g_esp + 0x10;MEM32(g_esp + 0x260) = g_esi;
 MEM32(g_esp + 0x26C) = g_esi;MEM32(g_esp + 0x278) = g_esi;MEM32(g_esp + 0x284) = g_esi;
 MEM32(g_esp + 0x290) = g_esi;g_esi = g_esp + 0x50;MEM32(g_esp + 0x29C) = g_esi;
 g_esi = g_esp + 0x5C;MEM32(g_esp + 0x2A8) = g_esi;g_esi = g_esp + 0x1C;
 MEM32(g_esp + 0x2B4) = g_esi;g_esi = g_esp + 0x10;MEM32(g_esp + 0x2C0) = g_esi;
 MEM32(g_esp + 0x2CC) = g_esi;g_esi = g_esp + 0x1C;MEM32(g_esp + 0x2D8) = g_esi;
 MEM32(g_esp + 0x2E4) = g_esi;MEM32(g_esp + 0x1EC) = 0x483484;MEM32(g_esp + 0x1F0) = 0x40;
 MEM32(g_esp + 0x1F8) = 0x483474;MEM32(g_esp + 0x1FC) = 0x4C;MEM32(g_esp + 0x204) = 0x483464;
 MEM32(g_esp + 0x208) = 0x58;MEM32(g_esp + 0x210) = 0x48344C;MEM32(g_esp + 0x214) = 0x60;
 MEM32(g_esp + 0x21C) = 0x483438;MEM32(g_esp + 0x220) = 0x6C;MEM32(g_esp + 0x228) = 0x483424;
 MEM32(g_esp + 0x22C) = 0x70;MEM32(g_esp + 0x234) = 0x483410;MEM32(g_esp + 0x238) = 0x7C;
 MEM32(g_esp + 0x240) = 0x4833FC;MEM32(g_esp + 0x244) = 0x80;MEM32(g_esp + 0x24C) = 0x4833E8;
 MEM32(g_esp + 0x250) = 0x8C;MEM32(g_esp + 0x258) = 0x4833CC;MEM32(g_esp + 0x25C) = 0x90;
 MEM32(g_esp + 0x264) = 0x4833B4;MEM32(g_esp + 0x268) = 0x94;MEM32(g_esp + 0x270) = 0x4833A0;
 MEM32(g_esp + 0x274) = 0x98;MEM32(g_esp + 0x27C) = 0x48338C;MEM32(g_esp + 0x280) = 0x9C;
 MEM32(g_esp + 0x288) = 0x483378;MEM32(g_esp + 0x28C) = 0xA0;MEM32(g_esp + 0x294) = 0x483368;
 MEM32(g_esp + 0x298) = 0xA8;MEM32(g_esp + 0x2A0) = 0x483354;MEM32(g_esp + 0x2A4) = 0xD4;
 MEM32(g_esp + 0x2AC) = 0x48333C;MEM32(g_esp + 0x2B0) = 0xAC;MEM32(g_esp + 0x2B8) = 0x483324;
 MEM32(g_esp + 0x2BC) = 0x110;MEM32(g_esp + 0x2C4) = 0x483310;MEM32(g_esp + 0x2C8) = 0xB0;
 MEM32(g_esp + 0x2D0) = 0x4832F4;MEM32(g_esp + 0x2D4) = 0xB5;MEM32(g_esp + 0x2DC) = 0x4832D8;
 MEM32(g_esp + 0x2E0) = 0xB6;MEM32(g_esp + 0x2E8) = 0x4832C0;MEM32(g_esp + 0x2EC) = 0xC0;
 g_esi = g_esp + 0x14;MEM32(g_esp + 0x2F0) = g_esi;g_esi = g_esp + 0x18;
 MEM32(g_esp + 0x2FC) = g_esi;g_esi = g_esp + 0x10;MEM32(g_esp + 0x304) = g_ecx;
 MEM32(g_esp + 0x310) = g_eax;MEM32(g_esp + 0x31C) = g_ecx;MEM32(g_esp + 0x328) = g_eax;
 g_ecx = g_esi;g_eax = g_esi;MEM32(g_esp + 0x2F4) = 0x4832A4;
 MEM32(g_esp + 0x2F8) = 0xCC;MEM32(g_esp + 0x300) = 0x48327C;MEM32(g_esp + 0x308) = g_esi;
 MEM32(g_esp + 0x30C) = 0x483254;MEM32(g_esp + 0x314) = g_esi;MEM32(g_esp + 0x318) = 0x483228;
 MEM32(g_esp + 0x320) = g_ecx;MEM32(g_esp + 0x324) = 0x4831FC;MEM32(g_esp + 0x32C) = g_eax;
 g_edi = 0;frame = g_esp + 0xC0;
 for(;;){
  g_esi=MEM32(frame);g_eax=MEM32(g_esp+0x340);
  for(;;){
   SET_LO8(g_ebx,MEM8(g_eax));SET_LO8(g_ecx,LO8(g_ebx));
   unsigned other=MEM8(g_esi);
   if(LO8(g_ebx)!=other){g_eax=LO8(g_ebx)<other?0xffffffffu:1;break;}
   if(LO8(g_ecx)==LO8(g_edx)){g_eax=0;break;}
   SET_LO8(g_ebx,MEM8(g_eax+1));SET_LO8(g_ecx,LO8(g_ebx));other=MEM8(g_esi+1);
   if(LO8(g_ebx)!=other){g_eax=LO8(g_ebx)<other?0xffffffffu:1;break;}
   g_eax+=2;g_esi+=2;
   if(LO8(g_ecx)==LO8(g_edx)){g_eax=0;break;}
  }
  if(g_eax==g_edx){
   g_ebx=MEM32(g_esp+0x348);g_eax=g_edi+g_edi*2;g_eax<<=2;
   g_ecx=MEM32(g_esp+g_eax+0xc8);g_eax=MEM32(g_esp+g_eax+0xc4);g_edx=MEM32(g_ecx);g_eax+=g_ebx;
   {uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+0x348);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x23f73c);RECOMP_ICALL_SAFE(target,saved);}
   SET_LO8(g_eax,LO8(g_eax)!=0);break;
  }
  ++g_edi;frame+=12;
  if(g_edi>=0x34){SET_LO8(g_eax,1);break;}
 }
 g_ecx=MEM32(g_esp+0x330);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
 MEM32(0)=g_ecx;g_esp+=0x32c;g_recomp_zf=g_esp==0;g_esp+=4;
}

/* Source-valid input includes completing classification/property callbacks and
 * lines that fit the original guest scratch buffers. No private preparse or
 * post-callback replay changes its mutable parsing state. */
static void parser_text(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=0x808;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+0x818);
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+0x818);PUSH32(g_esp,g_edi);g_ebx=0;
 for(;;){
  SET_LO8(g_eax,MEM8(g_esi));
  if(LO8(g_eax)==0x20||LO8(g_eax)==9){++g_esi;continue;}
  SET_LO8(g_eax,MEM8(g_esi));
  if(LO8(g_eax)==10||LO8(g_eax)==13){++g_esi;continue;}
  if(LO8(g_eax)==0x23){
   do{SET_LO8(g_eax,MEM8(g_esi+1));++g_esi;}while(LO8(g_eax)!=10&&LO8(g_eax)!=13&&LO8(g_eax)!=LO8(g_ebx));
   continue;
  }
  if(LO8(g_eax)==LO8(g_ebx)){POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);break;}
  g_edi=0;
  do{
   SET_LO8(g_eax,MEM8(g_esi));g_ecx=(uint32_t)(int32_t)SMEM8(g_esi+1);MEM8(g_esp+g_edi+0x414)=LO8(g_eax);++g_edi;++g_esi;
   PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x23f7c5);sub_002D6A1B();g_esp+=4;
  }while(g_eax||MEM8(g_esi)==0x5f);
  MEM8(g_esp+g_edi+0x414)=LO8(g_ebx);
  for(;;){SET_LO8(g_eax,MEM8(g_esi));if(LO8(g_eax)!=0x20&&LO8(g_eax)!=9)break;++g_esi;}
  SET_LO8(g_edx,MEM8(g_esi));SET_LO8(g_eax,MEM8(g_esi+1));++g_esi;MEM8(g_esp+0x10)=LO8(g_edx);g_ecx=1;
  while(LO8(g_eax)!=10&&LO8(g_eax)!=13&&LO8(g_eax)!=LO8(g_ebx)){
   MEM8(g_esp+g_ecx+0x10)=LO8(g_eax);SET_LO8(g_eax,MEM8(g_esi+1));++g_ecx;++g_esi;
  }
  PUSH32(g_esp,frame);g_eax=g_esp+0x14;MEM8(g_esp+g_ecx+0x14)=LO8(g_ebx);PUSH32(g_esp,g_eax);g_ecx=g_esp+0x41c;PUSH32(g_esp,g_ecx);
  PUSH32(g_esp,0x23f824);sub_0023EEA0();g_esp+=12;
  if(!LO8(g_eax)){POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);break;}
 }
 g_esp+=0x808;g_recomp_zf=g_esp==0;g_esp+=4;
}

static void parser_stream(void){
 uint32_t frame=g_seh_ebp;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_eax=MEM32(g_edi);g_ecx=g_edi;uint32_t target=MEM32(g_eax+16);PUSH32(g_esp,0x23f85d);RECOMP_ICALL_SAFE(target,saved);}
 g_esi=g_eax;
 if((int32_t)g_esi<4){POP32(g_esp,g_edi);SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_recomp_zf=1;g_esp+=4;return;}
 g_edx=MEM32(g_edi);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,4);g_ecx=g_edi;uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x23f874);RECOMP_ICALL_SAFE(target,saved);}
 g_esi-=4;g_eax=g_esi+1;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23f880);sub_002D589F();g_edx=MEM32(g_edi);g_esp+=4;frame=g_eax;
 {uint32_t saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);g_ecx=g_edi;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x23f88e);RECOMP_ICALL_SAFE(target,saved);}
 MEM8(g_esi+frame)=0;g_esi=MEM32(g_esp+24);PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);PUSH32(g_esp,0x23f89d);sub_0023F750();SET_LO8(g_ebx,LO8(g_eax));g_esp+=8;
 if(LO8(g_ebx)){
  uint32_t flags;
  g_xmm1=XMM_SCALAR(MEMF(g_esi+0x90));g_xmm0=XMM_SCALAR(MEMF(0x44bf80));
  {double a=g_xmm1.f[0],b=g_xmm0.f[0];flags=(a!=a||b!=b)?0x47u:a<b?0x03u:a>b?0x02u:0x42u;}
  SET_HI8(g_eax,flags);
  if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){
   g_xmm1=XMM_SCALAR(MEMF(g_esi+0x94));
   {double a=g_xmm1.f[0],b=g_xmm0.f[0];flags=(a!=a||b!=b)?0x47u:a<b?0x03u:a>b?0x02u:0x42u;}
   SET_HI8(g_eax,flags);
   if(!RECOMP_PARITY8(HI8(g_eax)&0x44)){g_xmm0=XMM_ZERO();MEMF(g_esi+0x94)=g_xmm0.f[0];MEMF(g_esi+0x90)=g_xmm0.f[0];g_xmm0=XMM_MEM(g_esi+0x60);XMM_STORE(g_esi+0x70,g_xmm0);}
  }
 }
 PUSH32(g_esp,frame);PUSH32(g_esp,0x23f8f1);sub_002D631D();g_esp+=4;POP32(g_esp,frame);SET_LO8(g_eax,LO8(g_ebx));POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_hybrid_particle_parser(unsigned operation){
 if(!parser_enabled()||operation>2)return 0;
 uint32_t start=g_esp;
 if((start&3)||start<0x11000u||!parser_span(start-4096,4112)||!parser_span(0x51c178,1)||MEM8(0x51c178))return 0;
 if(operation==0)parser_property();else if(operation==1)parser_text();else parser_stream();return 1;
}
