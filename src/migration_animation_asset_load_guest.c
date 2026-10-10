#include "migration_animation_asset_load_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_001F0B20(void),sub_001F0B40(void),sub_001F0B60(void),sub_001F0B80(void);
extern void sub_001F4A80(void),sub_001F35D0(void),sub_001F24B0(void),sub_001F2520(void),sub_001F2B90(void);
static int load_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int load_gate(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_ANIMATION_ASSET_LOAD");enabled=v&&!strcmp(v,"1");}
 return enabled&&!(g_esp&3)&&g_esp>=0x10200&&load_span(g_esp-512,520)&&load_span(0x51c178,1)&&!MEM8(0x51c178);
}
int uc_migration_animation_asset_times(void){
 if(!load_gate())return 0;g_eax=MEM32(g_esp+4);g_ecx=g_eax*4;MEM32(g_esp+4)=g_ecx;sub_001F4A80();return 1;
}
#define LOAD_CALL(fn,pc) do{PUSH32(g_esp,(pc));fn();}while(0)
#define LOAD_READ(fn,pc) do{g_ecx=g_ebx;LOAD_CALL(fn,pc);}while(0)
/* Asset publication follows the source stream and synchronous allocator order.
 * Once reads begin, valid authored input and allocation/callback lifetime are
 * inherited from those services; committed work is never replayed. */
