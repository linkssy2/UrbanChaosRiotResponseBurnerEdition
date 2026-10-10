#include "migration_interface_volume.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "kernel.h"
#include <stdlib.h>
static int volume_enabled(void){static int v=-1;if(v<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_VOLUME");v=s&&!strcmp(s,"1");}return v;}
static int volume_span(uint64_t p,unsigned n){return p>=0x10000&&p+n<=g_xbox_total_ram&&p+n<=0x80000000ull;}
static int volume_apart(uint64_t p,unsigned n,uint64_t s,unsigned m){return p+n<=s||s+m<=p;}
static void volume_call(uint32_t slot,uint32_t ret,uint32_t saved){uint32_t target=MEM32(slot);PUSH32(g_esp,ret);RECOMP_ICALL_SAFE(target,saved);}
static void volume_set_error(void){unsigned level=MEM8(0x24);g_ecx=MEM32(4);g_eax=MEM32(0x519c50);g_eax=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+4);MEM32(g_eax+8)=g_ecx;g_recomp_zf=level==2;g_esp+=8;}
static void volume_error(void){uint32_t saved=g_esp;PUSH32(g_esp,MEM32(g_esp+4));volume_call(0x44b27c,0x820f9,saved);g_edx=g_eax;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x82101);volume_set_error();g_eax=g_edx;g_esp+=8;}
static void volume_multiply(void){
 uint32_t a=MEM32(g_esp+4),hi=MEM32(g_esp+8),b=MEM32(g_esp+12),bh=MEM32(g_esp+16);g_ecx=b;
 uint64_t product=(uint64_t)a*b;
 if(b){PUSH32(g_esp,g_ebx);g_ebx=hi*b+a*bh;g_eax=(uint32_t)product;g_edx=(uint32_t)(product>>32)+g_ebx;POP32(g_esp,g_ebx);g_recomp_zf=g_edx==0;}
 else g_eax=g_edx=0;
 g_esp+=20;
}
int migration_interface_volume(unsigned space){
 if(!volume_enabled()||space>1)return 0;
 uint32_t s=g_esp;if(s<0x10100||(s&3)||!volume_span(s-256,280)||!volume_span(0x519c50,4))return 0;
 uint64_t tls=(uint64_t)MEM32(4)+(uint64_t)MEM32(0x519c50)*4;
 if(!volume_span(tls,4)||!volume_span((uint64_t)MEM32((uint32_t)tls)+8,4)||!volume_apart(tls,4,s-256,280)||!volume_apart(MEM32((uint32_t)tls),12,s-256,280))return 0;
 static const uint32_t slots[]={0x44b27c,0x44b2b8,0x44b2b4,0x44b3a0,0x44b2bc};
 for(unsigned i=0;i<5;i++){uint32_t v=MEM32(slots[i]);if(v<0xfe000000||v>=0xfe000000+XBOX_KERNEL_THUNK_TABLE_SIZE*4||(v&3))return 0;}
 uint32_t path=MEM32(s+4),length=0;while(length<260&&volume_span((uint64_t)path+length,1)&&MEM8(path+length))length++;
 if(length==260||!volume_span((uint64_t)path+length,1)||!volume_apart(path,length+1,s-256,280))return 0;
 if(space)for(unsigned i=2;i<=4;i++){uint32_t p=MEM32(s+i*4);if(p&&(!volume_span(p,8)||!volume_apart(p,8,s-256,280)))return 0;}
 uint32_t saved=g_esp;PUSH32(g_esp,g_ebp);const uint32_t frame=g_esp;g_ebp=frame;g_esp-=52;PUSH32(g_esp,MEM32(frame+8));g_eax=frame-8;PUSH32(g_esp,g_eax);volume_call(0x44b2b8,space?0x804a4:0x808e8,saved);
 saved=g_esp;PUSH32(g_esp,0x800021);g_eax=frame-8;MEM32(frame-24)=g_eax;PUSH32(g_esp,3);g_eax=frame-16;PUSH32(g_esp,g_eax);g_eax=frame-28;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x100001);g_eax=frame+8;PUSH32(g_esp,g_eax);MEM32(frame-28)=0xfffffffd;MEM32(frame-20)=0x40;volume_call(0x44b2b4,space?0x804d6:0x8091a,saved);
 if((int32_t)g_eax<0){
  PUSH32(g_esp,g_eax);g_ebp=frame;PUSH32(g_esp,space?0x804e0:0x80924);volume_error();g_ebp=frame;PUSH32(g_esp,space?0x804e5:0x80929);
  g_ecx=MEM32(4);g_eax=MEM32(g_ecx+MEM32(0x519c50)*4);g_eax=MEM32(g_eax+8);g_recomp_zf=MEM8(0x24)==2;g_esp+=4;
  if(g_eax==2){PUSH32(g_esp,3);g_ebp=frame;PUSH32(g_esp,space?0x804f1:0x80935);volume_set_error();}g_eax=0;
 }else{
  saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,3);PUSH32(g_esp,24);g_eax=frame-52;PUSH32(g_esp,g_eax);g_eax=frame-16;PUSH32(g_esp,g_eax);PUSH32(g_esp,MEM32(frame+8));volume_call(0x44b3a0,space?0x8050e:0x8094f,saved);
  saved=g_esp;PUSH32(g_esp,MEM32(frame+8));g_esi=g_eax;volume_call(0x44b2bc,space?0x80519:0x8095a,saved);
  if((int32_t)g_esi<0){PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,space?0x80523:0x80964);volume_error();g_eax=0;}
  else if(!space)g_eax=MEM32(frame-36)*MEM32(frame-32);
  else{
   g_esi=MEM32(frame-36)*MEM32(frame-32);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);g_edi=0;PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame-40));PUSH32(g_esp,MEM32(frame-44));g_ebp=frame;PUSH32(g_esp,0x8053f);volume_multiply();
   PUSH32(g_esp,g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,MEM32(frame-48));g_ebx=g_eax;PUSH32(g_esp,MEM32(frame-52));MEM32(frame-4)=g_edx;g_ebp=frame;PUSH32(g_esp,0x80551);volume_multiply();
   g_ecx=MEM32(frame+12);if(g_ecx){g_esi=MEM32(frame-4);MEM32(g_ecx)=g_ebx;MEM32(g_ecx+4)=g_esi;}g_ecx=MEM32(frame+16);if(g_ecx){MEM32(g_ecx)=g_eax;MEM32(g_ecx+4)=g_edx;}g_eax=MEM32(frame+20);if(g_eax){g_ecx=MEM32(frame-4);MEM32(g_eax)=g_ebx;MEM32(g_eax+4)=g_ecx;}
   g_eax=1;POP32(g_esp,g_edi);POP32(g_esp,g_ebx);
  }
  POP32(g_esp,g_esi);
 }
 g_esp=frame+4+(space?20:8);return 1;
}
