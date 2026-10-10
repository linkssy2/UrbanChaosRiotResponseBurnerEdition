/* Original island merge: body indices, constraint ownership and world lists. */
#include "migration_physics_islands.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E00E0(void);void sub_002E0150(void);void sub_002ED490(void);void sub_002ED500(void);
void sub_0032A450(void);void sub_003889E0(void);void sub_002FE060(void);void sub_002FB1A0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int apart(uint32_t p,uint32_t n,uint32_t q,uint32_t m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&apart(p,n,g_esp-512,544);}
static int array_valid(uint32_t h){uint32_t n=MEM32(h+4),cap=MEM32(h+8)&0x3fffffff;return n<=cap&&cap<=65535&&(!cap||safe(MEM32(h),cap*4));}
static int island_valid(uint32_t p){
 if(!safe(p,0x6c)||!safe(MEM32(p),4)||!array_valid(p+0x34)||!array_valid(p+0x5c))return 0;
 uint32_t n=MEM32(p+0x38),base=MEM32(p+0x34);
 for(unsigned i=0;i<n;i++){uint32_t b=MEM32(base+i*4);if(!safe(b,0x90)||MEM32(b+0x6c)>65535||!safe(MEM32(b+0x50),32))return 0;uint32_t c=MEM32(b+0x6c),r=MEM32(b+0x68);if(c&&!safe(r,c*28))return 0;for(unsigned j=0;j<c;j++)if(!safe(MEM32(r+j*28),12))return 0;}
 n=MEM32(p+0x60);base=MEM32(p+0x5c);for(unsigned i=0;i<n;i++){uint32_t q=MEM32(base+i*4);if(q&&!safe(q,16))return 0;}return 1;
}
static int world_valid(uint32_t w){
 if(!safe(w,0x154))return 0;
 for(unsigned h=0x38;h<=0x50;h+=12){if(!array_valid(w+h))return 0;for(unsigned i=0;i<MEM32(w+h+4);i++){uint32_t p=MEM32(MEM32(w+h)+i*4);if(p&&!safe(p,0x24))return 0;}}
 return 1;
}
static int membership(uint32_t w,uint32_t p){uint32_t h=w+(MEM8(p+0x29)?0x38:0x44),i=MEM16(p+0x20),q=MEM16(p+0x22);return i<MEM32(h+4)&&MEM32(MEM32(h)+i*4)==p&&(q==65535||(q<MEM32(w+0x54)&&MEM32(MEM32(w+0x50)+q*4)==p));}
static void activate(void){
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+16);g_eax=MEM32(g_edi+0x40)&0x3fffffff;g_ecx=MEM32(g_edi+0x3c);g_ebx=g_edi+0x38;
 if(g_ecx==g_eax){PUSH32(g_esp,4);PUSH32(g_esp,g_ebx);CALL(0x2f6b01,sub_002E0150);g_esp+=8;}
 g_ecx=MEM32(g_ebx+4);g_edx=MEM32(g_ebx);g_esi=MEM32(g_esp+20);MEM32(g_edx+g_ecx*4)=g_esi;++MEM32(g_ebx+4);
 g_eax=MEM32(g_edi+0x44);g_ecx=MEM32(g_edi+0x48);g_ecx=MEM32(g_eax+g_ecx*4-4);g_edx=MEM16(g_esi+0x20);MEM32(g_eax+g_edx*4)=g_ecx;SET_LO16(g_eax,MEM16(g_esi+0x20));g_ecx=MEM32(g_edi+0x44);g_edx=(uint16_t)g_eax;g_edx=MEM32(g_ecx+g_edx*4);MEM16(g_edx+0x20)=(uint16_t)g_eax;--MEM32(g_edi+0x48);
 SET_LO16(g_eax,MEM16(g_edi+0x3c));SET_LO16(g_eax,(uint16_t)g_eax-1);MEM16(g_esi+0x20)=(uint16_t)g_eax;MEM8(g_esi+0x29)=1;g_eax=MEM32(g_esi+0x38);g_ebx=0;MEM32(g_esi+0x2c)=MEM32(g_esi+0x30)=0;
 while((int32_t)g_ebx<(int32_t)g_eax){g_ecx=MEM32(g_esi+0x34);g_edx=MEM32(g_ecx+g_ebx*4);g_eax=MEM32(g_edx+0x50)+16;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);PUSH32(g_esp,0);CALL(0x2f6b76,sub_003889E0);g_eax=MEM32(g_esi+0x38);g_esp+=12;++g_ebx;}
 g_ecx=MEM32(g_edi+0x74);g_edx=MEM32(g_edi+0x150);g_eax=MEM32(g_esi+0x68);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);CALL(0x2f6b96,sub_002FE060);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);CALL(0x2f6b9d,sub_002FB1A0);g_esp+=24;
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;g_esp+=4;
}
static void merge(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);g_ecx=MEM32(0x520208);
 if(g_ecx<MEM32(0x52020c)){MEM32(g_ecx)=0x49aa48;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp)=g_eax;g_eax=MEM32(g_esp);MEM32(g_ecx+4)=g_eax;g_ecx+=12;MEM32(0x520208)=g_ecx;}
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+28);g_eax=MEM32(g_esi+0x38);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);g_ecx=MEM32(g_edi+0x38);if((int32_t)g_ecx<(int32_t)g_eax){g_eax=g_edi;g_edi=g_esi;g_esi=g_eax;}
 g_eax=MEM32(g_esp+24);++MEM32(g_eax+0x88);SET_LO8(g_ecx,MEM8(g_edi+0x29));if(!(uint8_t)g_ecx)SET_LO8(g_edx,MEM8(g_esi+0x29));SET_LO8(g_edx,((uint8_t)g_ecx||(uint8_t)g_edx)?1:0);
 SET_LO8(g_ebx,MEM8(g_edi+0x28));if(!(uint8_t)g_ebx){SET_LO8(g_ebx,MEM8(g_esi+0x28));MEM8(g_esp+28)=0;}if((uint8_t)g_ebx)MEM8(g_esp+28)=1;
 if((uint8_t)g_edx){
  if(!(uint8_t)g_ecx){PUSH32(g_esp,g_edi);PUSH32(g_esp,g_eax);MEM8(g_edi+0x28)=1;CALL(0x2f702e,activate);SET_LO8(g_edx,MEM8(g_esi+0x24));MEM8(g_edi+0x24)=(uint8_t)g_edx;SET_LO8(g_eax,MEM8(g_esi+0x25));g_esp+=8;MEM8(g_edi+0x25)=(uint8_t)g_eax;}
  else{SET_LO8(g_ecx,MEM8(g_esi+0x29));if(!(uint8_t)g_ecx){PUSH32(g_esp,g_esi);PUSH32(g_esp,g_eax);MEM8(g_esi+0x28)=1;CALL(0x2f704e,activate);g_esp+=8;}
   else{SET_LO8(g_eax,MEM8(g_esi+0x24));SET_LO8(g_ecx,MEM8(g_edi+0x24));if((uint8_t)g_ecx<(uint8_t)g_eax)SET_LO8(g_eax,(uint8_t)g_ecx);SET_LO8(g_ecx,MEM8(g_edi+0x25));MEM8(g_edi+0x24)=(uint8_t)g_eax;SET_LO8(g_eax,MEM8(g_esi+0x25));if((uint8_t)g_ecx<(uint8_t)g_eax)SET_LO8(g_eax,(uint8_t)g_ecx);MEM8(g_edi+0x25)=(uint8_t)g_eax;}}
 }
 g_ecx=g_esi+0x44;PUSH32(g_esp,g_ecx);g_edx=g_edi+0x44;PUSH32(g_esp,g_edx);CALL(0x2f707e,sub_0032A450);
 g_ebx=MEM32(g_edi+0x38);frame=MEM32(g_esi+0x38);g_eax=MEM32(g_edi+0x3c);g_ecx=g_edi+0x34;frame+=g_ebx;g_eax&=0x3fffffff;g_esp+=8;
 if((int32_t)g_eax<(int32_t)frame){g_eax*=2;if((int32_t)frame>=(int32_t)g_eax)g_eax=frame;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);CALL(0x2f70a9,sub_002E00E0);g_esp+=12;}
 MEM32(g_edi+0x38)=frame;g_ecx=MEM32(g_esi+0x38);g_eax=0;
 while((int32_t)g_eax<(int32_t)g_ecx){g_ecx=MEM32(g_esi+0x34);g_ecx=MEM32(g_ecx+g_eax*4);frame=MEM32(g_edi+0x34);g_edx=(uint16_t)g_ebx;MEM32(frame+g_edx*4)=g_ecx;g_edx=MEM32(g_esi+0x34);g_ecx=MEM32(g_edx+g_eax*4);MEM32(g_ecx+0x54)=g_edi;g_edx=MEM32(g_esi+0x34);g_ecx=MEM32(g_edx+g_eax*4);MEM16(g_ecx+0x8c)=(uint16_t)g_ebx;g_ecx=MEM32(g_esi+0x38);++g_eax;++g_ebx;}
 frame=MEM32(g_edi+0x60);g_eax=MEM32(g_esi+0x60);g_ecx=MEM32(g_edi+0x64);g_ebx=g_edi+0x5c;g_eax+=frame;g_ecx&=0x3fffffff;MEM32(g_esp+32)=g_eax;
 if((int32_t)g_ecx<(int32_t)g_eax){g_ecx*=2;if((int32_t)g_eax<(int32_t)g_ecx)g_eax=g_ecx;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x2f711c,sub_002E00E0);g_eax=MEM32(g_esp+44);g_esp+=12;}
 MEM32(g_ebx+4)=g_eax;g_eax=MEM32(g_esi+0x60);g_ecx=0;
 while((int32_t)g_ecx<(int32_t)g_eax){g_edx=MEM32(g_esi+0x5c);g_eax=MEM32(g_edx+g_ecx*4);if(g_eax){g_edx=MEM32(g_ebx);MEM32(g_edx+frame*4)=g_eax;g_eax=MEM32(g_ebx);g_edx=MEM32(g_eax+frame*4);MEM32(g_edx+12)=g_edi;++frame;}g_eax=MEM32(g_esi+0x60);++g_ecx;}
 g_eax=MEM32(g_ebx+8)&0x3fffffff;if((int32_t)g_eax<(int32_t)frame){g_eax*=2;if((int32_t)frame>=(int32_t)g_eax)g_eax=frame;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);CALL(0x2f716d,sub_002E00E0);g_esp+=12;}
 MEM32(g_ebx+4)=frame;g_eax=MEM32(g_esi+0x38);g_edx=0;
 while((int32_t)g_edx<(int32_t)g_eax){g_eax=MEM32(g_esi+0x34);g_ecx=MEM32(g_eax+g_edx*4);g_eax=MEM32(g_ecx+0x68);g_ecx=MEM32(g_ecx+0x6c)-1;if((int32_t)g_ecx>=0){++g_ecx;do{g_ebx=MEM32(g_eax);g_eax+=28;--g_ecx;MEM32(g_ebx+8)=g_edi;}while(g_ecx);}g_eax=MEM32(g_esi+0x38);++g_edx;}
 g_eax=MEM32(g_esi+8);g_ecx=MEM32(g_edi+8);if((int32_t)g_ecx>(int32_t)g_eax)g_eax=g_ecx;
 frame=MEM32(g_edi+16);g_ebx=MEM32(g_edi+24);MEM32(g_edi+8)=g_eax;g_ecx=MEM32(g_esi+12);MEM32(g_edi+12)+=g_ecx;g_edx=MEM32(g_esi+16);frame+=g_edx;g_edx=MEM32(g_edi+20);MEM32(g_edi+16)=frame;g_eax=MEM32(g_esi+24);g_ebx+=g_eax;MEM32(g_edi+24)=g_ebx;g_ecx=MEM32(g_esi+20);g_edx+=g_ecx;MEM32(g_edi+20)=g_edx;
 SET_LO8(g_eax,MEM8(g_esi+0x29));SET_LO16(g_edx,MEM16(g_esi+0x20));int active=(uint8_t)g_eax!=0;g_eax=MEM32(g_esp+24)+(active?0x38:0x44);frame=MEM32(g_eax+4);g_ecx=(uint16_t)g_edx;g_ebx=frame-1;
 if((int32_t)g_ecx<(int32_t)g_ebx){g_ebx=MEM32(g_eax);frame=MEM32(g_ebx+frame*4-4);MEM32(g_ebx+g_ecx*4)=frame;g_ebx=MEM32(g_eax);g_ecx=MEM32(g_ebx+g_ecx*4);MEM16(g_ecx+0x20)=(uint16_t)g_edx;}--MEM32(g_eax+4);
 SET_LO8(g_eax,MEM8(g_edi+0x26));if(!(uint8_t)g_eax)SET_LO8(g_eax,MEM8(g_esi+0x26));SET_LO8(g_eax,(uint8_t)g_eax!=0);MEM8(g_edi+0x26)=(uint8_t)g_eax;
 SET_LO8(g_eax,MEM8(g_edi+0x27));if(!(uint8_t)g_eax)SET_LO8(g_eax,MEM8(g_esi+0x27));SET_LO8(g_eax,(uint8_t)g_eax!=0);SET_LO8(g_edx,MEM8(g_esp+28));MEM8(g_edi+0x27)=(uint8_t)g_eax;g_ecx=65535;MEM8(g_edi+0x28)=(uint8_t)g_edx;
 if(MEM16(g_esi+0x22)!=65535&&MEM16(g_edi+0x22)==65535){frame=MEM32(g_esp+24);SET_LO16(g_eax,MEM16(frame+0x54));g_ebx=frame+0x50;MEM16(g_edi+0x22)=(uint16_t)g_eax;g_edx=MEM32(g_ebx+8);g_eax=MEM32(g_ebx+4);g_edx&=0x3fffffff;if(g_eax==g_edx){PUSH32(g_esp,4);PUSH32(g_esp,g_ebx);CALL(0x2f727f,sub_002E0150);g_esp+=8;g_ecx=65535;}g_eax=MEM32(g_ebx+4);g_edx=MEM32(g_ebx);MEM32(g_edx+g_eax*4)=g_edi;++MEM32(g_ebx+4);}
 else frame=MEM32(g_esp+24);
 SET_LO16(g_eax,MEM16(g_esi+0x22));if((uint16_t)g_eax!=(uint16_t)g_ecx){g_edx=MEM32(frame+0x50);g_eax=(uint16_t)g_eax;MEM32(g_edx+g_eax*4)=0;MEM16(g_esi+0x22)=(uint16_t)g_ecx;}
 g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x2f72ba);RECOMP_ICALL_SAFE(target,saved);
 g_ecx=MEM32(0x520208);if(g_ecx<MEM32(0x52020c)){MEM32(g_ecx)=0x49895c;uint64_t t=recomp_read_tsc();g_eax=(uint32_t)t;g_edx=(uint32_t)(t>>32);MEM32(g_esp+24)=g_eax;g_edx=MEM32(g_esp+24);MEM32(g_ecx+4)=g_edx;g_ecx+=12;MEM32(0x520208)=g_ecx;}
 --MEM32(frame+0x88);if(!MEM32(frame+0x88)){g_eax=MEM32(frame+0x84);if(g_eax){SET_LO8(g_eax,MEM8(frame+0x90));if(!(uint8_t)g_eax){g_ecx=frame;CALL(0x2f7307,sub_002ED490);}}}
 g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_esp+=4;
}
static void request_merge(void){
 g_ecx=MEM32(g_esp+4);g_eax=MEM32(g_esp+8);g_edx=MEM32(g_eax+0x54);g_esp-=12;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_ecx+0x88);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+32);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esi+0x54);
 if(g_ebx){MEM32(g_esp+16)=g_eax;g_eax=g_esp+12;PUSH32(g_esp,g_eax);MEM8(g_esp+16)=12;MEM32(g_esp+24)=g_esi;CALL(0x2f7c49,sub_002ED500);}
 else{PUSH32(g_esp,g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ecx);CALL(0x2f7c58,merge);g_esp+=12;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_physics_islands(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_ISLANDS");enabled=v&&!strcmp(v,"1");}
 if(!enabled||!span(g_esp-512,544)||!span(0x520208,8))return 0;
 uint32_t w=MEM32(g_esp+4),a=MEM32(g_esp+8),b=MEM32(g_esp+12);if(!world_valid(w))return 0;
 if(entry==0x2f7c10){if(!safe(a,0x58)||!safe(b,0x58))return 0;a=MEM32(a+0x54);b=MEM32(b+0x54);}
 else if(entry!=0x2f6fa0&&entry!=0x2f6ae0)return 0;
 if(!island_valid(a)||!membership(w,a))return 0;
 if(entry==0x2f6ae0){if(MEM8(a+0x29))return 0;activate();return 1;}
 if(a==b||!island_valid(b)||!membership(w,b)||MEM32(a+0x38)+MEM32(b+0x38)>65535||MEM32(a+0x60)+MEM32(b+0x60)>65535)return 0;
 uint32_t p=MEM32(0x520208),end=MEM32(0x52020c);if(p<end&&(!safe(p,24)||end-p>0x100000))return 0;
 if(entry==0x2f7c10)request_merge();else merge();return 1;
}
#undef CALL
