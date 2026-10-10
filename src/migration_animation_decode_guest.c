#include "migration_animation_decode_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int decode_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int decode_separate(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int decode(unsigned wide){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_DECODE");enabled=v&&!strcmp(v,"1");}
 uint32_t s=g_esp,p=g_ecx,width=wide?6:4;
 if(!enabled||(s&3)||s<0x1001c||g_fp_top<0||g_fp_top>7||!decode_span(s-28,36)||!decode_span(p,width)||!decode_separate(p,width,s-28,36)||!decode_span(0x51c178,1)||MEM8(0x51c178))return 0;
 uint32_t out=MEM32(s+4);if(!decode_span(out,16)||!decode_separate(out,16,s-28,36))return 0;
 const uint32_t constants[]={wide?0x47faf4:0x47faf0,0x47fad0,0x44bf94,0x51c178};
 for(unsigned i=0;i<4;i++)if(!decode_span(constants[i],4)||!decode_separate(constants[i],4,out,16)||!decode_separate(constants[i],4,s-28,36))return 0;
 uint64_t packed=MEM32(p);if(wide)packed|=(uint64_t)MEM16(p+4)<<32;
 unsigned hole=(unsigned)packed&3,bits=wide?15:10,slot=((unsigned)g_fp_top+7)&7,second=((unsigned)g_fp_top+6)&7;packed>>=2;
 /* The wide decoder writes six bytes into an eight-byte temporary. Retain
  * its stale high word when component 2 is omitted; other holes overwrite it. */
 if(wide){MEM16(s-24)=MEM16(p);MEM16(s-22)=MEM16(p+2);MEM16(s-20)=MEM16(p+4);}
 MEM32(s-28)=g_esi;g_xmm0=XMM_ZERO();
 for(unsigned component=0;component<4;component++){
  uint32_t dst=s-16+component*4;
  if(component==hole){MEM32(dst)=0;continue;}
  uint32_t quant=(uint32_t)packed&((1u<<bits)-1);packed>>=bits;MEM32(s-24)=quant;
  g_fp_stack[slot]=(double)quant;g_fp_stack[slot]*=MEMF(constants[0]);g_fp_stack[slot]-=MEMF(0x47fad0);MEMF(dst)=(float)g_fp_stack[slot];
  if(component==2)MEM32(s-20)=0;
 }
 /* Original x87 model rounds each decoded lane to float before its ordered
  * double-precision square/sum/root, retaining both retired FP stack slots. */
 g_fp_stack[slot]=(double)MEMF(s-4);g_fp_stack[slot]*=MEMF(s-4);
 for(int i=2;i>=0;--i){g_fp_stack[second]=(double)MEMF(s-16+i*4);g_fp_stack[second]*=MEMF(s-16+i*4);g_fp_stack[slot]+=g_fp_stack[second];}
 g_fp_stack[slot]=MEMF(0x44bf94)-g_fp_stack[slot];g_fp_stack[slot]=sqrt(g_fp_stack[slot]);MEMF(s-16+hole*4)=(float)g_fp_stack[slot];
 g_eax=g_ecx=out;for(unsigned i=0;i<4;i++){g_edx=MEM32(s-16+i*4);MEM32(out+i*4)=g_edx;}
 g_recomp_zf=s==0;g_esp=s+8;return 1;
}
int uc_migration_animation_decode32(void){return decode(0);}
int uc_migration_animation_decode48(void){return decode(1);}
