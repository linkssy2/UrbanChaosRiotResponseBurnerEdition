#include "hybrid_surface_access_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9890(void),sub_003EAFD0(void),sub_003EB200(void),sub_003EB2A0(void);
static int access_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_NATIVE_SURFACE_ACCESS");f=v&&!strcmp(v,"1");}return f;}
static int access_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int access_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
/* Keep synchronization and its live register/argument feedback. Layout remains
 * the original service until these additional consumers are jointly qualified.
 * No validation or whole-owner replay is allowed after the first source push. */
static void access_rectangle(void){
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_esp+32));PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+16);
 if(!(LO8(g_ebx)&0x20)){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3eb216);sub_003E9890();}
 g_eax=g_esp+24;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+16;PUSH32(g_esp,g_edx);
 g_edx=MEM32(g_esp+36);g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+36);g_ecx=g_esp+52;PUSH32(g_esp,g_ecx);
 PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x3eb23f);sub_003EAFD0();
 g_ecx=MEM32(g_esp+36);if(LO8(g_ebx)&0x40)g_ecx|=0xf0000000u;
 g_eax=MEM32(g_esp+32);
 if(g_eax){
  g_edx=MEM8(MEM8(g_esi+13)+0x3f1d70);g_esi=MEM32(g_eax+4);g_edx&=0x3c;g_edx*=MEM32(g_eax);g_eax=MEM32(g_esp+16);g_esi*=g_eax;g_edx>>=3;
  g_esi+=g_ecx;g_ecx=g_esi+g_edx;g_edx=MEM32(g_esp+28);POP32(g_esp,g_esi);MEM32(g_edx)=g_eax;MEM32(g_edx+4)=g_ecx;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=g_esi==0;
 }else{
  g_eax=MEM32(g_esp+28);g_edx=MEM32(g_esp+16);POP32(g_esp,g_esi);MEM32(g_eax)=g_edx;MEM32(g_eax+4)=g_ecx;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;
 }
 g_esp+=28;
}
static void access_volume(void){
 uint32_t frame=g_seh_ebp;
 g_esp-=8;PUSH32(g_esp,g_ebx);SET_LO8(g_ebx,MEM8(g_esp+32));PUSH32(g_esp,frame);frame=MEM32(g_esp+20);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);
 if(!(LO8(g_ebx)&0x20)){PUSH32(g_esp,frame);PUSH32(g_esp,0x3eb2ba);sub_003E9890();}
 g_eax=g_esp+16;PUSH32(g_esp,g_eax);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+44);
 g_eax=g_esp+44;PUSH32(g_esp,g_eax);g_ecx=g_esp+60;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);PUSH32(g_esp,frame);PUSH32(g_esp,0x3eb2e0);sub_003EAFD0();
 g_ecx=MEM32(g_esp+44);if(LO8(g_ebx)&0x40)g_ecx|=0xf0000000u;
 g_eax=MEM32(g_esp+40);g_esi=MEM32(g_esp+28);g_edi=MEM32(g_esp+32);
 if(g_eax){
  g_edx=MEM8(MEM8(frame+13)+0x3f1d70);g_ebx=MEM32(g_eax+4);g_edx&=0x3c;g_ebx*=g_edi;g_edx*=MEM32(g_eax);g_eax=MEM32(g_eax+16);g_eax*=g_esi;g_edx>>=3;
  g_edx+=g_ebx;g_eax+=g_ecx;g_ecx=g_eax+g_edx;
 }
 g_eax=MEM32(g_esp+36);MEM32(g_eax)=g_edi;POP32(g_esp,g_edi);MEM32(g_eax+4)=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(g_eax+8)=g_ecx;POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=24;
}
int uc_hybrid_surface_access(uint32_t entry){
 if(!access_enabled()||(entry!=0x3e61f0&&entry!=0x3eb200&&entry!=0x3eb2a0&&entry!=0x3f1950&&entry!=0x3f1980&&entry!=0x3f1990))return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10200||!access_span(s-256,284)||!access_span(0x51c178,1)||MEM8(0x51c178)||!access_apart(s-256,284,0x51c178,1)||!access_span(0x3f1d70,256)||!access_apart(s-256,284,0x3f1d70,256))return 0;
 uint32_t texture=MEM32(s+4),out,region;
 if(entry==0x3e61f0){out=MEM32(s+8);region=MEM32(s+12);}
 else if(entry==0x3eb200||entry==0x3f1980){out=MEM32(s+16);region=MEM32(s+20);}
 else{out=MEM32(s+12);region=MEM32(s+16);}
 unsigned volume=entry==0x3eb2a0||entry==0x3f1990;
 if(!access_span(texture,20)||!access_apart(texture,20,s-256,256)||!access_span(out,volume?12:8)||!access_apart(out,volume?12:8,0x51c178,1)||(region&&!access_span(region,volume?20:8)))return 0;
 switch(entry){
 case 0x3eb200:access_rectangle();break;
 case 0x3eb2a0:access_volume();break;
 case 0x3f1980:sub_003EB200();break;
 case 0x3f1990:sub_003EB2A0();break;
 case 0x3e61f0:
  g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esp+12);g_edx=MEM32(g_esp+8);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x3e620d);sub_003EB200();g_esp+=20;break;
 default:
  g_eax=MEM32(g_esp+20);g_ecx=MEM32(g_esp+16);g_edx=MEM32(g_esp+12);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x3f1970);sub_003EB200();g_esp+=24;break;
 }
 return 1;
}
