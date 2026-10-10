#include "migration_interface_profileio.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_00081DA2(void),sub_00081EAC(void),sub_00081EF3(void),sub_00080491(void),sub_000808D5(void),sub_00081265(void),sub_000811E6(void),sub_00081221(void),sub_00080D27(void),sub_00080C80(void),sub_00080C94(void),sub_00080CE2(void),sub_00081B3F(void),sub_00080E14(void),sub_00080EEA(void),sub_000816D2(void);
static int profileio_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void profileio_load(void){
 PUSH32(g_esp,g_ebp);uint32_t frame=g_esp;g_ebp=frame;g_esp=(g_esp&~7u)-0x7e4;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);uint32_t base=g_esp;
 g_eax=0;g_ecx=209;g_edi=base+0x4a8;memset((void*)XBOX_PTR(g_edi),0,836);g_edi+=836;g_ecx=0;g_eax=base+0x160;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x45236c);PUSH32(g_esp,0x63e01);sub_00081DA2();
 g_ecx=g_esp+0x28;PUSH32(g_esp,g_ecx);g_esi=g_eax;g_edx=g_esp+0x24;PUSH32(g_esp,g_edx);g_eax=g_esp+0x18;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x45236c);MEM32(g_esp+0x1c)=g_esi;g_ebp=frame;PUSH32(g_esp,0x63e20);sub_00080491();PUSH32(g_esp,0x45236c);g_ebp=frame;PUSH32(g_esp,0x63e2a);sub_000808D5();
 g_ecx=g_eax;g_edx=0;g_eax=1680;g_edx=g_eax%g_ecx;g_eax/=g_ecx;g_eax=((g_eax+3)*g_ecx>>14)+1;MEM32(0x4e8358)=g_eax;
 if(g_esi==UINT32_MAX){g_ecx=MEM32(base+16);g_edx=MEM32(base+20);g_ecx=(g_ecx>>14)|(g_edx<<18);g_edx>>=14;g_eax=g_ecx<g_eax?5:0;g_recomp_zf=!g_eax;goto done;}
 do{
  g_ecx=MEM32(base+0x4c0);g_edx=MEM32(base+0x4bc);g_edi=MEM32(base+0x174);g_eax=MEM32(base+0x178);g_esi=g_edx;g_ebx=0;g_edx=g_edi;
  if(g_eax>g_ecx||(g_eax==g_ecx&&g_edx>g_esi)){g_ecx=209;g_esi=base+0x160;g_edi=base+0x4a8;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),836);g_esi+=836;g_edi+=836;g_ecx=0;}
  g_esi=MEM32(base+12);g_ecx=base+0x160;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63ecd);sub_00081EAC();
 }while(g_eax);
 PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63ed7);sub_00081EF3();g_eax=0;
 do{SET_LO8(g_ecx,MEM8(base+g_eax+0x5e8));MEM8(base+g_eax+0x58)=LO8(g_ecx);g_eax++;}while(LO8(g_ecx));g_eax=base+0x58;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));
 g_ecx=MEM32(0x452374);PUSH32(g_esp,0);g_eax-=g_edx;PUSH32(g_esp,0x80);g_edx=g_esp+g_eax+0x60;g_eax=MEM32(0x452370);PUSH32(g_esp,3);PUSH32(g_esp,0);MEM32(g_edx)=g_eax;SET_LO8(g_eax,MEM8(0x452378));PUSH32(g_esp,1);MEM32(g_edx+4)=g_ecx;PUSH32(g_esp,0x80000000);g_ecx=g_esp+0x70;PUSH32(g_esp,g_ecx);MEM8(g_edx+8)=LO8(g_eax);g_ebp=frame;PUSH32(g_esp,0x63f38);sub_00081265();g_esi=g_eax;
 if(g_esi==UINT32_MAX){g_eax=2;goto done;}PUSH32(g_esp,0);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63f47);sub_000811E6();
 if(g_eax>1680){PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63f54);sub_00081221();g_eax=2;goto done;}
 g_edi=MEM32(frame+8);PUSH32(g_esp,0);g_edx=g_esp+0x1c;PUSH32(g_esp,g_edx);PUSH32(g_esp,1660);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63f76);sub_00080D27();PUSH32(g_esp,0);g_ebx=g_eax;g_eax=g_esp+0x20;PUSH32(g_esp,g_eax);PUSH32(g_esp,20);g_ecx=g_esp+0x50;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63f8c);sub_00080D27();PUSH32(g_esp,g_esi);MEM32(g_esp+0x10)=g_eax;g_ebp=frame;PUSH32(g_esp,0x63f96);sub_00081221();PUSH32(g_esp,0);g_ebp=frame;PUSH32(g_esp,0x63f9d);sub_00080C80();g_esi=g_eax;
 if(g_esi!=UINT32_MAX){PUSH32(g_esp,1660);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63fb0);sub_00080C94();g_edx=g_esp+0x30;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x63fbb);sub_00080CE2();}
 if(g_ebx){g_eax=MEM32(base+12);if(g_eax){g_ecx=base+0x44;g_esi=MEM32(g_ecx);g_edx=base+0x30;g_eax=MEM32(g_edx)!=g_esi?2:0;g_recomp_zf=!g_eax;goto done;}}
 g_eax=3;
done:POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame+8;
}
static void profileio_save(void){
 uint32_t ebp=g_seh_ebp;g_esp-=0x11c;PUSH32(g_esp,260);g_eax=g_esp+0x1c;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,4);PUSH32(g_esp,0x4523a0);PUSH32(g_esp,0x45236c);PUSH32(g_esp,0x64013);sub_00081B3F();if(g_eax){g_eax=3;goto finish;}
 g_eax=g_esp+0x18;g_edx=g_eax+1;do{SET_LO8(g_ecx,MEM8(g_eax));g_eax++;}while(LO8(g_ecx));PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);PUSH32(g_esp,0);g_eax-=g_edx;g_edx=MEM32(0x452370);PUSH32(g_esp,0x80);g_edi=g_esp+g_eax+0x28;g_eax=MEM32(0x452374);PUSH32(g_esp,4);PUSH32(g_esp,0);g_ecx=g_edi;MEM32(g_ecx)=g_edx;SET_LO8(g_edx,MEM8(0x452378));PUSH32(g_esp,0);MEM32(g_ecx+4)=g_eax;PUSH32(g_esp,0x40000000);g_eax=g_esp+0x38;PUSH32(g_esp,g_eax);MEM8(g_ecx+8)=LO8(g_edx);PUSH32(g_esp,0x64076);sub_00081265();g_esi=g_eax;
 if(g_esi==UINT32_MAX){POP32(g_esp,g_edi);g_eax=3;POP32(g_esp,g_esi);goto finish;}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,ebp);ebp=MEM32(g_esp+0x130);PUSH32(g_esp,0);g_ecx=g_esp+0x14;PUSH32(g_esp,g_ecx);PUSH32(g_esp,1660);PUSH32(g_esp,ebp);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x640a7);sub_00080E14();PUSH32(g_esp,0);PUSH32(g_esp,0x640ae);sub_00080C80();g_ebx=g_eax;
 if(g_ebx!=UINT32_MAX){PUSH32(g_esp,1660);PUSH32(g_esp,ebp);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x640c1);sub_00080C94();g_edx=g_esp+0x14;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x640cc);sub_00080CE2();PUSH32(g_esp,0);g_eax=g_esp+0x14;PUSH32(g_esp,g_eax);PUSH32(g_esp,20);g_ecx=g_esp+0x20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x640e0);sub_00080E14();g_ebx=g_eax;}else g_ebx=0;
 PUSH32(g_esp,g_esi);PUSH32(g_esp,0x640ec);sub_00080EEA();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x640f2);sub_00081221();g_edx=MEM32(0x452390);g_eax=MEM32(0x452394);g_ecx=MEM32(0x452398);MEM32(g_edi)=g_edx;SET_LO16(g_edx,MEM16(0x45239c));MEM32(g_edi+4)=g_eax;PUSH32(g_esp,0);g_eax=g_esp+0x2c;PUSH32(g_esp,g_eax);MEM32(g_edi+8)=g_ecx;PUSH32(g_esp,0x45237c);MEM16(g_edi+12)=LO16(g_edx);PUSH32(g_esp,0x64127);sub_000816D2();g_eax=g_ebx?1:0;POP32(g_esp,ebp);POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=(g_eax-1)&3;
finish:g_esp+=0x11c;g_recomp_zf=!g_esp;g_esp+=4;
}
int migration_interface_profileio(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_PROFILEIO");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;uint32_t s=g_esp;if(s<0x12000||(s&3)||!profileio_span(s-8192,8200)||!profileio_span(MEM32(s+4),1660)||!profileio_span(0x45236c,128)||!profileio_span(0x4e8358,4))return 0;if(op)profileio_save();else profileio_load();return 1;}
