#include "migration_graphics_combiner_state.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003F11E0(void),sub_003E9810(void);
static int combiner_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_COMBINER_STATE");flag=v&&!strcmp(v,"1");}return flag;}
static int combiner_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static void combiner_input(uint32_t slot,uint32_t pc){g_edx=g_ebx;g_ecx=slot;PUSH32(g_esp,pc);sub_003F11E0();}
/* Emit one authored operation. Registers remain published at every input
 * encoder call; the scratch words are the original guest-visible record. */
static int combiner_operation(uint32_t*frame){
 unsigned operation=g_eax;int final_arg=0;g_ecx=g_eax-1;
 switch(operation){
 case 1:
  if(!(g_ebx&16)){g_eax=0;MEM32(g_esp+20)=0;break;}
  g_ecx=g_ebx&32;g_eax=(g_ecx<<23)|0x4200000;*frame=0;
  if(!g_ecx){g_edi=MEM32(g_esp+24);g_esi=MEM32(g_esp+32);MEM32(g_edi)=g_eax;g_eax|=0x10000000;MEM32(g_edi+36)=g_edx;MEM32(g_edi+72)=g_eax;MEM32(g_edi+108)=g_edx;g_edi+=4;g_esi++;return 1;}break;
 case 2:g_edi=g_esi;combiner_input(0x30000,0x3f1396);g_eax|=0x200000;break;
 case 3:g_edi=*frame;combiner_input(0,0x3f13ab);g_eax|=0x2000;break;
 case 4:case 5:case 6:
  if(operation!=4){g_eax=g_edx|(operation==5?0x10000:0x20000);MEM32(g_esp+20)=g_eax;}
  g_edi=g_esi;combiner_input(0x30000,0x3f13d7);g_esi=g_eax;g_ecx=0x20000;final_arg=1;break;
 case 7:case 8:case 9:case 10:
  if(operation==9){g_eax=g_edx|0x18000;MEM32(g_esp+20)=g_eax;}
  if(operation==8||operation==9)MEM32(g_esp+20)|=0x8000;
  g_edi=g_esi;combiner_input(0x30000,operation==10?0x3f141f:0x3f1404);g_esi=g_eax|(operation==10?0x204000:0x202000);g_ecx=0;final_arg=1;break;
 case 11:case 18:case 20:
  g_edi=g_esi;combiner_input(0x30000,operation==11?0x3f143a:operation==18?0x3f14f8:0x3f152b);g_eax|=0x200000;MEM32(g_esp+16)=g_eax;
  combiner_input(operation==11?0x10010:operation==18?0x10020:0x10030,operation==11?0x3f144f:operation==18?0x3f150d:0x3f1540);g_esi=MEM32(g_esp+16)|g_eax;g_ecx=0;final_arg=1;break;
 case 12:case 13:case 14:case 15:
  g_eax-=12;g_edi=g_esi;MEM32(g_esp+16)=g_eax;combiner_input(0x30000,0x3f146d);g_edi=MEM32(g_esp+16);g_esi=g_eax;combiner_input(0x20020,0x3f147f);g_ecx=0x10030;g_esi|=g_eax;g_edx=g_ebx;PUSH32(g_esp,0x3f1657);sub_003F11E0();g_esi|=g_eax;g_ecx=0;final_arg=1;break;
 case 16:
  g_edi=g_esi;combiner_input(0x30000,0x3f1497);g_esi=g_eax|0x200000;g_edi=2;combiner_input(0x10030,0x3f1657);g_esi|=g_eax;g_ecx=0;final_arg=1;break;
 case 17:
  g_edi=g_esi;combiner_input(0x30000,0x3f14bc);g_esi=g_eax;
  if(g_ebx&16){g_edi=2;combiner_input(0x20000,0x3f14d4);g_esi|=g_eax;}else g_esi|=0x200000;g_eax=g_esi;break;
 case 19:case 21:
  g_edi=g_esi;combiner_input(operation==19?0x30000:0x30010,0x3f1557);g_edi=*frame;MEM32(g_esp+16)=g_eax;combiner_input(0x20000,0x3f1569);*frame=MEM32(g_esp+16)|g_eax;g_edi=g_esi;combiner_input(0x10020,0x3f157d);g_eax|=*frame;g_eax|=32;break;
 case 22:
  g_edi=g_esi;combiner_input(0x30040,0x3f15c3);g_edi=*frame;g_esi=g_eax;combiner_input(0x20040,0x3f15d3);g_ecx=(MEM32(g_esp+44)|0x820000)>>4;g_edx=0;MEM32(g_esp+20)=g_ecx;g_ecx=MEM32(g_esp+24);g_eax|=g_esi;MEM32(g_ecx+72)=0;MEM32(g_ecx+108)=0;g_ebx|=128;break;
 case 23:
  combiner_input(0x30000,0x3f1606);g_eax|=0x200000;g_edi=g_esi;MEM32(g_esp+16)=g_eax;combiner_input(0x10000,0x3f161d);g_esi=MEM32(g_esp+16)|g_eax;g_ecx=0;final_arg=1;break;
 case 24:
  combiner_input(0x30000,0x3f162f);g_edi=g_esi;MEM32(g_esp+48)=g_eax;combiner_input(0x20000,0x3f1641);g_esi=MEM32(g_esp+48);g_edi=MEM32(g_esp+16);g_ecx=0x10010;g_esi|=g_eax;g_edx=g_ebx;PUSH32(g_esp,0x3f1657);sub_003F11E0();g_esi|=g_eax;g_ecx=0;final_arg=1;break;
 case 25:case 26:
  g_edi=1;combiner_input(0x30000,0x3f1598);g_ecx=MEM32(g_esp+204);g_edx=MEM32(g_ecx+8);g_eax|=0x200000;g_edx|=0x10000;MEM32(g_ecx+8)=g_edx;break;
 }
 if(final_arg){g_edx=g_ebx;g_edi=*frame;PUSH32(g_esp,0x3f1664);sub_003F11E0();g_eax|=g_esi;}
 return 0;
}
int uc_migration_graphics_combiner_state(uint32_t entry){
 if(!combiner_enabled()||entry!=0x3f1280)return 0;uint32_t s=g_esp;
 if((s&3)||s<0x10200||!combiner_span(s-256,272)||!combiner_span(0x51c178,1)||MEM8(0x51c178)||!combiner_span(MEM32(s+4),0x788))return 0;
 static const uint32_t targets[]={0x3f133e,0x3f1388,0x3f13a0,0x3f13c9,0x3f13be,0x3f13b5,0x3f13f6,0x3f13ee,0x3f13e3,0x3f1411,0x3f142c,0x3f1458,0x3f1458,0x3f1458,0x3f1458,0x3f1489,0x3f14ae,0x3f14ea,0x3f1516,0x3f151d,0x3f1549,0x3f15b5,0x3f15fa,0x3f1623,0x3f1587,0x3f1587};
 if(!MEM32(MEM32(s+4)+0x784))for(unsigned stage=0;stage<4;stage++)for(unsigned part=0;part<2;part++){
  uint32_t p=0x3f3dd0+128*stage+16*part,op=MEM32(p);if(op<1||op>26||MEM32(0x3f17d8+4*(op-1))!=targets[op-1])return 0;for(unsigned arg=1;arg<4;arg++)if((MEM32(p+4*arg)&15)>5)return 0;
 }
 uint32_t frame=g_seh_ebp;g_ecx=MEM32(s+4);g_eax=MEM32(g_ecx+0x784);g_esp-=184;
 if(g_eax){g_eax=MEM32(g_esp+192);g_esp+=184;g_recomp_zf=g_esp==0;g_esp+=12;return 1;}
 g_eax=MEM32(g_ecx+8);MEM32(g_esp+36)=g_eax;g_eax&=0xfffeffbf;MEM32(g_ecx+8)=g_eax;g_ecx=MEM32(0x3f4178);g_eax=0;MEM32(g_esp+12)=0;if(g_ecx){MEM32(g_esp+12)=3;g_eax=MEM32(g_esp+12);}
 PUSH32(g_esp,g_ebx);g_ebx=g_eax;g_ecx=g_esp+48;MEM32(g_esp+20)=g_eax;g_ebx|=16;g_eax<<=7;PUSH32(g_esp,frame);MEM32(g_esp+16)=g_ecx;g_ecx=g_eax+0x3f3da0;g_eax=MEM32(g_ecx+48);PUSH32(g_esp,g_esi);MEM32(g_esp+32)=g_ecx;MEM32(g_esp+36)=g_ecx;PUSH32(g_esp,g_edi);
 int finished=0;
 while(!finished){
  g_ecx=MEM32(g_esp+36);g_edi=MEM32(g_ecx+52);g_esi=MEM32(g_ecx+56);frame=MEM32(g_ecx+60);MEM32(g_esp+16)=g_edi;g_edx=MEM32(g_ecx+80)==5?0xd00:0xc00;MEM32(g_esp+44)=g_edx;
  for(;;){
   MEM32(g_esp+20)=g_edx;if(combiner_operation(&frame)){finished=1;break;}
   g_edx=g_eax&0xff000000;if(g_edx==0xff000000){g_eax=(((~g_ebx)&16)<<23)|0x4200000;g_ecx=(g_ebx&32)<<23;g_eax|=g_ecx;}
   g_edi=MEM32(g_esp+24);g_edx=MEM32(g_esp+20);g_ecx=g_ebx&0x48;MEM32(g_ecx+g_edi)=g_eax;MEM32(g_ecx+g_edi+36)=g_edx;
   if(g_ebx&128)break;
   g_eax=MEM32(g_esp+36);g_ecx=MEM32(g_eax+68);g_esi=MEM32(g_eax+72);frame=MEM32(g_eax+76);g_eax=MEM32(g_eax+64);g_edx=MEM32(g_esp+44);g_ebx|=0xe8;MEM32(g_esp+16)=g_ecx;g_edi=g_ecx;
  }
  if(finished)break;
  g_esi=MEM32(g_esp+32);g_eax=MEM32(g_esp+40);g_edi+=4;g_esi++;g_eax+=128;MEM32(g_esp+24)=g_edi;MEM32(g_esp+32)=g_esi;MEM32(g_esp+40)=g_eax;
  if(g_esi==4){frame=0;break;}MEM32(g_esp+36)=g_eax;g_eax=MEM32(g_eax+48);g_ebx=g_esi;if(g_eax==1){frame=0;break;}
 }
 g_esi-=MEM32(g_esp+28);g_eax=8-g_esi;
 do{MEM32(g_edi)=frame;MEM32(g_edi+36)=frame;MEM32(g_edi+72)=frame;MEM32(g_edi+108)=frame;g_edi+=4;g_eax--;}while(g_eax);
 g_ecx=MEM32(g_esp+204);g_eax=MEM32(g_ecx);MEM32(g_esp+56)=0x200ac0;MEM32(g_esp+92)=0x201e40;MEM32(g_esp+128)=0x200260;MEM32(g_esp+164)=0x200aa0;if(g_eax>=MEM32(g_ecx+4)){PUSH32(g_esp,0x3f175a);sub_003E9810();}
 MEM32(g_eax)=0x41e60;MEM32(g_eax+4)=g_esi;g_edi=g_eax+8;g_ecx=36;g_esi=g_esp+56;memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),144);g_esi+=144;g_edi+=144;g_ecx=0;g_esi=MEM32(g_esp+204);g_edi=MEM32(g_esp+52);g_ecx=g_eax+152;g_eax=MEM32(g_esi+8);g_edx=g_eax^g_edi;
 if(g_edx&64){g_edx=MEM32(0x3f413c);if(!g_edx){g_eax=(g_eax>>6)&1;MEM32(g_ecx)=0x403b8;MEM32(g_ecx+4)=g_eax;g_ecx+=8;}}
 g_eax=MEM32(g_esi+8)|g_edi;MEM32(g_esi)=g_ecx;if(g_eax&0x10000)MEM32(g_esp+208)|=0x400f;g_eax=MEM32(g_esp+208);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=184;g_recomp_zf=g_esp==0;g_esp+=12;return 1;
}
