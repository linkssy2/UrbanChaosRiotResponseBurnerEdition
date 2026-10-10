/* Original island construction, body membership and queued-contact ownership. */
#include "migration_physics_body_storage.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+32<=p);}
static int list_valid(uint32_t h,uint32_t width){uint32_t n=MEM32(h+4),c=MEM32(h+8)&0x3fffffff;return n<=65534&&c>=n&&c<=65535&&(!c||safe(MEM32(h),c*width));}
static int island_valid(uint32_t p){return safe(p,0x6c)&&safe(MEM32(p),24)&&list_valid(p+0x34,4);}
static int body_valid(uint32_t p){return safe(p,0x94)&&safe(MEM32(p+0x50),4)&&safe(MEM32(MEM32(p+0x50)),32);}
static void construct(void){
 g_eax=g_ecx;g_edx=MEM32(g_esp+4);MEM32(g_eax+0x1c)=g_edx;MEM32(g_eax)=0x49ac94;MEM16(g_eax+0x22)=0xffff;g_ecx=0;
 for(unsigned o=8;o<=24;o+=4)MEM32(g_eax+o)=0;MEM8(g_eax+0x24)=0;MEM8(g_eax+0x25)=0;
 PUSH32(g_esp,g_ebx);g_ebx=1;MEM16(g_eax+6)=1;MEM8(g_eax+0x26)=0;MEM8(g_eax+0x27)=0;MEM8(g_eax+0x28)=1;MEM8(g_eax+0x29)=1;
 MEM32(g_eax+0x2c)=0;MEM32(g_eax+0x30)=0;MEM32(g_eax+0x38)=0;MEM32(g_eax+0x3c)=0x80000001;
 PUSH32(g_esp,g_esi);g_esi=g_eax+0x40;MEM32(g_eax+0x34)=g_esi;g_edx=MEM32(g_edx+0x7c);g_esi=MEM32(g_edx+0x1bf8);g_edx=MEM32(g_edx+0x1bfc);MEM16(g_eax+0x58)=(uint16_t)g_edx;MEM16(g_eax+0x5a)=(uint16_t)g_esi;g_edx=(uint16_t)g_esi;
 g_ebx=g_eax+0x50;MEM32(g_eax+0x44)=g_ebx;MEM32(g_eax+0x48)=0;MEM32(g_eax+0x4c)=0x80000001;MEM32(g_eax+0x54)=g_edx;POP32(g_esp,g_esi);
 MEM32(g_eax+0x5c)=0;MEM32(g_eax+0x60)=0;MEM32(g_eax+0x64)=0x80000000;MEM32(g_eax+0x68)=0xc1200000;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
static void account(void){g_edx=MEM32(g_ecx+0x14);g_eax=MEM32(g_esp+4);g_edx+=g_eax;MEM32(g_ecx+0x14)=g_edx;g_recomp_zf=g_edx==0;g_esp+=8;}
static void release_arrays(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esi)=0x49ac94;
 const uint32_t offsets[]={0x5c,0x44,0x34},returns[]={0x2fce2a,0x2fce4b,0x2fce6c};
 for(unsigned i=0;i<3;i++){g_eax=MEM32(g_esi+offsets[i]+8);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;uint32_t saved=g_esp;PUSH32(g_esp,0x14);g_eax<<=2;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi+offsets[i]);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+0x14);PUSH32(g_esp,returns[i]);RECOMP_ICALL_SAFE(target,saved);}}
 MEM32(g_esi)=0x4564ec;POP32(g_esp,g_esi);g_esp+=4;
}
static void destroy(void){
 PUSH32(g_esp,g_esi);g_esi=g_ecx;CALL(0x2fd188,release_arrays);int keep=!(MEM8(g_esp+8)&1);if(!keep){g_edx=MEM16(g_esi+4);g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0x2f);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+0x14);PUSH32(g_esp,0x2fd1a2);RECOMP_ICALL_SAFE(target,saved);}g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=keep;g_esp+=8;
}
static void attach(void){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;MEM32(g_ebx+0x54)=g_edi;SET_LO16(g_eax,MEM16(g_edi+0x38));g_esi=g_edi+0x34;MEM16(g_ebx+0x8c)=(uint16_t)g_eax;g_ecx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);
 if(g_eax==g_ecx){PUSH32(g_esp,4);PUSH32(g_esp,g_esi);CALL(0x2fcba2,sub_002E0150);g_esp+=8;}
 g_edx=MEM32(g_esi+4);g_eax=MEM32(g_esi);MEM32(g_eax+g_edx*4)=g_ebx;uint32_t count=++MEM32(g_esi+4);g_ecx=MEM32(g_ebx+0x50);g_edx=MEM32(g_ecx);g_esi=MEM32(g_edi);
 uint32_t saved=g_esp,target=MEM32(g_edx+0x1c);PUSH32(g_esp,0x2fcbba);RECOMP_ICALL_SAFE(target,saved);
 saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_edi;target=MEM32(g_esi+0x14);PUSH32(g_esp,0x2fcbc0);RECOMP_ICALL_SAFE(target,saved);
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=count==0;g_esp+=8;
}
static void detach(void){
 g_eax=MEM32(g_esp+4);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+0x34);g_edx=MEM32(g_esi+0x38);g_edx=MEM32(g_ecx+g_edx*4-4);PUSH32(g_esp,g_edi);g_edi=MEM16(g_eax+0x8c);MEM32(g_ecx+g_edi*4)=g_edx;
 SET_LO16(g_ecx,MEM16(g_eax+0x8c));g_edi=MEM32(g_esi+0x34);g_edx=(uint16_t)g_ecx;g_edx=MEM32(g_edi+g_edx*4);MEM16(g_edx+0x8c)=(uint16_t)g_ecx;--MEM32(g_esi+0x38);MEM32(g_eax+0x54)=0;MEM16(g_eax+0x8c)=0xffff;
 g_eax=MEM32(g_eax+0x50);g_edx=MEM32(g_eax);g_edi=MEM32(g_esi);g_ecx=g_eax;uint32_t target=MEM32(g_edx+0x1c);PUSH32(g_esp,0x2fc9b2);RECOMP_ICALL_SAFE(target,saved);
 g_eax=0u-g_eax;saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edi+0x14);PUSH32(g_esp,0x2fc9ba);RECOMP_ICALL_SAFE(target,saved);
 POP32(g_esp,g_edi);MEM8(g_esi+0x26)=1;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=8;
}
static void remove_island(void){
 g_ecx=MEM32(g_esp+8);SET_LO8(g_eax,MEM8(g_ecx+0x29));PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM16(g_ecx+0x20);unsigned offset=(uint8_t)g_eax?0x38:0x44;g_eax=MEM32(g_esp+12);
 g_edx=MEM32(g_eax+offset);g_esi=MEM32(g_eax+offset+4);g_esi=MEM32(g_edx+g_esi*4-4);MEM32(g_edx+g_edi*4)=g_esi;SET_LO16(g_edx,MEM16(g_ecx+0x20));g_edi=MEM32(g_eax+offset);g_esi=(uint16_t)g_edx;g_esi=MEM32(g_edi+g_esi*4);MEM16(g_esi+0x20)=(uint16_t)g_edx;--MEM32(g_eax+offset+4);
 SET_LO16(g_edx,MEM16(g_ecx+0x22));int clean=(uint16_t)g_edx==0xffff;POP32(g_esp,g_edi);POP32(g_esp,g_esi);if(!clean){g_eax=MEM32(g_eax+0x50);g_edx=(uint16_t)g_edx;MEM32(g_eax+g_edx*4)=0;MEM16(g_ecx+0x22)=0xffff;}g_recomp_zf=clean;g_esp+=4;
}
static void remove_body(void){
 g_eax=MEM32(g_esp+8);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+0x54);PUSH32(g_esp,g_eax);g_ecx=g_esi;MEM32(g_eax+8)=0;CALL(0x2f6f67,detach);
 if(MEM16(g_esi+0x20)!=0xffff){g_eax=MEM32(g_esi+0x38);if(!g_eax){g_eax=MEM32(g_esp+8);SET_LO8(g_ecx,MEM8(g_eax+0x9c));if((uint8_t)g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);CALL(0x2f6f8b,remove_island);g_eax=MEM32(g_esi);g_esp+=8;uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2f6f96);RECOMP_ICALL_SAFE(target,saved);}}}
 POP32(g_esp,g_esi);g_esp+=4;
}
static void add_body(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=MEM32(g_esp+12);SET_LO8(g_eax,MEM8(frame+0x91));int fixed=(uint8_t)g_eax!=0;PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);MEM32(frame+8)=g_edi;
 if(fixed){g_ecx=MEM32(g_edi+0x30);PUSH32(g_esp,frame);CALL(0x2f6750,attach);POP32(g_esp,g_edi);POP32(g_esp,frame);g_recomp_zf=0;g_esp+=4;return;}
 SET_LO8(g_eax,MEM8(g_edi+0x9c));if((uint8_t)g_eax){
  g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);PUSH32(g_esp,0x2f);PUSH32(g_esp,0x6c);uint32_t target=MEM32(g_eax+0x10);PUSH32(g_esp,0x2f6771);RECOMP_ICALL_SAFE(target,saved);
  PUSH32(g_esp,g_edi);g_ecx=g_eax;MEM16(g_eax+4)=0x6c;CALL(0x2f677f,construct);g_ecx=MEM32(g_esp+24);g_esi=g_eax;g_eax=1;
  if(g_ecx==g_eax){MEM8(g_esi+0x28)=1;MEM8(g_esi+0x29)=1;SET_LO16(g_ecx,MEM16(g_edi+0x3c));g_edi+=0x38;MEM16(g_esi+0x20)=(uint16_t)g_ecx;g_edx=MEM32(g_edi+8)&0x3fffffff;g_eax=MEM32(g_edi+4);if(g_eax==g_edx){PUSH32(g_esp,4);PUSH32(g_esp,g_edi);CALL(0x2f67b7,sub_002E0150);g_esp+=8;}g_eax=MEM32(g_edi+4);g_ecx=MEM32(g_edi);MEM32(g_ecx+g_eax*4)=g_esi;}
  else{MEM8(g_esi+0x28)=0;MEM8(g_esi+0x29)=0;SET_LO16(g_edx,MEM16(g_edi+0x48));g_edi+=0x44;MEM16(g_esi+0x20)=(uint16_t)g_edx;g_eax=MEM32(g_edi+8)&0x3fffffff;g_ecx=MEM32(g_edi+4);if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_edi);CALL(0x2f67ee,sub_002E0150);g_esp+=8;}g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_edi);MEM32(g_edx+g_ecx*4)=g_esi;}
  g_eax=MEM32(g_edi+4)+1;uint32_t count=g_eax;PUSH32(g_esp,frame);g_ecx=g_esi;MEM32(g_edi+4)=g_eax;CALL(0x2f6808,attach);POP32(g_esp,g_esi);POP32(g_esp,g_edi);POP32(g_esp,frame);g_recomp_zf=count==0;g_esp+=4;return;
 }
 int active=MEM32(g_esp+20)==1;if(active){g_eax=MEM32(g_edi+0x38);g_ecx=MEM32(g_eax);PUSH32(g_esp,frame);CALL(0x2f681e,attach);}POP32(g_esp,g_edi);POP32(g_esp,frame);g_recomp_zf=active;g_esp+=4;
}
static void purge(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi+0x18);PUSH32(g_esp,g_edi);g_edi=0;
 if((int32_t)g_eax>0){PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);PUSH32(g_esp,frame);g_eax=0;do{g_ecx=MEM32(g_esi+0x14);if(MEM32(g_eax+g_ecx+4)==g_ebx||MEM32(g_eax+g_ecx+8)==g_ebx){g_edx=--MEM32(g_esi+0x18);g_edx<<=6;frame=MEM32(g_edx+g_ecx);g_edx+=g_ecx;MEM32(g_eax+g_ecx)=frame;for(unsigned o=4;o<=24;o+=4){frame=MEM32(g_edx+o);MEM32(g_eax+g_ecx+o)=frame;}g_xmm0=XMM_MEM(g_edx+32);XMM_STORE(g_eax+g_ecx+32,g_xmm0);g_xmm0=XMM_MEM(g_edx+48);XMM_STORE(g_eax+g_ecx+48,g_xmm0);--g_edi;g_eax-=64;}g_ecx=MEM32(g_esi+0x18);++g_edi;g_eax+=64;}while((int32_t)g_edi<(int32_t)g_ecx);POP32(g_esp,frame);POP32(g_esp,g_ebx);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=4;
}
static int removable(uint32_t w,uint32_t i){
 if(!safe(w,0xa0)||!island_valid(i))return 0;uint32_t h=w+(MEM8(i+0x29)?0x38:0x44),n=MEM32(h+4),p=MEM32(h),k=MEM16(i+0x20),d=MEM16(i+0x22);
 return list_valid(h,4)&&n&&k<n&&MEM32(p+k*4)==i&&safe(MEM32(p+(n-1)*4),0x24)&&(d==0xffff||(d<MEM32(w+0x54)&&safe(MEM32(w+0x50)+d*4,4)));
}
static int detachable(uint32_t i,uint32_t b){if(!island_valid(i)||!body_valid(b))return 0;uint32_t n=MEM32(i+0x38),k=MEM16(b+0x8c),p=MEM32(i+0x34);return n&&k<n&&MEM32(p+k*4)==b&&MEM32(b+0x54)==i&&safe(MEM32(p+(n-1)*4),0x90);}
int uc_migration_physics_body_storage(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BODY_STORAGE");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-256,288))return 0;
 uint32_t a=MEM32(g_esp+4),b=MEM32(g_esp+8);
 if(entry==0x2fce00||entry==0x2fd180){if(!safe(g_ecx,0x6c)||!span(0x51e6f4,4))return 0;uint32_t alloc=MEM32(0x51e6f4);if(!safe(alloc,4)||!safe(MEM32(alloc),24))return 0;const unsigned offsets[]={0x5c,0x44,0x34};for(unsigned i=0;i<3;i++){uint32_t cap=MEM32(g_ecx+offsets[i]+8);if((int32_t)cap>=0){cap&=0x3fffffff;if(cap>65535||(cap&&!safe(MEM32(g_ecx+offsets[i]),cap*4)))return 0;}}if(entry==0x2fce00)release_arrays();else destroy();return 1;}
 if(entry==0x2fc8f0){if(!safe(g_ecx,24))return 0;account();return 1;}
 if(entry==0x2fd0e0){if(!safe(g_ecx,0x6c)||!safe(a,0x80)||!safe(MEM32(a+0x7c),0x1c00))return 0;construct();return 1;}
 if(entry==0x2f7400){if(!safe(a,0x1c))return 0;uint32_t n=MEM32(a+0x18);if(n>65535||(n&&!safe(MEM32(a+0x14),n*64)))return 0;purge();return 1;}
 if(entry==0x2f6910){if(!removable(a,b))return 0;remove_island();return 1;}
 if(entry==0x2fcb70){if(!island_valid(g_ecx)||!body_valid(a))return 0;attach();return 1;}
 if(entry==0x2fc960){if(!detachable(g_ecx,a))return 0;detach();return 1;}
 if(entry!=0x2f6730&&entry!=0x2f6f50)return 0;if(!safe(a,0xa0)||!body_valid(b))return 0;
 if(entry==0x2f6f50){uint32_t i=MEM32(b+0x54);if(!detachable(i,b))return 0;if(MEM16(i+0x20)!=0xffff&&MEM32(i+0x38)==1&&MEM8(a+0x9c)&&!removable(a,i))return 0;remove_body();return 1;}
 if(MEM8(b+0x91)){if(!island_valid(MEM32(a+0x30)))return 0;}
 else if(MEM8(a+0x9c)){if(!safe(MEM32(a+0x7c),0x1c00)||!list_valid(a+0x38,4)||!list_valid(a+0x44,4)||!safe(MEM32(0x51e6f4),4)||!safe(MEM32(MEM32(0x51e6f4)),20))return 0;}
 else if(MEM32(g_esp+12)==1){if(!list_valid(a+0x38,4)||!MEM32(a+0x3c)||!island_valid(MEM32(MEM32(a+0x38))))return 0;}
 add_body();return 1;
}
#undef CALL
