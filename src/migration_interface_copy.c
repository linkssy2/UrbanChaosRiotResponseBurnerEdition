#include "migration_interface_copy.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
static int copy_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static void copy_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void copy_set_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void copy_error(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));copy_call(0x44b27c,0x820f9,saved);g_edx=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);copy_set_error();g_eax=g_edx;g_esp+=8;}
static void copy_progress(uint32_t frame,int initial){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(frame+116));PUSH32(g_esp,MEM32(frame+84));PUSH32(g_esp,MEM32(frame+80));PUSH32(g_esp,initial?1:g_edi);PUSH32(g_esp,1);PUSH32(g_esp,initial?g_edi:MEM32(frame+92));PUSH32(g_esp,initial?g_edi:g_eax);PUSH32(g_esp,MEM32(frame+52));PUSH32(g_esp,MEM32(frame+48));PUSH32(g_esp,initial?g_edi:MEM32(frame+92));PUSH32(g_esp,initial?g_edi:g_eax);PUSH32(g_esp,MEM32(frame+52));PUSH32(g_esp,MEM32(frame+48));copy_call(frame+112,initial?0x8152e:0x815f5,saved);}
static void copy_file(void){
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);uint32_t frame=g_esp-96;g_esp-=156;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);PUSH32(g_esp,4);g_eax=frame-20;g_ebx=65536;PUSH32(g_esp,g_ebx);MEM32(frame-4)=0xfffffffd;MEM32(frame+4)=0x40;MEM32(frame)=g_eax;copy_call(0x44b3d4,0x8141f,saved);g_edi=0;MEM32(frame+72)=g_eax;
 if(!g_eax){PUSH32(g_esp,8);PUSH32(g_esp,0x8142f);copy_set_error();g_eax=0;goto finish;}
 saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame+104));g_esi=MEM32(0x44b2b8);g_eax=frame-20;PUSH32(g_esp,g_eax);copy_call(0x44b2b8,0x81446,saved);
 saved=g_esp;PUSH32(g_esp,0x60);PUSH32(g_esp,1);g_eax=frame+64;PUSH32(g_esp,g_eax);g_eax=frame-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x80100000);g_eax=frame+80;PUSH32(g_esp,g_eax);copy_call(0x44b2b4,0x81461,saved);MEM32(frame+104)=g_eax;
 if((int32_t)g_eax<0)goto source_failed;
 saved=g_esp;PUSH32(g_esp,34);PUSH32(g_esp,56);g_eax=frame+8;PUSH32(g_esp,g_eax);g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+80));copy_call(0x44b3bc,0x81496,saved);MEM32(frame+104)=g_eax;
 if((int32_t)g_eax<0){saved=g_esp;PUSH32(g_esp,MEM32(frame+80));copy_call(0x44b2bc,0x814a6,saved);goto source_failed;}
 saved=g_esp;PUSH32(g_esp,MEM32(frame+108));g_eax=frame-20;PUSH32(g_esp,g_eax);copy_call(0x44b2b8,0x814b1,saved);g_eax=MEM32(frame+124);int exclusive=g_eax&1;saved=g_esp;PUSH32(g_esp,0x64);g_eax=exclusive?2:5;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x80);g_eax=frame+48;PUSH32(g_esp,g_eax);g_eax=frame+64;PUSH32(g_esp,g_eax);g_eax=frame-4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x40100000);g_eax=frame+84;PUSH32(g_esp,g_eax);copy_call(0x44b320,0x814e4,saved);g_esi=g_eax;
 if((int32_t)g_esi<0){saved=g_esp;PUSH32(g_esp,MEM32(frame+80));copy_call(0x44b2bc,0x814f3,saved);saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+72));copy_call(0x44b3d0,0x814fd,saved);PUSH32(g_esp,g_esi);goto early_error;}
 MEM32(frame+88)=MEM32(frame+92)=g_edi;
 if(MEM32(frame+112)!=g_edi){copy_progress(frame,1);if(g_eax==3)MEM32(frame+112)=g_edi;else if(g_eax!=g_edi){g_esi=0xc0000240;goto cleanup;}}
 g_eax=MEM32(frame+48);MEM32(frame-12)=g_eax;g_eax=MEM32(frame+52);saved=g_esp;PUSH32(g_esp,20);MEM32(frame-8)=g_eax;PUSH32(g_esp,8);g_eax=frame-12;PUSH32(g_esp,g_eax);g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+84));copy_call(0x44b34c,0x8155b,saved);g_esi=g_eax;
 if((int32_t)g_esi<0)goto cleanup;
 for(;;){g_eax=MEM32(frame+92);if((int32_t)g_eax>(int32_t)MEM32(frame+52))break;if(g_eax==MEM32(frame+52)){g_eax=MEM32(frame+88);if(g_eax>=MEM32(frame+48))break;}
  g_eax=frame+88;saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+72));g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame+80));copy_call(0x44b2c0,0x8158f,saved);g_esi=g_eax;if((int32_t)g_esi<0)goto cleanup;
  g_eax=MEM32(frame+68);g_ecx=frame+88;saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+72));MEM32(frame+104)=g_eax;g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edi);PUSH32(g_esp,MEM32(frame+84));copy_call(0x44b2c4,0x815b7,saved);g_esi=g_eax;if((int32_t)g_esi<0)goto cleanup;
  uint64_t sum=(uint64_t)MEM32(frame+88)+MEM32(frame+104);g_eax=(uint32_t)sum;MEM32(frame+92)+=g_edi+(uint32_t)(sum>>32);MEM32(frame+88)=g_eax;
  if(MEM32(frame+112)!=g_edi){copy_progress(frame,0);if(g_eax==3)MEM32(frame+112)=g_edi;else if(g_eax!=g_edi){g_esi=0xc0000240;break;}}
 }
 if((int32_t)g_esi<0)goto cleanup;
 for(unsigned i=0;i<8;i++){g_eax=MEM32(frame+8+i*4);MEM32(frame-60+i*4)=g_eax;}g_eax=MEM32(frame+56);saved=g_esp;PUSH32(g_esp,4);MEM32(frame-28)=g_eax;PUSH32(g_esp,40);g_eax=frame-60;PUSH32(g_esp,g_eax);g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+84));copy_call(0x44b34c,0x81682,saved);g_esi=g_eax;
