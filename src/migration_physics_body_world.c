/* Original world body lifetime: locking, queued work, references and listeners. */
#include "migration_physics_body_world.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002F9FB0(void);void sub_003889E0(void);void sub_002F6730(void);void sub_002F76E0(void);
void sub_002F7900(void);void sub_002F7400(void);void sub_002F6F50(void);void sub_002ED490(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,552);}
static int list_valid(uint32_t h,uint32_t stride,int nullable,uint32_t vsize){
 uint32_t n=MEM32(h+4),p=MEM32(h);if(n>65535||(n&&!safe(p,n*stride)))return 0;
 for(unsigned i=0;i<n;i++){uint32_t q=MEM32(p+i*stride);if(nullable&&!q)continue;if(!safe(q,4)||!safe(MEM32(q),vsize))return 0;}return 1;
}
static int body_valid(uint32_t b){return safe(b,0xc0)&&safe(MEM32(b),20)&&safe(MEM32(b+0x50),32)&&list_valid(b+0xac,4,1,12)&&list_valid(b+0x68,28,0,16)&&list_valid(b+0x74,4,0,16)&&list_valid(b+0xb8,4,1,24);}
static void profile(uint32_t tag,uint32_t offset,int low_edx){
 if(g_ecx<MEM32(0x52020c)){MEM32(g_ecx)=tag;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp+offset)=g_eax;if(low_edx)g_edx=MEM32(g_esp+offset);else g_eax=MEM32(g_esp+offset);MEM32(g_ecx+4)=low_edx?g_edx:g_eax;g_ecx+=12;MEM32(0x520208)=g_ecx;}
}
static void world_notify(int remove){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+0xcc)-1;
 if((int32_t)g_edi>=0){g_ebx=MEM32(g_esp+20);do{g_eax=MEM32(g_esi+0xc8);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);uint32_t target=MEM32(g_edx+(remove?8:4));PUSH32(g_esp,remove?0x2fab17:0x2faa87);RECOMP_ICALL_SAFE(target,saved);}--g_edi;}while((int32_t)g_edi>=0);}
 g_edx=MEM32(g_esi+0xcc)-1;if((int32_t)g_edx>=0)do{g_eax=MEM32(g_esi+0xc8);if(!MEM32(g_eax+g_edx*4)){g_ebx=MEM32(g_esi+0xcc)-1;g_ecx=g_ebx;MEM32(g_esi+0xcc)=g_ebx;g_eax=g_edx;if((int32_t)g_edx<(int32_t)g_ecx)do{g_ecx=MEM32(g_esi+0xc8);g_edi=MEM32(g_ecx+g_eax*4+4);g_ecx+=g_eax*4;MEM32(g_ecx)=g_edi;g_ecx=MEM32(g_esi+0xcc);++g_eax;}while((int32_t)g_eax<(int32_t)g_ecx);}--g_edx;}while((int32_t)g_edx>=0);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=0;g_esp+=4;
}
static void added(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=MEM32(g_edi+0xb0)-1;
 if((int32_t)g_esi>=0)do{g_eax=MEM32(g_edi+0xac);g_ecx=MEM32(g_eax+g_esi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x2f8873);RECOMP_ICALL_SAFE(target,saved);}--g_esi;}while((int32_t)g_esi>=0);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=4;
}
static void removed(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+0xb0)-1;
 if((int32_t)g_edi>=0)do{g_eax=MEM32(g_esi+0xac);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+8);PUSH32(g_esp,0x2f8b43);RECOMP_ICALL_SAFE(target,saved);}--g_edi;}while((int32_t)g_edi>=0);
 g_eax=MEM32(g_esi+0x6c);while(g_eax){g_eax=MEM32(g_esi+0x68);g_ecx=MEM32(g_eax);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2f8b5b);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+0x6c);}
 g_eax=MEM32(g_esi+0x78);while(g_eax){g_eax=MEM32(g_esi+0x74);g_ecx=MEM32(g_eax);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x2f8b7b);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+0x78);}
 g_eax=MEM32(g_esi+0xbc);g_edi=0;while((int32_t)g_edi<(int32_t)g_eax){g_eax=MEM32(g_esi+0xb8);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x2f8ba3);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esi+0xbc);++g_edi;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static void add_body(void){
 g_esp-=16;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x88);
 if(g_eax){g_ecx=MEM32(g_esp+28);g_eax=MEM32(g_esp+24);g_edx=g_esp+8;MEM32(g_esp+16)=g_ecx;g_ecx=MEM32(g_esi+0x80);PUSH32(g_esp,g_edx);MEM8(g_esp+12)=1;MEM32(g_esp+16)=g_eax;CALL(0x2ed5a5,sub_002F9FB0);++MEM32(g_esi+0x84);g_eax=0;POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;return;}
 g_ecx=MEM32(0x520208);profile(0x498d2c,4,0);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);g_eax=MEM32(g_edi+28);
 if(!g_eax){g_edx=MEM32(g_edi);g_ecx=g_edi;uint32_t saved=g_esp,target=MEM32(g_edx+12);PUSH32(g_esp,0x2ed5f1);RECOMP_ICALL_SAFE(target,saved);MEM32(g_edi+28)=g_eax;}
 g_eax=MEM32(g_edi+0x50)+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);CALL(0x2ed604,sub_003889E0);g_esp+=12;MEM8(g_esi+0x91)=0;if(MEM16(g_edi+4))++MEM16(g_edi+6);
 g_ecx=MEM32(g_esp+32);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);CALL(0x2ed625,sub_002F6730);g_ecx=MEM32(g_esi+0x88);g_esp+=12;++g_ecx;MEM8(g_esi+0x91)=1;MEM32(g_esi+0x88)=g_ecx;g_ecx=MEM32(0x520208);profile(0x498d1c,28,1);
 PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);CALL(0x2ed66d,sub_002F76E0);g_ecx=MEM32(0x520208);g_eax=MEM32(0x52020c);g_esp+=8;profile(0x498d10,28,0);
 PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2ed6a2);world_notify(0);PUSH32(g_esp,g_edi);CALL(0x2ed6a8,added);g_eax=MEM32(g_esi+0x88);g_esp+=12;--g_eax;MEM32(g_esi+0x88)=g_eax;
 if(!g_eax){g_eax=MEM32(g_esi+0x84);if(g_eax){SET_LO8(g_eax,MEM8(g_esi+0x90));if(!(uint8_t)g_eax){g_ecx=g_esi;CALL(0x2ed6d5,sub_002ED490);}}}
 g_ecx=MEM32(0x520208);profile(0x4988f8,28,1);g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void remove_body(void){
 g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi+0x88);
 if(g_eax){g_eax=MEM32(g_esp+20);g_ecx=g_esp+4;PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esi+0x80);MEM8(g_esp+8)=2;MEM32(g_esp+12)=g_eax;CALL(0x2ed73d,sub_002F9FB0);++MEM32(g_esi+0x84);g_eax=MEM32(g_esp+16);MEM8(g_eax)=0;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;return;}
 MEM32(g_esi+0x88)=1;g_ecx=MEM32(0x520208);profile(0x498d4c,4,1);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+24);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);CALL(0x2ed791,sub_002F7900);g_eax=MEM32(g_esi+0xac);g_esp+=8;
 if((int32_t)g_eax>=4){g_eax=MEM32(g_esi+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);CALL(0x2ed7a9,sub_002F7400);g_esp+=8;}
 g_ecx=MEM32(0x520208);profile(0x498d10,24,1);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2ed7dd);world_notify(1);PUSH32(g_esp,g_edi);CALL(0x2ed7e3,removed);MEM8(g_esi+0x91)=0;g_ecx=MEM32(0x520208);g_eax=MEM32(0x52020c);g_esp+=12;profile(0x498d40,24,0);
 g_eax=MEM32(g_edi+8);if(g_eax){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);CALL(0x2ed826,sub_002F6F50);g_esp+=8;
  if(!MEM16(g_edi+4)){g_edx=MEM32(g_edi);g_ecx=g_edi;uint32_t saved=g_esp,target=MEM32(g_edx+16);PUSH32(g_esp,0x2ed837);RECOMP_ICALL_SAFE(target,saved);}
  if(MEM16(g_edi+4)){--MEM16(g_edi+6);if(!MEM16(g_edi+6)){g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_edi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2ed851);RECOMP_ICALL_SAFE(target,saved);}}
 }
 g_eax=MEM32(g_esi+0x88)-1;MEM8(g_esi+0x91)=1;MEM32(g_esi+0x88)=g_eax;POP32(g_esp,g_edi);
 if(!g_eax){g_eax=MEM32(g_esi+0x84);if(g_eax){SET_LO8(g_eax,MEM8(g_esi+0x90));if(!(uint8_t)g_eax){g_ecx=g_esi;CALL(0x2ed883,sub_002ED490);}}}
 g_ecx=MEM32(0x520208);profile(0x4988f8,20,1);g_eax=MEM32(g_esp+16);MEM8(g_eax)=1;POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_physics_body_world(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_WORLD");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-512,552))return 0;
 if(entry==0x2f8850||entry==0x2f8b20){if(!body_valid(MEM32(g_esp+4)))return 0;if(entry==0x2f8850)added();else removed();return 1;}
 uint32_t w,b;if(entry==0x2faa60||entry==0x2faaf0){w=MEM32(g_esp+4);b=MEM32(g_esp+8);}else if(entry==0x2ed570||entry==0x2ed710){w=g_ecx;b=MEM32(g_esp+(entry==0x2ed710?8:4));}else return 0;
 if(!safe(w,0xd0)||!body_valid(b)||!safe(MEM32(w+0x80),16)||!list_valid(w+0xc8,4,1,12))return 0;
 if(entry==0x2faa60||entry==0x2faaf0){world_notify(entry==0x2faaf0);return 1;}
 if(entry==0x2ed710&&!safe(MEM32(g_esp+4),1))return 0;if(!span(0x520208,8))return 0;uint32_t p=MEM32(0x520208);if(p<MEM32(0x52020c)&&!safe(p,48))return 0;
 if(entry==0x2ed570)add_body();else remove_body();return 1;
}
#undef CALL