int uc_migration_animation_asset_load(void){
 uint32_t s=g_esp,p=g_ecx;
 if(!load_gate()||!load_span(p,120)||!((uint64_t)p+120<=s-512||(uint64_t)s+8<=p)||!load_span(MEM32(s+4),4))return 0;
 uint32_t frame=g_seh_ebp;g_esp-=28;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+36);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_eax=g_esp+40;g_esi=g_ecx;
 PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f3609);
 g_ecx=g_esi+68;PUSH32(g_esp,g_ecx);LOAD_READ(sub_001F0B60,0x1f3614);
 g_edx=g_esi+52;PUSH32(g_esp,g_edx);LOAD_READ(sub_001F0B60,0x1f361f);
 g_eax=g_esi+56;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f362a);
 g_ecx=g_esi+60;PUSH32(g_esp,g_ecx);LOAD_READ(sub_001F0B60,0x1f3635);
 g_eax=g_esi+64;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f3640);
 g_eax=g_esi+72;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B80,0x1f364b);
 g_edi=g_esi+20;PUSH32(g_esp,g_edi);LOAD_READ(sub_001F0B60,0x1f3656);
 frame=g_esi+28;PUSH32(g_esp,frame);LOAD_READ(sub_001F0B60,0x1f3661);
 g_edx=g_esi+84;PUSH32(g_esp,g_edx);LOAD_READ(sub_001F0B60,0x1f366c);
 g_eax=g_esp+32;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f3678);
 g_eax=MEM32(g_edi);g_ecx=g_eax*4;PUSH32(g_esp,g_ecx);LOAD_CALL(sub_001F4A80,0x1f3687);
 g_edx=MEM32(frame);PUSH32(g_esp,g_edx);MEM32(g_esi+24)=g_eax;LOAD_CALL(sub_001F35D0,0x1f3693);
 g_ecx=MEM32(g_esi+24);g_esp+=8;MEM32(g_esi+32)=g_eax;g_eax=MEM32(g_edi);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);LOAD_READ(sub_001F0B20,0x1f36a7);
 g_eax=MEM32(g_edi);MEM32(g_esp+20)=0;
 if((int32_t)g_eax>0){g_ecx=MEM32(g_esi+24);g_edx=g_eax;MEM32(g_esp+48)=g_edx;do{if((int32_t)MEM32(g_ecx)>=0)++MEM32(g_esp+20);g_edx=MEM32(g_esp+48);g_ecx+=4;--g_edx;MEM32(g_esp+48)=g_edx;}while(g_edx);}
 SET_LO8(g_ecx,MEM8(g_esi+72));g_eax+=7;g_edx=(int32_t)g_eax<0?0xffffffffu:0;g_edx&=7;g_eax+=g_edx;g_eax=(uint32_t)((int32_t)g_eax>>3);MEM32(g_esp+24)=g_eax;
 if(LO8(g_ecx)&8){g_ecx=MEM32(frame)*g_eax;PUSH32(g_esp,g_ecx);LOAD_CALL(sub_001F4A80,0x1f36fb);g_ecx=MEM32(frame)*MEM32(g_esp+28);g_esp+=4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;MEM32(g_esi+40)=g_eax;LOAD_CALL(sub_001F0B40,0x1f3712);}else MEM32(g_esi+40)=0;
 g_edx=MEM32(g_esp+32);PUSH32(g_esp,g_edx);LOAD_CALL(sub_001F4A80,0x1f3725);g_ecx=MEM32(g_esp+36);g_esp+=4;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;MEM32(g_esi+36)=g_eax;LOAD_CALL(sub_001F0B40,0x1f3738);
 g_edx=MEM32(frame);g_eax=MEM32(g_esi+32);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B20,0x1f3747);
 g_eax=MEM32(frame);g_ecx=g_eax*4;PUSH32(g_esp,g_ecx);LOAD_CALL(sub_001F4A80,0x1f3757);MEM32(g_esi+44)=g_eax;g_eax=MEM32(frame);g_edx=g_eax*4;PUSH32(g_esp,g_edx);LOAD_CALL(sub_001F4A80,0x1f376a);
 g_ecx=MEM32(g_esi+24);MEM32(g_esi+48)=g_eax;g_eax=MEM32(g_edi);g_eax=MEM32(g_ecx+g_eax*4-4);g_esp+=8;if((int32_t)g_eax<0)g_eax=~g_eax;++g_eax;g_ecx=g_esi+76;PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F24B0,0x1f3789);
 g_eax=MEM32(g_esi+40);g_edx=MEM32(g_esi+36);MEM32(g_esp+28)=g_eax;MEM32(g_esp+48)=g_edx;MEM32(g_esp+16)=0;
 if((int32_t)MEM32(g_edi)>0){
  do{g_ecx=MEM32(g_esi+24);g_edx=MEM32(g_esp+16);g_eax=MEM32(g_ecx+g_edx*4);if((int32_t)g_eax<0)g_eax=~g_eax;PUSH32(g_esp,1);PUSH32(g_esp,g_eax);g_ecx=g_esi+76;LOAD_CALL(sub_001F2520,0x1f37bf);g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_edi);++g_eax;MEM32(g_esp+16)=g_eax;}while((int32_t)g_eax<(int32_t)g_ecx);
  g_edx=MEM32(g_esp+48);
 }
 /* First translation row contains every track; subsequent rows omit constants. */
 g_eax=MEM32(g_edi);g_ecx=MEM32(frame);MEM32(g_esp+36)=g_eax;g_eax=0;
 if((int32_t)g_ecx>0)do{g_ecx=MEM32(g_esi+48);MEM32(g_ecx+g_eax*4)=g_edx;g_ecx=MEM32(g_esp+20);if((int32_t)g_eax<=0)g_ecx=MEM32(g_edi);g_ecx+=g_ecx*2;g_edx+=g_ecx*4;g_ecx=MEM32(frame);++g_eax;MEM32(g_esp+48)=g_edx;}while((int32_t)g_eax<(int32_t)g_ecx);
 g_ecx=MEM32(frame);g_eax=0;MEM32(g_esp+16)=0;
 if((int32_t)g_ecx>0)do{
  g_ecx=MEM32(g_esi+44);MEM32(g_ecx+g_eax*4)=g_edx;
  if(MEM8(g_esi+72)&8){g_edx=MEM32(g_esp+24);g_eax=MEM32(g_esp+28);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F2B90,0x1f3831);g_ecx=MEM32(g_edi);g_edx=MEM32(g_esp+32);g_ecx-=g_eax;g_eax=MEM32(g_esp+36);g_esp+=8;g_eax+=g_edx;g_edx=MEM32(g_esp+48);MEM32(g_esp+36)=g_ecx;MEM32(g_esp+28)=g_eax;}else g_ecx=MEM32(g_esp+36);
  if(MEM8(g_esi+64)&2){g_eax=g_ecx+g_ecx*2;g_ecx=g_edx+g_eax*2;MEM32(g_esp+48)=g_ecx;}else{g_edx+=g_ecx*4;MEM32(g_esp+48)=g_edx;}
  g_eax=MEM32(g_esp+16);g_ecx=MEM32(frame);++g_eax;MEM32(g_esp+16)=g_eax;if((int32_t)g_eax>=(int32_t)g_ecx)break;g_edx=MEM32(g_esp+48);
 }while(1);
 /* Events retain their authored order and their contiguous attribute slots. */
 frame=g_esi+88;PUSH32(g_esp,frame);LOAD_READ(sub_001F0B60,0x1f3888);g_eax=MEM32(frame);g_edi=g_esi+96;MEM32(g_edi)=0;MEM32(g_esi+104)=0;
 if((int32_t)g_eax>0){
  g_eax=(g_eax+g_eax*2)<<3;PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F4A80,0x1f38af);g_esp+=4;PUSH32(g_esp,g_edi);g_ecx=g_ebx;MEM32(g_esi+92)=g_eax;LOAD_CALL(sub_001F0B60,0x1f38bd);
  g_eax=MEM32(g_edi);if((int32_t)g_eax>0){g_eax<<=4;PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F4A80,0x1f38cc);g_esp+=4;MEM32(g_esi+100)=g_eax;}
  g_ecx=MEM32(g_edi);g_eax=0;if((int32_t)g_ecx>0){g_ecx=MEM32(g_esi+100);MEM32(g_esp+28)=g_ecx;}else MEM32(g_esp+28)=g_eax;MEM32(g_esp+16)=g_eax;
  if((int32_t)MEM32(frame)>0){
   g_edi=0;
   do{
    g_edx=MEM32(g_esi+92);g_eax=g_edi+g_edx+8;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f3905);
    g_ecx=MEM32(g_esi+92);g_edx=g_edi+g_ecx+12;PUSH32(g_esp,g_edx);LOAD_READ(sub_001F0B60,0x1f3914);
    g_eax=MEM32(g_esi+92);g_ecx=g_edi+g_eax+16;PUSH32(g_esp,g_ecx);LOAD_READ(sub_001F0B60,0x1f3923);
    g_edx=MEM32(g_esi+92);g_ecx=MEM32(g_edi+g_edx+16);g_eax=g_edi+g_edx;frame=0;
    if((int32_t)g_ecx>0){g_ecx=MEM32(g_esp+28);MEM32(g_eax+20)=g_ecx;g_eax=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_eax+16)<<4;g_ecx+=g_edx;MEM32(g_esp+28)=g_ecx;}else MEM32(g_eax+20)=frame;
    g_eax=MEM32(g_esi+92);MEM32(g_esp+48)=frame;
    if((int32_t)MEM32(g_edi+g_eax+16)>0)do{
     g_ecx=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_ecx+20);PUSH32(g_esp,1);g_eax=g_edx+frame+4;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B20,0x1f3975);
     g_ecx=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_ecx+20);g_eax=g_edx+frame+12;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B60,0x1f3988);
     g_eax=MEM32(g_esp+48);g_ecx=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_ecx+16);++g_eax;frame+=16;MEM32(g_esp+48)=g_eax;
    }while((int32_t)g_eax<(int32_t)g_edx);
    g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esi+88);frame=g_esi+88;++g_eax;g_edi+=24;MEM32(g_esp+16)=g_eax;
   }while((int32_t)g_eax<(int32_t)g_ecx);
  }
  g_edi=g_esi+104;PUSH32(g_esp,g_edi);LOAD_READ(sub_001F0B60,0x1f39c4);g_eax=MEM32(g_edi);PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F4A80,0x1f39cc);g_edx=MEM32(g_edi);g_esp+=4;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;MEM32(g_esi+108)=g_eax;LOAD_CALL(sub_001F0B40,0x1f39dd);
  unsigned more=(int32_t)MEM32(frame)>0;g_ecx=MEM32(g_esi+108);MEM32(g_esp+16)=0;
  if(more){
   g_edi=0;
   do{
    g_eax=MEM32(g_esi+92);MEM32(g_edi+g_eax)=g_ecx;g_eax=g_ecx;frame=g_eax+1;do{SET_LO8(g_edx,MEM8(g_eax));++g_eax;}while(LO8(g_edx));g_eax-=frame;frame=g_ecx+g_eax+1;g_ecx=MEM32(g_esi+92);g_eax=frame;MEM32(g_edi+g_ecx+4)=frame;g_ecx=g_eax+1;
    do{SET_LO8(g_edx,MEM8(g_eax));++g_eax;}while(LO8(g_edx));g_edx=MEM32(g_esi+92);g_eax-=g_ecx;g_ecx=g_eax+frame+1;g_eax=MEM32(g_edi+g_edx+16);frame=0;MEM32(g_esp+48)=frame;
    if((int32_t)g_eax>0)do{
     g_eax=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_eax+20);g_eax=g_ecx;MEM32(g_edx+frame)=g_ecx;g_edx=g_eax+1;MEM32(g_esp+36)=g_edx;
     do{SET_LO8(g_edx,MEM8(g_eax));++g_eax;}while(LO8(g_edx));g_eax-=MEM32(g_esp+36);g_ecx+=g_eax+1;g_eax=MEM32(g_esi+92);g_edx=MEM32(g_edi+g_eax+20);g_eax=g_ecx;MEM32(g_edx+frame+8)=g_ecx;g_edx=g_eax+1;MEM32(g_esp+36)=g_edx;
     do{SET_LO8(g_edx,MEM8(g_eax));++g_eax;}while(LO8(g_edx));g_eax-=MEM32(g_esp+36);g_edx=MEM32(g_esi+92);g_ecx+=g_eax+1;g_eax=MEM32(g_esp+48);++g_eax;frame+=16;MEM32(g_esp+48)=g_eax;
    }while((int32_t)g_eax<(int32_t)MEM32(g_edi+g_edx+16));
    g_eax=MEM32(g_esp+16);g_edx=MEM32(g_esi+88);++g_eax;g_edi+=24;MEM32(g_esp+16)=g_eax;
   }while((int32_t)g_eax<(int32_t)g_edx);
  }
 }
 /* Authored boolean states are eight-byte ID/value/time records. */
 g_edi=g_esi+112;PUSH32(g_esp,g_edi);LOAD_READ(sub_001F0B60,0x1f3abc);g_eax=MEM32(g_edi);
 if((int32_t)g_eax>0){
  g_eax<<=3;PUSH32(g_esp,g_eax);LOAD_CALL(sub_001F4A80,0x1f3acb);MEM32(g_esi+116)=g_eax;g_eax=MEM32(g_edi);g_esp+=4;frame=0;
  if((int32_t)g_eax>0)do{
   g_ecx=MEM32(g_esi+116);g_edi=frame*8;g_ecx+=g_edi;PUSH32(g_esp,g_ecx);LOAD_READ(sub_001F0B80,0x1f3af4);
   g_edx=MEM32(g_esi+116);g_eax=g_edi+g_edx+2;PUSH32(g_esp,g_eax);LOAD_READ(sub_001F0B80,0x1f3b03);
   g_ecx=MEM32(g_esi+116);g_edx=g_edi+g_ecx+4;PUSH32(g_esp,g_edx);LOAD_READ(sub_001F0B60,0x1f3b12);g_eax=MEM32(g_esi+112);++frame;
  }while((int32_t)frame<(int32_t)g_eax);
 }
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=28;g_recomp_zf=g_esp==0;g_esp+=8;return 1;
}
#undef LOAD_CALL
#undef LOAD_READ
