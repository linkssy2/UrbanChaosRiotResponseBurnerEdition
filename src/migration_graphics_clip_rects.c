#include "migration_graphics_clip_rects.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_003EAA50(void),sub_003E9810(void);
static int clip_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_CLIP_RECTS");flag=v&&!strcmp(v,"1");}return flag;}
static int clip_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define TOP g_fp_stack[g_fp_top]
#define CALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
static void clip_push(double x){g_fp_top=(g_fp_top+7)&7;TOP=x;}
static void clip_round(uint32_t pc){TOP+=MEMF(0x44c1f8);MEMF(esp)=(float)TOP;g_fp_top=(g_fp_top+1)&7;CALL(sub_003EAA50,pc);}
int uc_migration_graphics_clip_rects(uint32_t entry){
 if(entry!=0x3e3cd0||!clip_enabled())return 0;uint32_t s=esp;
 if((s&3)||s<0x10100||!clip_span(s-128,144)||!clip_span(0x51c178,1)||MEM8(0x51c178)||g_fp_top<0||g_fp_top>7)return 0;
 uint32_t d=MEM32(0x3f4238),n=MEM32(s+4),rect=MEM32(s+12);
 if(n>8||!clip_span(d,0x1c08)||!clip_span(MEM32(d),148)||!clip_span(rect,n*16))return 0;
 if(n&&((uint64_t)rect<d+0x1c08&&(uint64_t)rect+n*16>d))return 0;
 if(n&&((uint64_t)rect<s+16&&(uint64_t)rect+n*16>s-128))return 0;
 uint32_t frame=g_seh_ebp;esp-=20;PUSH32(esp,ebx);PUSH32(esp,frame);frame=MEM32(0x3f4238);eax=MEM32(frame+0x95c);ecx=MEM32(frame+0x960);PUSH32(esp,esi);PUSH32(esp,edi);edi=MEM32(frame);MEM32(esp+16)=eax;MEM32(esp+20)=ecx;
 if(edi>=MEM32(frame+4)){CALL(sub_003E9810,0x3e3cfe);edi=eax;}
 edx=MEM32(esp+40);
 if(!edx){
  static const uint32_t pcs[]={0x3e3d35,0x3e3d60,0x3e3d8d,0x3e3db8};
  for(unsigned i=0;i<4;i++){
   uint32_t value=MEM32(frame+0xee0+i*4);
   if(i==0)edx=value;
   clip_push((double)(int32_t)value);
   if(i==1){esi=eax;eax=value;}if(i==2){ecx=value;MEM32(esp+24)=eax;}if(i==3){edx=value;ebx=eax;}
   if((int32_t)value<0)TOP+=MEMF(0x44c510);
   TOP*=MEMF(esp+16+(i&1)*4);PUSH32(esp,ecx);clip_round(pcs[i]);
  }
 }else if(edx==1&&!MEM32(esp+44)){
  eax=MEM32(esp+44);ebx=MEM32(esp+48);clip_push((double)SMEM32(ebx));PUSH32(esp,ecx);TOP*=MEMF(esp+20);clip_round(0x3e3deb);
  clip_push((double)SMEM32(ebx+4));PUSH32(esp,ecx);esi=eax;TOP*=MEMF(esp+24);clip_round(0x3e3e03);
  clip_push((double)SMEM32(ebx+8));PUSH32(esp,ecx);MEM32(esp+28)=eax;TOP*=MEMF(esp+20);clip_round(0x3e3e1d);
  ebx=eax;eax=MEM32(esp+48);clip_push((double)SMEM32(eax+12));PUSH32(esp,ecx);ebx-=esi;TOP*=MEMF(esp+24);clip_round(0x3e3e3b);eax-=MEM32(esp+24);
 }else{
  if(edx==1)eax=MEM32(esp+44);
  ecx=MEM32(frame+0x954);eax=MEM32(frame+0x958);ecx<<=16;MEM32(edi)=0x80200;MEM32(edi+4)=ecx;ecx=MEM32(esp+44);eax<<=16;MEM32(edi+8)=eax;MEM32(edi+12)=0x402b4;MEM32(edi+16)=ecx;ebx=edi+20;
  if(edx){eax=MEM32(esp+48);ecx=MEM32(esp+40);edx=ebx-edi;esi=0x402e0;edi=edx+eax-16;MEM32(esp+24)=ecx;
   do{
    clip_push((double)SMEM32(edi-4));PUSH32(esp,ecx);TOP*=MEMF(esp+20);MEMF(esp+32)=(float)TOP;g_fp_top=(g_fp_top+1)&7;
    clip_push((double)SMEM32(edi+4));TOP*=MEMF(esp+20);clip_round(0x3e3f06);
    clip_push(MEMF(esp+28));PUSH32(esp,ecx);MEM32(esp+36)=eax;clip_round(0x3e3f1d);
    edx=(MEM32(esp+32)-1)<<16;ecx=esi-32;MEM32(ebx)=ecx;eax|=edx;MEM32(ebx+4)=eax;
    clip_push((double)SMEM32(edi));PUSH32(esp,ecx);TOP*=MEMF(esp+24);MEMF(esp+36)=(float)TOP;g_fp_top=(g_fp_top+1)&7;
    clip_push((double)SMEM32(edi+8));TOP*=MEMF(esp+24);clip_round(0x3e3f4f);
    clip_push(MEMF(esp+32));PUSH32(esp,ecx);MEM32(esp+32)=eax;clip_round(0x3e3f66);
    edx=(MEM32(esp+28)-1)<<16;eax|=edx;MEM32(ebx+8)=esi;MEM32(ebx+12)=eax;eax=MEM32(esp+24);ebx+=16;edi+=16;esi+=4;eax--;MEM32(esp+24)=eax;
   }while(eax);
  }
  goto publish;
 }
 MEM32(edi)=0x80200;ebx=(ebx<<16)|esi;esi=MEM32(esp+24);MEM32(edi+4)=ebx;eax=(eax<<16)|esi;MEM32(edi+8)=eax;MEM32(edi+12)=0x402b4;MEM32(edi+16)=0;eax=MEM32(frame+0x954)<<16;MEM32(edi+20)=0x402c0;MEM32(edi+24)=eax;eax=MEM32(frame+0x958)<<16;MEM32(edi+28)=0x402e0;MEM32(edi+32)=eax;ebx=edi+36;
publish:
 edx=MEM32(esp+40);esi=MEM32(esp+48);ecx=edx;edx=MEM32(esp+44);ecx<<=4;eax=ecx;ecx>>=2;MEM32(frame)=ebx;edi=frame+0x1b80;
 memcpy((void*)XBOX_PTR(edi),(const void*)XBOX_PTR(esi),ecx*4);esi+=ecx*4;edi+=ecx*4;ecx=0;ecx=eax&3;
 memcpy((void*)XBOX_PTR(edi),(const void*)XBOX_PTR(esi),ecx);esi+=ecx;edi+=ecx;ecx=MEM32(esp+40);POP32(esp,edi);POP32(esp,esi);MEM32(frame+0x1c00)=ecx;MEM32(frame+0x1c04)=edx;POP32(esp,frame);POP32(esp,ebx);esp+=20;g_recomp_zf=esp==0;esp+=16;return 1;
}
#undef CALL
#undef TOP
