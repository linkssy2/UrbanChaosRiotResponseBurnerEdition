#include "migration_graphics_sdk_scalars.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003E9810(void),sub_003E7450(void),sub_003E7E40(void),sub_003E7EA0(void),sub_003E7F70(void),sub_003E6DF0(void),sub_003E7C50(void);
static int scalars_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_SDK_SCALARS");flag=v&&!strcmp(v,"1");}return flag;}
static int scalar_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define SCALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
static void scalar_grow(uint32_t pc){PUSH32(esp,pc);sub_003E9810();}
int uc_migration_graphics_sdk_scalars(uint32_t entry){
 if(!scalars_enabled())return 0;
 switch(entry){case 0x3e6950:case 0x3e7050:case 0x3e7380:case 0x3e73c0:case 0x3e74c0:case 0x3e7500:case 0x3e7710:case 0x3e7770:case 0x3e77c0:case 0x3e7820:case 0x3e78a0:case 0x3e8470:case 0x3e8580:case 0x3e85a0:case 0x3e85c0:case 0x3e85e0:case 0x3e8610:case 0x3e8640:case 0x3e86c0:case 0x3e8710:break;default:return 0;}
 uint32_t s=esp;if((s&3)||s<0x10100||!scalar_span(s-128,140)||!scalar_span(0x51c178,1)||MEM8(0x51c178)||!scalar_span(0x3f3d98,0x6b0))return 0;
 uint32_t device=MEM32(0x3f4238);if(!scalar_span(device,0x1a18)||!scalar_span(MEM32(device),128))return 0;
 if(entry==0x3e6950&&(!scalar_span(edx,16)||ecx>191||!scalar_span(MEM32(0x3f4240),28)))return 0;
 uint32_t frame=g_seh_ebp;int equal;
 if(entry==0x3e6950){PUSH32(esp,ebx);for(;;){eax=MEM32(0x3f4240)+28;if(eax<MEM32(0x3f4244))break;PUSH32(esp,edx);PUSH32(esp,ecx);scalar_grow(0x3e69ab);POP32(esp,ecx);POP32(esp,edx);}MEM32(0x3f4240)=eax;MEM32(eax-28)=0x41ea4;MEM32(eax-24)=ecx;MEM32(eax-20)=0x100b80;ecx=(ecx<<4)+0x3f3180;for(unsigned i=0;i<4;i++){ebx=MEM32(edx+i*4);MEM32(eax-16+i*4)=ebx;MEM32(ecx+i*4)=ebx;}POP32(esp,ebx);g_recomp_zf=ecx==0;esp+=4;return 1;}
 if(entry==0x3e7050){eax=MEM32(esp+4);ecx=MEM32(0x3f4238);MEM32(ecx+0x790)=eax;MEM32(0x3f3d98)|=0x4000;MEM32(0x3f41c0)=eax;esp+=8;return 1;}
 if(entry==0x3e8580||entry==0x3e85a0||entry==0x3e85c0){eax=MEM32(esp+4);MEM32(0x3f422c+(entry-0x3e8580)/8)=eax;SCALL(sub_003E7EA0,entry+14);esp+=8;return 1;}
 if(entry==0x3e85e0||entry==0x3e8610){eax=MEM32(esp+4);ecx=MEM32(0x3f4238);MEM32(entry==0x3e85e0?0x3f4208:0x3f420c)=eax;edx=MEM32(ecx+0x1a04);equal=edx==MEM32(ecx+0x1a14);if(equal==(entry==0x3e85e0)){edx=0;SCALL(sub_003E7F70,entry+36);}esp+=8;return 1;}
 if(entry==0x3e8710){eax=MEM32(esp+4);PUSH32(esp,esi);esi=MEM32(0x3f4204)<<16;PUSH32(esp,edi);edi=MEM32(0x3f4238);esi|=eax;MEM32(0x3f4218)=eax;eax=MEM32(edi+8);if((int8_t)HI8(eax)<0){eax=MEM32(0x3f4200);if(eax)esi|=1;}eax=MEM32(edi);if(eax>=MEM32(edi+4))scalar_grow(0x3e874b);MEM32(eax)=0x41d7c;MEM32(eax+4)=esi;eax+=8;MEM32(edi)=eax;POP32(esp,edi);POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;}
 if(entry==0x3e86c0){eax=MEM32(esp+4);ecx=MEM32(0x3f4218);MEM32(0x3f4204)=eax;eax<<=16;PUSH32(esp,esi);esi=MEM32(0x3f4238);eax|=ecx;PUSH32(esp,edi);edi=eax;eax=MEM32(esi+8);if((int8_t)HI8(eax)<0){eax=MEM32(0x3f4200);if(eax)edi|=1;}eax=MEM32(esi);if(eax>=MEM32(esi+4))scalar_grow(0x3e86fd);MEM32(eax)=0x41d7c;MEM32(eax+4)=edi;eax+=8;POP32(esp,edi);MEM32(esi)=eax;POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;}
 if(entry==0x3e8640){eax=MEM32(esp+4);PUSH32(esp,esi);esi=MEM32(0x3f4238);PUSH32(esp,edi);ecx=esi;MEM32(0x3f4200)=eax;SCALL(sub_003E6DF0,0x3e8658);edi=MEM32(0x3f4204);ecx=MEM32(0x3f4218);eax=MEM32(esi+8);edi=(edi<<16)|ecx;if((int8_t)HI8(eax)<0){eax=MEM32(0x3f4200);if(eax)edi|=1;}eax=MEM32(esi);if(eax>=MEM32(esi+4))scalar_grow(0x3e8688);MEM32(eax)=0x41d7c;MEM32(eax+4)=edi;ecx=MEM32(esi+4);eax+=8;equal=eax==ecx;MEM32(esi)=eax;if(eax>=ecx)scalar_grow(0x3e86a2);edx=eax;ecx=esi;SCALL(sub_003E7C50,0x3e86ab);POP32(esp,edi);MEM32(esi)=eax;POP32(esp,esi);g_recomp_zf=equal;esp+=8;return 1;}
 if(entry==0x3e77c0){eax=MEM32(esp+4);PUSH32(esp,esi);esi=MEM32(0x3f4238);MEM32(0x3f41d0)=eax;eax=MEM32(esi);int grow=eax>=MEM32(esi+4);PUSH32(esp,edi);edi=MEM32(0x3f41cc);if(grow)scalar_grow(0x3e77e3);ecx=MEM32(0x3f41d4);int active=ecx!=0;ecx=MEM32(0x3f41d0);if(!active)ecx=edi;MEM32(eax)=0x8038c;MEM32(eax+4)=edi;MEM32(eax+8)=ecx;eax+=12;MEM32(esi)=eax;MEM32(0x3f41cc)=edi;POP32(esp,edi);POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;}
 if(entry==0x3e7820){PUSH32(esp,esi);esi=MEM32(0x3f4238);eax=MEM32(esi);int grow=eax>=MEM32(esi+4);PUSH32(esp,edi);if(grow)scalar_grow(0x3e7834);ecx=MEM32(esp+12);MEM32(eax)=0x417c4;MEM32(eax+4)=ecx;eax+=8;MEM32(esi)=eax;if(ecx)MEM32(0x3f3d98)|=0x9000;esi=MEM32(0x3f4238);edi=MEM32(0x3f41cc);MEM32(0x3f41d4)=ecx;eax=MEM32(esi);if(eax>=MEM32(esi+4))scalar_grow(0x3e7872);ecx=MEM32(0x3f41d4);int active=ecx!=0;ecx=MEM32(0x3f41d0);if(!active)ecx=edi;MEM32(eax)=0x8038c;MEM32(eax+4)=edi;MEM32(eax+8)=ecx;eax+=12;MEM32(esi)=eax;MEM32(0x3f41cc)=edi;POP32(esp,edi);POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;}
 if(entry==0x3e78a0){edx=MEM32(0x3f3d98)|0x200;PUSH32(esp,esi);esi=MEM32(0x3f4238);MEM32(0x3f3d98)=edx;}
 else if(entry==0x3e8470){eax=MEM32(esp+4);PUSH32(esp,esi);esi=MEM32(0x3f4238);MEM32(0x3f4220)=eax;}
 else{PUSH32(esp,esi);esi=MEM32(0x3f4238);}
 eax=MEM32(esi);equal=eax==MEM32(esi+4);if(eax>=MEM32(esi+4)){uint32_t pc=entry==0x3e78a0?0x3e78c5:entry==0x3e8470?0x3e848c:entry+19;scalar_grow(pc);}
 if(entry==0x3e8470){edx=eax;ecx=esi;SCALL(sub_003E7E40,0x3e8495);MEM32(esi)=eax;POP32(esp,esi);g_recomp_zf=equal;esp+=8;return 1;}
 if(entry==0x3e7770){ecx=MEM32(0x3f41d4);edx=MEM32(0x3f41d0);int active=ecx!=0;ecx=MEM32(esp+8);if(!active)edx=ecx;MEM32(eax)=0x8038c;MEM32(eax+4)=ecx;MEM32(eax+8)=edx;eax+=12;MEM32(esi)=eax;MEM32(0x3f41cc)=ecx;}
 else{ecx=MEM32(esp+8);
  switch(entry){
  case 0x3e7380:MEM32(eax)=0x80320;MEM32(eax+4)=ecx;MEM32(eax+8)=ecx;eax+=12;MEM32(esi)=eax;MEM32(0x3f41fc)=ecx;break;
  case 0x3e73c0:MEM32(eax)=0x41e6c;edx=ecx-512;MEM32(eax+4)=edx;eax+=8;MEM32(esi)=eax;MEM32(0x3f4210)=ecx;break;
  case 0x3e74c0:MEM32(eax)=0x403a0;MEM32(eax+4)=ecx;eax+=8;MEM32(esi)=eax;eax=MEM32(0x3f41ec);MEM32(0x3f41e8)=ecx;POP32(esp,esi);MEM32(esp+4)=eax;g_seh_ebp=frame;sub_003E7450();return 1;
  case 0x3e7500:MEM32(eax)=0x403a4;MEM32(eax+4)=ecx;eax+=8;MEM32(esi)=eax;MEM32(0x3f3d98)|=0x200;MEM32(0x3f41d8)=ecx;POP32(esp,esi);esp+=8;return 1;
  case 0x3e7710:if(!ecx){MEM32(eax)=0x417bc;MEM32(eax+4)=ecx;eax+=8;}else{MEM32(eax)=0x817bc;MEM32(eax+4)=1;MEM32(eax+8)=ecx;eax+=12;}MEM32(esi)=eax;MEM32(0x3f41f8)=ecx;break;
  case 0x3e78a0:MEM32(eax)=0x40328;MEM32(eax+4)=ecx;eax+=8;MEM32(esi)=eax;MEM32(0x3f41c4)=ecx;break;
  }
 }
 POP32(esp,esi);g_recomp_zf=eax==0;esp+=8;return 1;
}
#undef SCALL
