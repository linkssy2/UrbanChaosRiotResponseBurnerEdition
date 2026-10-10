/* Original island sleep transition and activation/deactivation listeners. */
#include "migration_physics_island_events.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);void sub_00389070(void);void sub_002ED490(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,544);}
static int list_valid(uint32_t h,int listeners){
 if(!safe(h,8))return 0;uint32_t n=MEM32(h+4),p=MEM32(h);if(n>65535||(n&&!safe(p,n*4)))return 0;
 if(listeners)for(unsigned i=0;i<n;i++){uint32_t l=MEM32(p+i*4);if(l&&(!safe(l,4)||!safe(MEM32(l),12)))return 0;}return 1;
}
static void compact(void){
 PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax+4)-1;
 if((int32_t)g_esi>=0){PUSH32(g_esp,g_edi);do{g_ecx=MEM32(g_eax);if(!MEM32(g_ecx+g_esi*4)){g_ecx=MEM32(g_eax+4)-1;g_edx=g_ecx;MEM32(g_eax+4)=g_ecx;g_ecx=g_esi;if((int32_t)g_esi<(int32_t)g_edx)do{g_edx=MEM32(g_eax);g_edi=MEM32(g_edx+g_ecx*4+4);g_edx+=g_ecx*4;MEM32(g_edx)=g_edi;g_edx=MEM32(g_eax+4);++g_ecx;}while((int32_t)g_ecx<(int32_t)g_edx);}--g_esi;}while((int32_t)g_esi>=0);POP32(g_esp,g_edi);}
 POP32(g_esp,g_esi);g_recomp_zf=0;g_esp+=4;
}
static void notify(int sleeping){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+8);g_ecx=MEM32(g_ebx+0x88)+1;PUSH32(g_esp,g_esi);MEM32(g_ebx+0x88)=g_ecx;g_esi=MEM32(g_ebx+0xfc)-1;
 if((int32_t)g_esi>=0)do{
  g_eax=MEM32(g_ebx+0xf8);g_ecx=MEM32(g_eax+g_esi*4);if(g_ecx){g_eax=MEM32(g_esp+16);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+(sleeping?8:4));PUSH32(g_esp,sleeping?0x2fb327:0x2fb1d7);RECOMP_ICALL_SAFE(target,saved);}--g_esi;
 }while((int32_t)g_esi>=0);
 g_edx=MEM32(g_ebx+0xfc)-1;PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
 if((int32_t)g_edx>=0)do{g_ecx=MEM32(g_ebx+0xf8);if(!MEM32(g_ecx+g_edx*4)){g_edi=MEM32(g_ebx+0xfc)-1;g_ecx=g_edi;MEM32(g_ebx+0xfc)=g_edi;g_eax=g_edx;if((int32_t)g_edx<(int32_t)g_ecx)do{g_ecx=MEM32(g_ebx+0xf8);g_esi=MEM32(g_ecx+g_eax*4+4);g_ecx+=g_eax*4;MEM32(g_ecx)=g_esi;g_ecx=MEM32(g_ebx+0xfc);++g_eax;}while((int32_t)g_eax<(int32_t)g_ecx);}--g_edx;}while((int32_t)g_edx>=0);
 g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_eax+0x38);frame=0;
 while((int32_t)frame<(int32_t)g_ecx){
  g_edx=MEM32(g_eax+0x34);g_esi=MEM32(g_edx+frame*4);g_edi=MEM32(g_esi+0xa4);g_esi+=0xa0;--g_edi;
  if((int32_t)g_edi>=0)do{g_eax=MEM32(g_esi);g_ecx=MEM32(g_eax+g_edi*4);if(g_ecx){g_eax=MEM32(g_esp+24);g_eax=MEM32(g_eax+0x34);g_eax=MEM32(g_eax+frame*4);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+(sleeping?4:8));PUSH32(g_esp,sleeping?0x2fb3be:0x2fb26e);RECOMP_ICALL_SAFE(target,saved);}--g_edi;}while((int32_t)g_edi>=0);
  g_edx=MEM32(g_esi+4)-1;if((int32_t)g_edx>=0)do{g_ecx=MEM32(g_esi);if(!MEM32(g_ecx+g_edx*4)){g_eax=MEM32(g_esi+4)-1;g_ecx=g_eax;MEM32(g_esi+4)=g_eax;g_eax=g_edx;if((int32_t)g_edx<(int32_t)g_ecx)do{g_ecx=MEM32(g_esi);g_edi=MEM32(g_ecx+g_eax*4+4);g_ecx+=g_eax*4;MEM32(g_ecx)=g_edi;g_ecx=MEM32(g_esi+4);++g_eax;}while((int32_t)g_eax<(int32_t)g_ecx);}--g_edx;}while((int32_t)g_edx>=0);
  g_eax=MEM32(g_esp+24);g_ecx=MEM32(g_eax+0x38);++frame;
 }
 g_eax=MEM32(g_ebx+0x88)-1;POP32(g_esp,g_edi);MEM32(g_ebx+0x88)=g_eax;POP32(g_esp,frame);
 if(!g_eax){g_eax=MEM32(g_ebx+0x84);if(g_eax){SET_LO8(g_eax,MEM8(g_ebx+0x90));if(!(uint8_t)g_eax){POP32(g_esp,g_esi);g_ecx=g_ebx;POP32(g_esp,g_ebx);g_seh_ebp=frame;sub_002ED490();return;}}}
 POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=4;
}
static void sleep_island(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+12);g_eax=MEM32(frame+0x4c);g_ecx=MEM32(frame+0x48);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=frame+0x44;g_eax&=0x3fffffff;
 if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_edi);CALL(0x2f6bd2,sub_002E0150);g_esp+=8;}
 g_ecx=MEM32(g_edi+4);g_edx=MEM32(g_edi);g_esi=MEM32(g_esp+24);MEM32(g_edx+g_ecx*4)=g_esi;++MEM32(g_edi+4);g_eax=MEM32(frame+0x38);g_ecx=MEM32(frame+0x3c);g_ecx=MEM32(g_eax+g_ecx*4-4);g_edx=MEM16(g_esi+0x20);MEM32(g_eax+g_edx*4)=g_ecx;SET_LO16(g_eax,MEM16(g_esi+0x20));g_ecx=MEM32(frame+0x38);g_edx=(uint16_t)g_eax;g_edx=MEM32(g_ecx+g_edx*4);MEM16(g_edx+0x20)=(uint16_t)g_eax;--MEM32(frame+0x3c);SET_LO16(g_eax,MEM16(frame+0x48));SET_LO16(g_eax,(uint16_t)g_eax-1);MEM16(g_esi+0x20)=(uint16_t)g_eax;
 g_ebx=0;MEM8(g_esi+0x29)=0;g_ecx=MEM32(frame+0x150);MEM32(g_esi+0x68)=g_ecx;
 if((int32_t)MEM32(g_esi+0x38)>0)do{
  g_edx=MEM32(g_esi+0x34);g_edi=MEM32(g_edx+g_ebx*4);g_eax=MEM32(g_edi+0x50);g_ecx=MEM32(g_esi+0x68);g_eax+=16;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);CALL(0x2f6c3c,sub_00389070);
  g_ecx=MEM32(g_edi+0x50);g_edx=MEM32(g_ecx);g_esp+=8;uint32_t saved=g_esp;PUSH32(g_esp,0x51e7c0);uint32_t target=MEM32(g_edx+0x54);PUSH32(g_esp,0x2f6c4c);RECOMP_ICALL_SAFE(target,saved);
  g_ecx=MEM32(g_edi+0x50);g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0x51e7c0);target=MEM32(g_eax+0x58);PUSH32(g_esp,0x2f6c59);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_esi+0x38);++g_ebx;
 }while((int32_t)g_ebx<(int32_t)g_eax);
 PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);PUSH32(g_esp,0x2f6c68);notify(1);g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_island_events(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_ISLAND_EVENTS");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-512,544))return 0;
 if(entry==0x2f5f40){if(!list_valid(g_eax,0))return 0;compact();return 1;}
 if(entry!=0x2fb1a0&&entry!=0x2fb2f0&&entry!=0x2f6bb0)return 0;
 uint32_t w=MEM32(g_esp+4),p=MEM32(g_esp+8);if(!safe(w,0x154)||!safe(p,0x6c)||!list_valid(w+0xf8,1)||!list_valid(p+0x34,0))return 0;
 for(unsigned i=0;i<MEM32(p+0x38);i++){uint32_t b=MEM32(MEM32(p+0x34)+i*4);if(!safe(b,0xa8)||!list_valid(b+0xa0,1))return 0;if(entry==0x2f6bb0){uint32_t m=MEM32(b+0x50);if(!safe(m,32)||!safe(MEM32(m),0x5c))return 0;}}
 if(entry==0x2f6bb0){uint32_t index=MEM16(p+0x20),n=MEM32(w+0x3c),cap=MEM32(w+0x4c)&0x3fffffff;if(!list_valid(w+0x38,0)||!list_valid(w+0x44,0)||index>=n||MEM32(MEM32(w+0x38)+index*4)!=p||!MEM8(p+0x29)||MEM32(w+0x48)>cap||cap>65535||(cap&&!safe(MEM32(w+0x44),cap*4)))return 0;for(unsigned i=0;i<n;i++)if(!safe(MEM32(MEM32(w+0x38)+i*4),0x24))return 0;sleep_island();}
 else notify(entry==0x2fb2f0);return 1;
}
#undef CALL
