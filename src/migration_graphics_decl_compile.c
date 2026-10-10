#include "migration_graphics_decl_compile.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int decl_compile_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_DECL_COMPILE");flag=v&&!strcmp(v,"1");}return flag;}
static int decl_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int decl_apart(uint32_t a,uint64_t n,uint32_t b,uint64_t m){return(uint64_t)a+n<=b||(uint64_t)b+m<=a;}
int uc_migration_graphics_decl_compile(uint32_t entry){
 if(!decl_compile_enabled()||(entry!=0x3e6550&&entry!=0x3e66f0))return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!decl_span(s-128,144)||!decl_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t input=MEM32(s+(entry==0x3e6550?8:4)),output=MEM32(s+(entry==0x3e6550?4:8)),cursor=input;uint64_t output_bytes=entry==0x3e6550?0x118:0;int ended=0;
 /* Asset creation boundary: preflight once before any guest writes. */
 for(unsigned i=0;i<4096;i++){
  if(!decl_span(cursor,4))return 0;uint32_t token=MEM32(cursor),kind=token&0xe0000000;cursor+=4;
  if((entry==0x3e6550&&kind>0x80000000)||token==0xffffffff){ended=1;break;}
  if(entry==0x3e6550&&(kind==0x40000000||kind==0x60000000)&&!(token&0x10000000)&&(token&31)>15)return 0;
  if(entry==0x3e6550&&kind==0x60000000&&(token&31)>15)return 0;
  if(kind==0x80000000){unsigned n=(token>>25)&15;if(!decl_span(cursor,n*16))return 0;cursor+=n*16;if(entry==0x3e66f0)output_bytes+=(uint64_t)n*16+((n+7)/8?((n+7)/8):1)*4+8;}
 }
 if(!ended||!decl_apart(input,cursor-input,s-128,144)||(output&&(!decl_span(output,output_bytes)||!decl_apart(output,output_bytes,s-128,144)||!decl_apart(output,output_bytes,input,cursor-input)))||(entry==0x3e6550&&!output))return 0;
 uint32_t frame=g_seh_ebp;
 if(entry==0x3e6550){
  PUSH32(esp,ecx);PUSH32(esp,ebx);ebx=MEM32(esp+16);PUSH32(esp,frame);PUSH32(esp,esi);frame=0;PUSH32(esp,edi);edi=MEM32(esp+24);MEM32(esp+16)=frame;
  for(;;){eax=MEM32(ebx);ecx=eax&0xe0000000;ebx+=4;MEM32(esp+24)=eax;
   if(ecx==0)continue;
   if(ecx==0x20000000){eax&=15;MEM32(esp+16)=eax;frame=0;continue;}
   if(ecx==0x40000000){
    if(eax&0x10000000){ecx=(eax>>16)&15;if(!(eax&0x8000000))ecx<<=2;frame+=ecx;continue;}
    esi=MEM8(esp+26);edx=MEM32(esp+16);eax&=31;ecx=(eax<<4)+edi;MEM32(ecx+24)=frame;MEM32(ecx+20)=edx;MEM32(ecx+28)=esi;ecx=(eax+2)<<4;MEM8(ecx+edi)=0;ecx=MEM32(esp+32);
    if(ecx){if(eax>=9&&eax<=12){edx=(esi>>4)&15;if(edx==7)edx=4;else if(esi==0x16)edx=3;ecx=eax*8-72;edx<<=LO8(ecx);MEM32(edi+16)|=edx;}
     if(eax==3){eax=MEM32(edi+4)|0x400;MEM32(edi+4)=eax;}else if(eax==4){eax=MEM32(edi+4)|0x800;MEM32(edi+4)=eax;}else if(eax==7){eax=MEM32(edi+4)|0x1000;MEM32(edi+4)=eax;}else if(eax==8){eax=MEM32(edi+4)|0x2000;MEM32(edi+4)=eax;}}
    edx=(esi>>4)&15;eax=MEM8(edx+0x3f1ff4);esi&=15;ecx=MEM8(esi+0x3f1fec);eax=(uint32_t)((int32_t)eax*(int32_t)ecx);frame+=eax;continue;
   }
   if(ecx==0x60000000){esi=eax&31;edx=esi<<4;ecx=edx+edi;edx=0;MEM32(ecx+24)=edx;MEM32(ecx+20)=edx;edx=eax>>20;esi+=2;SET_LO8(edx,LO8(edx)&15);esi<<=4;MEM32(ecx+28)=2;MEM8(ecx+33)=LO8(edx);MEM8(esi+edi)=eax&0x10000000?2:1;continue;}
   if(ecx==0x80000000){eax=((eax>>25)&15)<<4;ebx+=eax;continue;}
   break;
  }
  POP32(esp,edi);POP32(esp,esi);POP32(esp,frame);POP32(esp,ebx);POP32(esp,ecx);g_recomp_zf=0;esp+=16;return 1;
 }
 esp-=8;PUSH32(esp,frame);frame=MEM32(esp+20);PUSH32(esp,esi);esi=MEM32(esp+20);ecx=MEM32(esi);eax=0;esi+=4;PUSH32(esp,edi);edi=0xdeadbeef;MEM32(esp+28)=eax;MEM32(esp+24)=esi;
 if(ecx!=0xffffffff){PUSH32(esp,ebx);for(;;){edx=ecx&0xe0000000;
  if(edx==0x80000000){edx=(ecx>>25)&15;ecx&=255;edi++;MEM32(esp+16)=edx;if(ecx!=edi){if(frame){MEM32(frame)=0x41ea4;MEM32(frame+4)=ecx;frame+=8;}eax+=2;MEM32(esp+32)=eax;}MEM32(esp+20)=ecx;
   do{ebx=edx<8?edx:8;
    if(frame){eax=(ebx<<20)+0xb80;MEM32(frame)=eax;edx=ebx<<4;ecx=edx;eax=ecx;ecx>>=2;frame+=4;edi=frame;memcpy((void*)XBOX_PTR(edi),(const void*)XBOX_PTR(esi),ecx*4);esi+=ecx*4;edi+=ecx*4;ecx=0;ecx=eax;eax=MEM32(esp+32);ecx&=3;memcpy((void*)XBOX_PTR(edi),(const void*)XBOX_PTR(esi),ecx);esi+=ecx;edi+=ecx;ecx=0;esi=MEM32(esp+28);frame+=edx;edx=MEM32(esp+16);}
    ecx=ebx<<4;esi+=ecx;eax+=ebx*4+1;edx-=ebx;MEM32(esp+28)=esi;MEM32(esp+32)=eax;MEM32(esp+16)=edx;
   }while(edx);edi=MEM32(esp+20);
  }
  ecx=MEM32(esi);esi+=4;MEM32(esp+28)=esi;if(ecx==0xffffffff)break;
 }POP32(esp,ebx);}
 POP32(esp,edi);POP32(esp,esi);POP32(esp,frame);esp+=8;g_recomp_zf=esp==0;esp+=12;return 1;
}
