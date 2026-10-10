/* Original ragdoll construction: body/joint remapping and authored bone properties. */
#include "migration_physics_ragdoll_construct.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002A4FF0(void);void sub_002E00E0(void);void sub_002F1FE0(void);void sub_00208C10(void);void sub_002F12A0(void);void sub_002F1030(void);void sub_002A4F90(void);void sub_002A3190(void);void sub_002A3120(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-256||(uint64_t)g_esp+32<=p);}
static void grow(unsigned array,unsigned needed,unsigned ret){g_eax+=g_eax;PUSH32(g_esp,4);if((int32_t)needed>=(int32_t)g_eax)g_eax=needed;PUSH32(g_esp,g_eax);PUSH32(g_esp,array);CALL(ret,sub_002E00E0);g_esp+=12;}
static void readable_name(uint32_t body){g_edx=MEM32(body+16);g_ecx=g_esp+28;g_eax=g_ecx;g_edx-=g_eax;for(;;){SET_LO8(g_eax,MEM8(g_ecx+g_edx));MEM8(g_ecx)=(uint8_t)g_eax;if((uint8_t)g_eax=='_')MEM8(g_ecx)=' ';if(!(uint8_t)g_eax)break;++g_ecx;}}
static void construct(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x3a1712);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=44;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ebx=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_ebx;g_esi=0;MEM32(g_ebx)=0x48dd60;g_eax=0x80000000;MEM32(g_esp+68)=0;MEM32(g_ebx+4)=MEM32(g_ebx+8)=0;MEM32(g_ebx+12)=g_eax;MEM32(g_ebx+16)=MEM32(g_ebx+20)=0;MEM32(g_ebx+24)=g_eax;g_edi=g_ebx+112;MEM32(g_edi)=MEM32(g_edi+4)=0;MEM32(g_edi+8)=g_eax;frame=g_ebx+124;MEM32(frame)=MEM32(frame+4)=0;MEM32(frame+8)=g_eax;MEM32(g_ebx+140)=MEM32(g_ebx+144)=0;MEM32(g_ebx+148)=g_eax;MEM8(g_esp+68)=5;CALL(0x2a61a8,sub_002A4FF0);
 g_eax=MEM32(g_esp+76);g_ecx=MEM32(g_esp+80);MEM32(g_ebx+136)=g_eax;MEM32(g_ebx+28)=g_esi;MEM32(g_ebx+32)=g_esi;MEM8(g_ebx+36)=0;MEM8(g_ebx+152)=0;g_eax=MEM32(g_edi+8)&0x3fffffff;g_esi=MEM32(g_ecx+4);if((int32_t)g_eax<(int32_t)g_esi)grow(g_edi,g_esi,0x2a61e6);g_edx=MEM32(g_esp+80);MEM32(g_edi+4)=g_esi;g_eax=MEM32(g_edx+4);MEM32(g_esp+76)=0;
 while((int32_t)MEM32(g_esp+76)<(int32_t)g_eax){
  g_esi=MEM32(g_esp+76)<<2;g_eax=MEM32(g_edi)+g_esi;MEM32(g_esp+24)=g_eax;g_eax=MEM32(g_esp+80);g_ecx=MEM32(g_eax);g_ecx=MEM32(g_esi+g_ecx);CALL(0x2a621d,sub_002F1FE0);g_edx=MEM32(g_esp+24);MEM32(g_edx)=g_eax;g_eax=MEM32(g_esp+80);g_ecx=MEM32(g_eax);g_edx=MEM32(g_esi+g_ecx);g_eax=MEM32(g_edi);g_ecx=MEM32(g_esi+g_eax);g_edx=MEM32(g_edx+16);MEM32(g_ecx+16)=g_edx;g_ecx=MEM32(0x51c4dc);CALL(0x2a6242,sub_00208C10);g_ecx=MEM32(g_edi);g_edx=MEM32(g_esi+g_ecx);g_ecx=MEM32(g_esp+80);g_eax|=0x24800000;MEM32(g_edx+48)=g_eax;g_eax=MEM32(g_esp+76);g_edx=MEM32(g_ecx+4);++g_eax;MEM32(g_esp+76)=g_eax;if((int32_t)g_eax>=(int32_t)g_edx)break;g_eax=g_edx;
 }
 g_esi=MEM32(g_esp+84);g_eax=MEM32(frame+8)&0x3fffffff;g_edi=MEM32(g_esi+4);if((int32_t)g_eax<(int32_t)g_edi)grow(frame,g_edi,0x2a6286);MEM32(frame+4)=g_edi;g_eax=MEM32(g_esi+4);MEM32(g_esp+16)=0;
 while((int32_t)MEM32(g_esp+16)<(int32_t)g_eax){
  g_edi=MEM32(g_esp+16)<<2;g_edx=MEM32(g_esi);g_eax=MEM32(MEM32(g_esp+80)+4);g_ecx=MEM32(g_edi+g_edx);g_esi=MEM32(g_ecx+16);g_edx=0;MEM32(g_esp+76)=g_ecx;uint32_t first=0xffffffff;
  if((int32_t)g_eax>0){g_ecx=MEM32(MEM32(g_esp+80));do{if(MEM32(g_ecx)==g_esi){first=g_edx;break;}++g_edx;g_ecx+=4;}while((int32_t)g_edx<(int32_t)g_eax);g_ecx=MEM32(g_esp+76);}MEM32(g_esp+24)=first;MEM32(g_esp+76)=0;int found=0;
  if((int32_t)g_eax>0){g_edx=MEM32(MEM32(g_esp+80));do{g_esi=MEM32(g_ecx+20);if(MEM32(g_edx)==g_esi){g_edx=MEM32(g_esp+76);found=1;break;}g_esi=MEM32(g_esp+76)+1;g_edx+=4;MEM32(g_esp+76)=g_esi;}while((int32_t)g_esi<(int32_t)g_eax);}if(!found)g_edx=0xffffffff;
  g_eax=MEM32(g_ebx+112);g_edx=MEM32(g_eax+g_edx*4);g_esi=MEM32(frame);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+28);g_eax=MEM32(g_eax+g_edx*4);PUSH32(g_esp,g_eax);g_esi+=g_edi;CALL(0x2a6337,sub_002F12A0);MEM32(g_esi)=g_eax;g_esi=MEM32(g_esp+84);g_ecx=MEM32(g_esi);g_edx=MEM32(g_edi+g_ecx);g_eax=MEM32(frame);g_ecx=MEM32(g_eax+g_edi);g_edx=MEM32(g_edx+28);MEM32(g_ecx+28)=g_edx;g_eax=MEM32(frame);g_ecx=MEM32(g_eax+g_edi);PUSH32(g_esp,2);CALL(0x2a635b,sub_002F1030);g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esi+4);++g_eax;MEM32(g_esp+16)=g_eax;if((int32_t)g_eax>=(int32_t)g_ecx)break;g_esi=MEM32(g_esp+84);g_eax=g_ecx;
 }
 g_eax=MEM32(g_ebx+12)&0x3fffffff;g_esi=MEM32(g_ebx+116);g_edi=g_ebx+4;if((int32_t)g_eax<(int32_t)g_esi)grow(g_edi,g_esi,0x2a6391);MEM32(g_edi+4)=g_esi;g_eax=MEM32(g_ebx+24)&0x3fffffff;g_esi=g_ebx+16;if((int32_t)g_eax<73){g_eax+=g_eax;if((int32_t)g_eax<=73)g_eax=73;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);CALL(0x2a63bc,sub_002E00E0);g_esp+=12;}MEM32(g_esi+4)=73;g_eax=0;do{g_ecx=MEM32(g_esi);MEM32(g_eax+g_ecx)=0xffffffff;g_eax+=4;}while(g_eax<292);g_eax=MEM32(g_ebx+116);g_esi=0;
 while((int32_t)g_esi<(int32_t)g_eax){g_edx=MEM32(g_ebx+112);frame=MEM32(g_edx+g_esi*4);readable_name(frame);g_eax=g_esp+28;CALL(0x2a6433,sub_002A4F90);PUSH32(g_esp,0);PUSH32(g_esp,33);PUSH32(g_esp,frame);g_edi=g_eax;CALL(0x2a643f,sub_002A3190);PUSH32(g_esp,0);PUSH32(g_esp,45);PUSH32(g_esp,frame);CALL(0x2a6449,sub_002A3190);g_ecx=MEM32(g_ebx+4);MEM32(g_ecx+g_esi*4)=g_edi;g_edx=MEM32(g_ebx+16);g_esp+=24;MEM32(g_edx+g_edi*4)=g_esi;g_eax=MEM32(g_ebx+116);++g_esi;}
 g_eax=MEM32(g_ebx+116);g_ecx=0;MEM32(g_esp+84)=0;if((int32_t)g_eax<=0){POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=g_ebx;POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+44);MEM32(0)=g_ecx;g_esp+=56;g_recomp_zf=g_esp==0;g_esp+=16;return;}
 for(;;){
  g_eax=MEM32(g_ebx+112);frame=MEM32(g_eax+g_ecx*4);readable_name(frame);g_eax=g_esp+28;CALL(0x2a64bc,sub_002A4F90);g_esi=g_eax;MEM32(g_esp+76)=g_esi;g_ebx=0x503bbc;MEM32(g_esp+80)=12;
  do{g_eax=MEM32(g_ebx-4);CALL(0x2a64dd,sub_002A4F90);if(g_eax==g_esi){g_eax=MEM32(g_ebx);g_edi=0;if(g_eax){do{g_eax=MEM32(g_ebx+g_edi*4+4);++g_edi;}while(g_eax);}PUSH32(g_esp,4);PUSH32(g_esp,1);PUSH32(g_esp,frame);CALL(0x2a6503,sub_002A3120);PUSH32(g_esp,g_esi);PUSH32(g_esp,3);PUSH32(g_esp,frame);CALL(0x2a650c,sub_002A3120);PUSH32(g_esp,g_edi);PUSH32(g_esp,4);PUSH32(g_esp,frame);CALL(0x2a6515,sub_002A3120);g_esp+=36;g_esi=0;while((int32_t)g_esi<(int32_t)g_edi){g_eax=MEM32(g_ebx+g_esi*4);CALL(0x2a6528,sub_002A4F90);PUSH32(g_esp,g_eax);g_ecx=g_esi+5;PUSH32(g_esp,g_ecx);PUSH32(g_esp,frame);CALL(0x2a6533,sub_002A3120);g_esp+=12;++g_esi;}}
   g_eax=MEM32(g_esp+80);g_ebx+=36;--g_eax;MEM32(g_esp+80)=g_eax;if(!g_eax)break;g_esi=MEM32(g_esp+76);
  }while(1);
  g_ecx=MEM32(g_esp+84);g_edx=MEM32(g_esp+20);g_eax=MEM32(g_edx+116);++g_ecx;MEM32(g_esp+84)=g_ecx;if((int32_t)g_ecx>=(int32_t)g_eax)break;g_ebx=MEM32(g_esp+20);
 }
 g_ecx=MEM32(g_esp+60);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=g_edx;POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=56;g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_physics_ragdoll_construct(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_RAGDOLL_CONSTRUCT");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x2a6130||!span(g_esp-256,288)||!safe(g_ecx,156)||!span(0x503bb8,432)||!span(0x51c4dc,4))return 0;
 for(unsigned k=0;k<2;k++){uint32_t list=MEM32(g_esp+8+k*4);if(!safe(list,8))return 0;uint32_t n=MEM32(list+4),items=MEM32(list);if(n>4096||(n&&!safe(items,n*4)))return 0;for(unsigned i=0;i<n;i++){uint32_t item=MEM32(items+i*4);if(!safe(item,52))return 0;if(!k){uint32_t name=MEM32(item+16);int terminated=0;for(unsigned c=0;c<32;c++){if(!span(name+c,1))return 0;if(!MEM8(name+c)){terminated=1;break;}}if(!terminated)return 0;}}}
 construct();return 1;
}
#undef CALL
