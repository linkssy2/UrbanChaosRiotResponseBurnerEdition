#include "migration_graphics_model_base.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_000BAC00(void),sub_000B5450(void),sub_000B51F0(void),sub_000B51D0(void),sub_000B5200(void),sub_000B5210(void),sub_000BABB0(void),sub_003E6AE0(void);
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
extern void uc_hybrid_models_base_begin(uint32_t,uint32_t,uint32_t,uint32_t),uc_hybrid_models_base_end(void);
#endif
static int model_base_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_MODEL_BASE");flag=v&&!strcmp(v,"1");}return flag;}
static int model_base_span(uint32_t p,uint64_t n){return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
#define BASE_CALL(fn,pc) do{PUSH32(esp,pc);fn();}while(0)
#define BASE_INDIRECT(target,pc,saved) do{uint32_t call_target=(target);PUSH32(esp,pc);RECOMP_ICALL_SAFE(call_target,saved);}while(0)
static void model_base_stream(uint32_t stream,uint32_t data,uint32_t stride,uint32_t pc){PUSH32(esp,stride);PUSH32(esp,data);PUSH32(esp,stream);BASE_CALL(sub_003E6AE0,pc);}
int uc_migration_graphics_model_base(uint32_t entry){
 if(entry!=0xbb5c0||!model_base_enabled())return 0;uint32_t s=esp;
 if((s&3)||s<0x10200||!model_base_span(s-256,264)||!model_base_span(0x51c178,1)||MEM8(0x51c178)||!model_base_span(ecx,0x164))return 0;
 uint32_t index=MEM32(s+4);if(index!=0xffffffff&&index>4096)return 0;
 uint32_t frame=g_seh_ebp;PUSH32(esp,0xffffffff);PUSH32(esp,0x38d4b8);eax=MEM32(0);PUSH32(esp,eax);MEM32(0)=esp;esp-=12;PUSH32(esp,ebx);PUSH32(esp,edi);edi=MEM32(esp+36);ebx=ecx;MEM32(esp+8)=ebx;
 int render=edi!=0;
 if(!render){SET_LO8(eax,MEM8(ebx+0xb3));render=LO8(eax)!=0;if(!render){eax=MEM32(ebx+0xb4);if(eax){ecx=MEM32(0x519f60);eax=MEM32(ecx);uint32_t saved=esp;BASE_INDIRECT(MEM32(eax+0xc8),0xbb60a,saved);ecx=MEM32(ebx+0xb4);edx=MEM32(eax);saved=esp;PUSH32(esp,ecx);ecx=eax;BASE_INDIRECT(MEM32(edx+12),0xbb618,saved);render=eax!=0;}}}
 if(!render){eax=MEM32(0x519e8c)+1;POP32(esp,edi);MEM32(0x519e8c)=eax;POP32(esp,ebx);ecx=MEM32(esp+12);MEM32(0)=ecx;esp+=24;g_recomp_zf=esp==0;esp+=8;return 1;}
 PUSH32(esp,frame);PUSH32(esp,esi);ecx=ebx;BASE_CALL(sub_000BAC00,0xbb643);ecx=ebx;frame=eax;BASE_CALL(sub_000B5450,0xbb64c);ecx=frame;esi=eax;BASE_CALL(sub_000B51F0,0xbb655);ecx=frame;BASE_CALL(sub_000B51F0,0xbb65c);model_base_stream(1,eax,4,0xbb666);
 if(edi&&edi!=0xffffffff){
  eax=MEM32(esi+0x40);edx=edi+edi*4-5;edi=MEM32(esi+0x3c)?10:16;ecx=frame;ebx=eax+edx*4;BASE_CALL(sub_000B51D0,0xbb695);model_base_stream(0,eax,edi,0xbb69e);ecx=frame;BASE_CALL(sub_000B5200,0xbb6a5);model_base_stream(2,eax,8,0xbb6af);esi=MEM32(esi+0x3c);model_base_stream(4,esi,8,0xbb6bc);ecx=frame;BASE_CALL(sub_000B5210,0xbb6c3);model_base_stream(5,eax,8,0xbb6cd);
  ecx=MEM16(ebx);eax=MEM32(esp+16);edx=MEM32(eax+0x68);edx=MEM32(edx+0x34);ecx=MEM32(edx+ecx*4);edx=ebx+2;MEM32(esp+20)=0x457af0;MEM32(esp+24)=edx;eax=MEM32(eax+0x160);MEM32(esp+36)=0;
  uint32_t pc;if(eax){edx=MEM16(ebx);eax=MEM32(eax+edx*4);edx=MEM32(ecx);pc=0xbb70f;}else{edx=MEM32(ecx);eax=0;pc=0xbb731;}
  uint32_t saved=esp;PUSH32(esp,eax);eax=esp+24;PUSH32(esp,eax);BASE_INDIRECT(MEM32(edx+24),pc,saved);
 }else{
  eax=MEM32(esi+12);MEM8(esp+44)=edi!=0;
  if(eax){edi=MEM32(esi+0x3c)?10:16;ecx=frame;BASE_CALL(sub_000B51D0,0xbb76c);model_base_stream(0,eax,edi,0xbb775);ecx=frame;BASE_CALL(sub_000B5200,0xbb77c);model_base_stream(2,eax,8,0xbb786);ecx=MEM32(esi+0x3c);model_base_stream(4,ecx,8,0xbb793);ecx=frame;BASE_CALL(sub_000B5210,0xbb79a);model_base_stream(5,eax,8,0xbb7a4);eax=MEM32(esi+12);ecx=0;MEM32(esp+20)=eax;MEM32(esp+16)=ecx;
   if((int32_t)eax>0){frame=0;do{
    edx=MEM32(esi+0x24);eax=edx+frame;
    if(MEM16(edx+frame+6)==0xffff&&(MEM8(eax+8)&2)){
     eax=MEM32(esi+0x38);if(eax){eax+=ecx*8;PUSH32(esp,eax);ecx=ebx;BASE_CALL(sub_000BABB0,0xbb7e7);}
     ecx=MEM32(esi+0x24);edx=MEM16(ecx+frame+4);eax=MEM32(ebx+0x68);ecx=MEM32(eax+0x34);edi=MEM32(ecx+edx*4);edx=MEM32(edi);ecx=edi;uint32_t saved=esp;BASE_INDIRECT(MEM32(edx+44),0xbb7ff,saved);int pass=(LO8(eax)&1)!=0;SET_LO8(eax,MEM8(esp+44));
     if(pass==(LO8(eax)!=0)){eax=MEM32(ebx+0x160);if(eax){ecx=MEM32(esi+0x24);edx=MEM16(ecx+frame+4);eax=MEM32(eax+edx*4);}else eax=0;
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
      uc_hybrid_models_base_begin(ebx,esi,MEM32(esp+16),eax);
#endif
      edx=MEM32(edi);saved=esp;PUSH32(esp,eax);eax=MEM32(esi+0x24)+frame;ecx=edi;PUSH32(esp,eax);BASE_INDIRECT(MEM32(edx+24),0xbb838,saved);
     }
    }
#if defined(UCRR_HYBRID_EXTENDED) && UCRR_HYBRID_EXTENDED
    uc_hybrid_models_base_end();
#endif
    ecx=MEM32(esp+16);eax=MEM32(esp+20);ecx++;frame+=33;MEM32(esp+16)=ecx;
   }while((int32_t)ecx<(int32_t)eax);}
  }
 }
 POP32(esp,esi);POP32(esp,frame);POP32(esp,edi);POP32(esp,ebx);ecx=MEM32(esp+12);MEM32(0)=ecx;esp+=24;g_recomp_zf=esp==0;esp+=8;return 1;
}
#undef BASE_INDIRECT
#undef BASE_CALL
