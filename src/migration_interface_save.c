#include "migration_interface_save.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>

static int save_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_SAVE");v=s&&!strcmp(s,"1");}return v;}
static int save_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int save_apart(uint64_t a,unsigned n,uint64_t b,unsigned m){return a+n<=b||b+m<=a;}
/* Retain the thunk's guest stack/volatile contract while using the existing
 * native SHA service directly. No extra SHA implementation or new save format. */
static void sha_init(void){
 xbox_XcSHAInit((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)));g_eax=0;g_esp+=8;
}
static void sha_update(void){
 xbox_XcSHAUpdate((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(const UCHAR*)XBOX_PTR(MEM32(g_esp+8)),MEM32(g_esp+12));g_eax=0;g_esp+=16;
}
static void sha_final(void){
 xbox_XcSHAFinal((PXBOX_SHA_CONTEXT)XBOX_PTR(MEM32(g_esp+4)),(UCHAR*)XBOX_PTR(MEM32(g_esp+8)));g_eax=0;g_esp+=12;
}
int migration_interface_save(unsigned op){
 if(!save_enabled()||op>1)return 0;
 uint32_t s=g_esp;
 if((s&3)||s<0x10100||!save_span(s-128,148))return 0;
 uint32_t key=MEM32(s+(op?8:4)),length=MEM32(s+(op?12:8));
 uint32_t context=MEM32(s+(op?4:12)),digest=op?MEM32(s+16):0;
 if(length>64)length=64; /* Title truncates long keys; RFC long-key hashing differs. */
 if(!save_span(key,length)||!save_span(context,sizeof(XBOX_SHA_CONTEXT))||!save_apart(key,length,s-128,148)||!save_apart(context,sizeof(XBOX_SHA_CONTEXT),s-128,148))return 0;
 if(op&&(!save_span(digest,20)||!save_apart(digest,20,s-128,148)))return 0;
 PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;
 g_esp-=op?84:64;
 if(op){PUSH32(g_esp,64);POP32(g_esp,g_eax);}
 if(MEM32(frame+(op?16:12))>64)MEM32(frame+(op?16:12))=64;
 PUSH32(g_esp,g_esi);g_esi=key;PUSH32(g_esp,g_edi);PUSH32(g_esp,16);POP32(g_esp,g_ecx);
 const uint32_t buffer=frame-(op?84:64);
 memset((void*)XBOX_PTR(buffer),0,64);memcpy((void*)XBOX_PTR(buffer),(const void*)XBOX_PTR(key),length);
 g_ecx=0;g_esi=key+length;g_edi=buffer+length;
 if(!op){POP32(g_esp,g_edi);POP32(g_esp,g_esi);}
 for(unsigned i=0;i<64;i++)MEM8(buffer+i)^=op?0x5c:0x36;
 g_eax=16;
 if(!op){
  PUSH32(g_esp,MEM32(frame+16));PUSH32(g_esp,0x809c4);sha_init();
  PUSH32(g_esp,64);g_eax=buffer;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+16));PUSH32(g_esp,0x809d2);sha_update();
 }else{
  g_esi=MEM32(frame+8);g_eax=frame-20;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x80a2c);sha_final();
  PUSH32(g_esp,g_esi);PUSH32(g_esp,0x80a32);sha_init();
  PUSH32(g_esp,84);g_eax=buffer;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x80a3e);sha_update();
  PUSH32(g_esp,MEM32(frame+20));PUSH32(g_esp,g_esi);PUSH32(g_esp,0x80a47);sha_final();
  POP32(g_esp,g_edi);POP32(g_esp,g_esi);
 }
 g_esp=frame+4+(op?20:16);g_recomp_zf=1;return 1;
}
