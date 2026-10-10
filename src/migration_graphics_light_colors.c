#include "migration_graphics_light_colors.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int colors_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_LIGHT_COLORS");flag=v&&!strcmp(v,"1");}return flag;}
static int colors_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void color_push(double v){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=v;}
static void color_store(uint32_t p){MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
#define COLOR_TOP g_fp_stack[g_fp_top]
static void light_colors(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(0x3f41d4);PUSH32(g_esp,frame);frame=(MEM32(g_esp+12)+0x4820)<<7;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(0x3f4238);g_esi=g_edi+0xf00;MEM32(g_eax)=frame;
 for(;;){
  const unsigned material[]={16,0,32},light[]={36,4,20},masks[]={12,48,192};
  for(unsigned channel=0;channel<3;channel++)for(unsigned component=0;component<3;component++){
   unsigned source=g_ecx+light[channel]+4*component,out=g_eax+4+channel*12+component*4;
   if(g_edx&masks[channel]){uint32_t value=MEM32(source);if(channel==2)g_esi=value;else frame=value;MEM32(out)=value;}
   else{unsigned mat=g_esi+material[channel]+component*4;
    if(channel==1&&component==0){color_push(MEMF(source));COLOR_TOP*=MEMF(mat);}else{color_push(MEMF(mat));COLOR_TOP*=MEMF(source);}color_store(out);
   }
  }
  g_eax+=40;g_esi=g_ebx;g_ebx--;if(!g_esi)break;
  g_esi=(MEM32(g_esp+20)+0x9030)<<6;MEM32(g_eax)=g_esi;g_edx>>=8;g_esi=g_edi+0xf44;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
static void ambient_colors(void){
 uint32_t frame=g_seh_ebp;g_esp-=28;g_edx=MEM32(0x3f4238);g_ecx=MEM32(0x3f41d4);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);MEM32(g_esp+40)=g_edx;MEM32(g_esp+36)=g_ecx;g_ecx=MEM32(0x3f416c);g_edx+=0xf00;g_esi=0xc0a10;g_edi=0xc03a8;frame=0x403b4;
 for(;;){
  g_ebx=(g_ecx>>16)&255;MEM32(g_esp+20)=g_ebx;color_push((double)(int32_t)g_ebx);g_ebx=(g_ecx>>8)&255;COLOR_TOP*=MEMF(0x44c23c);MEM32(g_esp+20)=g_ebx;color_push((double)(int32_t)g_ebx);COLOR_TOP*=MEMF(0x44c23c);g_ecx&=255;MEM32(g_esp+16)=g_ecx;color_store(g_esp+20);color_push((double)(int32_t)g_ecx);g_ebx=MEM32(g_esp+48);COLOR_TOP*=MEMF(0x44c23c);color_store(g_esp+16);
  if(g_ebx&12){
   g_ecx=MEM32(g_esp+20);color_store(g_esp+24);color_push(MEMF(g_edx+48));MEM32(g_esp+28)=g_ecx;g_ecx=MEM32(g_esp+16);color_push(MEMF(g_edx+52));MEM32(g_esp+32)=g_ecx;g_ecx=MEM32(g_edx+56);MEM32(g_esp+16)=g_ecx;
  }else{
   COLOR_TOP*=MEMF(g_edx+16);
   if(g_ebx&3){color_push(MEMF(g_esp+20));MEM32(g_esp+24)=0x3f800000;COLOR_TOP*=MEMF(g_edx+20);MEM32(g_esp+28)=0x3f800000;color_push(MEMF(g_esp+16));MEM32(g_esp+32)=0x3f800000;COLOR_TOP*=MEMF(g_edx+24);}
   else{COLOR_TOP+=MEMF(g_edx+48);color_push(MEMF(g_esp+20));MEM32(g_esp+24)=0;COLOR_TOP*=MEMF(g_edx+20);MEM32(g_esp+28)=0;MEM32(g_esp+32)=0;COLOR_TOP+=MEMF(g_edx+52);color_push(MEMF(g_esp+16));COLOR_TOP*=MEMF(g_edx+24);COLOR_TOP+=MEMF(g_edx+56);}
   color_store(g_esp+16);
  }
  g_ecx=MEM32(g_esp+16);double swap=COLOR_TOP;COLOR_TOP=g_fp_stack[(g_fp_top+1)&7];g_fp_stack[(g_fp_top+1)&7]=swap;MEM32(g_eax+12)=g_ecx;color_store(g_eax+4);g_ecx=MEM32(g_esp+24);MEM32(g_eax+20)=g_ecx;color_store(g_eax+8);g_ecx=MEM32(g_esp+28);MEM32(g_eax+24)=g_ecx;g_ecx=MEM32(g_esp+32);MEM32(g_eax)=g_esi;MEM32(g_eax+16)=g_edi;MEM32(g_eax+28)=g_ecx;g_ecx=MEM32(g_esp+36);MEM32(g_eax+32)=frame;g_edx=MEM32(g_edx+12);MEM32(g_eax+36)=g_edx;g_eax+=40;g_edx=g_ecx;g_ecx--;MEM32(g_esp+36)=g_ecx;if(!g_edx)break;
  g_edx=MEM32(g_esp+40);g_ecx=MEM32(0x3f4168);g_ebx>>=8;MEM32(g_esp+48)=g_ebx;g_edx+=0xf44;g_esi=0xc17a0;g_edi=0xc17b0;frame=0x417ac;
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_graphics_light_colors(uint32_t entry){
 if(!colors_enabled())return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10100||!colors_span(s-64,80)||!colors_span(0x51c178,1)||MEM8(0x51c178)||(unsigned)g_fp_top>7||!colors_span(0x3f41d4,104)||MEM32(0x3f41d4)>16||!colors_span(MEM32(0x3f4238),0xf88)||!colors_span(g_eax,40ull*(MEM32(0x3f41d4)+1)))return 0;
 if(entry==0x3edc10){if(!colors_span(g_ecx,48))return 0;light_colors();return 1;}
 if(entry==0x3edd00){if(!colors_span(0x44c23c,4))return 0;ambient_colors();return 1;}return 0;
}