cleanup:
 saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+72));copy_call(0x44b3d0,0x8168e,saved);saved=g_esp;PUSH32(g_esp,MEM32(frame+80));g_ebx=MEM32(0x44b2bc);copy_call(0x44b2bc,0x81699,saved);
 if((int32_t)g_esi<0){saved=g_esp;PUSH32(g_esp,13);PUSH32(g_esp,1);g_eax=frame+79;PUSH32(g_esp,g_eax);g_eax=frame+64;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+84));MEM8(frame+79)=1;copy_call(0x44b34c,0x816b6,saved);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x816bc);copy_error();}else g_edi=1;
 saved=g_esp;PUSH32(g_esp,MEM32(frame+84));copy_call(0x44b2bc,0x816c6,saved);g_eax=g_edi;goto pop_esi;
source_failed:saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,MEM32(frame+72));copy_call(0x44b3d0,0x81472,saved);PUSH32(g_esp,MEM32(frame+104));
early_error:PUSH32(g_esp,0x8147a);copy_error();g_eax=0;
pop_esi:POP32(g_esp,g_esi);
finish:POP32(g_esp,g_edi);POP32(g_esp,g_ebx);g_recomp_zf=!MEM32(frame+96);g_esp=frame+128;
}
int migration_interface_copy(unsigned op){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_COPY");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;uint32_t s=g_esp;if(s<0x10400||(s&3)||!copy_span(s-1024,1056))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;if(!copy_span(tls,4)||!copy_span((uint64_t)MEM32((uint32_t)tls)+8,4))return 0;
 const uint32_t slots[]={0x44b27c,0x44b2bc,0x44b2b8,0x44b2b4,0x44b320,0x44b34c,0x44b3bc,0x44b3d4,0x44b3d0,0x44b2c0,0x44b2c4};for(unsigned i=0;i<11;i++){uint32_t t=MEM32(slots[i]);if(t<0xfe000000||t>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(t&3))return 0;}
 for(unsigned k=1;k<=2;k++){uint32_t p=MEM32(s+k*4);unsigned i=0;for(;i<512;i++){if(!copy_span((uint64_t)p+i,1))return 0;if(!MEM8(p+i))break;}if(i==512)return 0;}
 if(!op){copy_file();return 1;}int zero=!MEM32(s+12);g_eax=0;g_ecx=!zero;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,0);PUSH32(g_esp,MEM32(g_esp+24));PUSH32(g_esp,MEM32(g_esp+24));PUSH32(g_esp,0x816ee);copy_file();g_recomp_zf=zero;g_esp+=16;return 1;
}
