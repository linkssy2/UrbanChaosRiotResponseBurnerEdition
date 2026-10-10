#include "migration_interface_device.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00208880(void);
static int device_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_DEVICE");v=s&&!strcmp(s,"1");}return v;}
static int device_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int device_apart(uint64_t a,unsigned n,uint64_t b,unsigned m){return a+n<=b||b+m<=a;}
static void device_axis(void){
 g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_eax+0x40);g_edx=MEM32(g_esp+4);
 double value=MEMF(g_ecx+g_edx*4);g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=value;g_esp+=8;
}
static void device_button(void){
 g_eax=MEM32(g_ecx+4);g_ecx=MEM32(g_eax+0x44);g_edx=MEM32(g_esp+4);g_eax=MEM8(g_edx+g_ecx)&1;
 g_recomp_zf=g_eax==0;g_esp+=8;
}
static int device_source(uint32_t m,uint32_t s){
 if(!device_span(m,0xe0)||!device_apart(m,0xe0,s-64,80))return 0;
 uint32_t source=MEM32(m+4);if(!device_span(source,0x48)||!device_apart(source,0x48,s-64,80))return 0;
 uint32_t axes=MEM32(source+0x40),buttons=MEM32(source+0x44);
 return device_span(axes,80)&&device_span(buttons,20)&&device_apart(axes,80,s-64,80)&&device_apart(buttons,20,s-64,80)&&device_apart(axes,80,m,0xe0)&&device_apart(buttons,20,m,0xe0)&&device_apart(source,0x48,m,0xe0);
}
int migration_interface_device(unsigned op){
 if(!device_enabled()||op>4)return 0;
 uint32_t s=g_esp,m=g_ecx;if((s&3)||s<0x10040||!device_span(s-64,80))return 0;
 if(op==4){
  if(g_eax>3||!device_span(0x5096e0,4)||!device_span(MEM32(0x5096e0),24))return 0;
  g_ecx=MEM32(0x5096e0);
  if(MEM8(g_ecx+g_eax+0x10)==255)g_eax=UINT32_MAX;
  else{SET_LO8(g_edx,MEM8(g_ecx+g_eax+0x14));g_eax=LO8(g_edx)?(uint32_t)(int32_t)SMEM8(g_ecx+g_eax+12):UINT32_MAX;}
  g_recomp_zf=0;g_esp+=4;return 1;
 }
 if(op==1){
  int32_t port=(int32_t)MEM32(s+4);if(port<0||!device_span(m,0x24)||!device_apart(m,0x24,s-8,20))return 0;
  uint32_t base=MEM32(m+0x20),source=MEM32(s+8);uint64_t destination=(uint64_t)base+(uint32_t)port*0xd0ull;
  if(port<8&&base&&(!device_span(destination,0xd0)||!device_span(source,0xd0)||!device_apart(destination,0xd0,source,0xd0)||!device_apart(destination,0xd0,s-8,20)||!device_apart(source,0xd0,s-8,20)))return 0;
  PUSH32(g_esp,g_edi);g_edi=(uint32_t)port;
  if(port<8){g_eax=base;if(base){PUSH32(g_esp,g_esi);g_esi=source;g_edi=(uint32_t)destination;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),0xd0);g_ecx=0;POP32(g_esp,g_esi);}}
  POP32(g_esp,g_edi);RECOMP_TRACE_EXIT("sub_00208880",0x208880);g_esp+=12;return 1;
 }
 if(!device_source(m,s))return 0;
 if(op==2||op==3){if(MEM32(s+4)>=20)return 0;if(op==2)device_axis();else device_button();return 1;}
 /* The public copy entry retains the existing native host publication hook. */
 uint32_t manager=MEM32(m+8),port=MEM32(m+12);
 if(port>=8||!device_span(manager,0x24)||!device_apart(manager,0x24,s-64,80))return 0;
 uint32_t base=MEM32(manager+0x20);uint64_t destination=(uint64_t)base+port*0xd0ull;
 if(base&&(!device_span(destination,0xd0)||!device_apart(destination,0xd0,m,0xe0)||!device_apart(destination,0xd0,s-64,80)))return 0;
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_esi=m;g_edi=m+0x10;
 static const uint32_t axis_return[20]={0x21f75f,0x21f76a,0x21f776,0x21f782,0x21f950,0x21f962,0x21f974,0x21f986,0x21f998,0x21f9aa,0x21f9bc,0x21f9ce,0x21f9e0,0x21f9fb,0x21fa16,0x21fa31,0x21fa4c,0x21fa67,0x21fa82,0x21fa9d};
 static const uint32_t button_return[16]={0x21f78e,0x21f7a9,0x21f7c2,0x21f7db,0x21f7f4,0x21f80d,0x21f826,0x21f83f,0x21f858,0x21f877,0x21f896,0x21f8b5,0x21f8d4,0x21f8f3,0x21f912,0x21f931};
 for(unsigned i=0;i<4;i++){PUSH32(g_esp,i);g_ecx=g_esi;PUSH32(g_esp,axis_return[i]);device_axis();MEMF(g_esi+0x10+i*4)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;}
 for(unsigned i=0;i<16;i++){PUSH32(g_esp,i+4);g_ecx=g_esi;PUSH32(g_esp,button_return[i]);device_button();int down=LO8(g_eax)!=0;SET_LO8(g_eax,MEM8(g_esi+0x20+i*12));if(!i)SET_LO8(g_ebx,0xfc);SET_LO8(g_eax,down?LO8(g_eax)|3:LO8(g_eax)&LO8(g_ebx));MEM8(g_esi+0x20+i*12)=LO8(g_eax);}
 for(unsigned i=0;i<16;i++){
  PUSH32(g_esp,i+4);g_ecx=g_esi;PUSH32(g_esp,axis_return[i+4]);device_axis();uint32_t p=g_esi+0x24+i*12,old=MEM32(p);
  if(i%3==0)g_eax=old;else if(i%3==1)g_ecx=old;else g_edx=old;
  MEMF(p)=(float)g_fp_stack[g_fp_top];g_fp_top=(g_fp_top+1)&7;MEM32(p+4)=old;
 }
 SET_LO8(g_eax,0xfd);
 for(unsigned i=0;i<16;i++){uint32_t p=g_esi+0x20+i*12;if(MEM8(p+1)&1)MEM8(p)&=LO8(g_eax);if(i&1)SET_LO8(g_edx,MEM8(p));else SET_LO8(g_ecx,MEM8(p));MEM8(p+1)=MEM8(p);}
 g_eax=MEM32(g_esi+12);g_ecx=MEM32(g_esi+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x21fc0e);sub_00208880();
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;return 1;
}
