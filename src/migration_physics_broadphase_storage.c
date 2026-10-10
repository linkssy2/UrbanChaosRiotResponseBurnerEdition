/* Original broadphase pair/partition storage and ordered endpoint maintenance. */
#include "migration_physics_broadphase_storage.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);void sub_002E00E0(void);
#define CALL(ret,fn) do{PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+28<=p);}
static int array(uint32_t p,unsigned stride){if(!safe(p,12))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff;return n<=65535&&cap<=65535&&n<=cap&&(!cap||safe(MEM32(p),cap*stride));}
static void pair_record(int remove){
 PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+12);g_eax=MEM32(g_ebx+12);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_edx;
 if(!(g_eax&1)){g_esi=MEM32(g_esp+24);g_eax=MEM32(g_esi+8)&0x3fffffff;g_ecx=MEM32(g_esi+4);if(g_ecx==g_eax){PUSH32(g_esp,8);PUSH32(g_esp,g_esi);CALL(remove?0x32462b:0x32459b,sub_002E0150);g_esp+=8;}g_eax=MEM32(g_esi+4);g_ecx=MEM32(g_esi)+g_eax*8;++g_eax;MEM32(g_esi+4)=g_eax;uint32_t count=g_eax;g_edx=MEM32(g_edi+12);POP32(g_esp,g_edi);MEM32(g_ecx)=g_edx;g_eax=MEM32(g_ebx+12);POP32(g_esp,g_esi);MEM32(g_ecx+4)=g_eax;POP32(g_esp,g_ebx);g_recomp_zf=count==0;g_esp+=16;return;}
 g_eax&=~1u;
 if(!remove){g_esi=g_eax+g_ecx+4;g_ecx=MEM32(g_esi+8)&0x3fffffff;g_eax=MEM32(g_esi+4);if(g_eax==g_ecx){PUSH32(g_esp,2);PUSH32(g_esp,g_esi);CALL(0x3245da,sub_002E0150);g_esp+=8;}g_edx=MEM32(g_esi+4);g_eax=MEM32(g_esi);SET_LO16(g_ecx,MEM16(g_esp+16));MEM16(g_eax+g_edx*2)=(uint16_t)g_ecx;g_eax=MEM32(g_esi+4)+1;POP32(g_esp,g_edi);MEM32(g_esi+4)=g_eax;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=g_eax==0;}
 else{g_edx=MEM32(g_eax+g_ecx+8);g_eax+=g_ecx;g_ecx=0;if((int32_t)g_edx>0){g_esi=MEM32(g_eax+4);g_edi=MEM32(g_esp+16);while(MEM16(g_esi)!=(uint16_t)g_edi){++g_ecx;g_esi+=2;if((int32_t)g_ecx>=(int32_t)g_edx)break;}if((int32_t)g_ecx>=(int32_t)g_edx)g_ecx=0xffffffff;}else g_ecx=0xffffffff;g_edx=MEM32(g_eax+8)-1;uint32_t count=g_edx;MEM32(g_eax+8)=g_edx;g_eax=MEM32(g_eax+4);SET_LO16(g_edx,MEM16(g_eax+g_edx*2));POP32(g_esp,g_edi);POP32(g_esp,g_esi);MEM16(g_eax+g_ecx*2)=(uint16_t)g_edx;POP32(g_esp,g_ebx);g_recomp_zf=count==0;}
 g_esp+=16;
}
static void compact_endpoints(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,frame);frame=g_ecx;g_ecx=MEM32(frame);g_eax=MEM32(frame+4);PUSH32(g_esp,g_esi);g_edx=g_ecx+g_eax*4;PUSH32(g_esp,g_edi);g_esi=g_ecx;g_edi=0;MEM32(g_esp+12)=g_edx;
 if(g_esi<g_edx){PUSH32(g_esp,g_ebx);do{g_eax=MEM16(g_esi+2);g_ebx=MEM32(g_esp+32);g_ebx=MEM32(g_ebx);g_eax=MEM32(g_ebx+g_eax*4);if((int32_t)g_eax>=0){g_edx=MEM32(g_esi);g_ebx=MEM32(g_esp+28);MEM32(g_ecx)=g_edx;g_edx=0;SET_LO8(g_edx,MEM8(g_ecx));MEM16(g_ecx+2)=(uint16_t)g_eax;g_eax<<=4;g_ecx+=4;g_edx&=1;g_edx+=g_ebx*2;g_ebx=MEM32(g_edx*4+0x505cd4);g_edx=MEM32(g_esp+24);g_eax+=g_ebx;MEM16(g_eax+g_edx)=(uint16_t)g_edi;g_edx=MEM32(g_esp+16);++g_edi;}g_esi+=4;}while(g_esi<g_edx);POP32(g_esp,g_ebx);}
 g_eax=MEM32(frame+8)&0x3fffffff;if((int32_t)g_eax<(int32_t)g_edi){g_eax*=2;if((int32_t)g_edi>=(int32_t)g_eax)g_eax=g_edi;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,frame);CALL(0x325128,sub_002E00E0);g_esp+=12;}MEM32(frame+4)=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ecx);g_esp+=16;
}
static void insert_endpoints(void){
 PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+4)+2;g_eax=MEM32(g_edi+8)&0x3fffffff;if((int32_t)g_eax<(int32_t)g_esi){g_eax*=2;if((int32_t)g_esi>=(int32_t)g_eax)g_eax=g_esi;PUSH32(g_esp,4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);CALL(0x325167,sub_002E00E0);g_esp+=12;}
 g_eax=MEM32(g_edi);SET_LO16(g_ecx,MEM16(g_esp+24));g_eax+=g_esi*4-12;MEM32(g_edi+4)=g_esi;while((uint16_t)g_ecx<=MEM16(g_eax)){g_edx=MEM32(g_eax);MEM32(g_eax+8)=g_edx;g_eax-=4;}g_edx=MEM32(g_esp+16);MEM16(g_eax+8)=(uint16_t)g_ecx;MEM16(g_eax+10)=(uint16_t)g_edx;g_esi=MEM32(g_edi);g_ecx=g_eax-g_esi;g_esi=MEM32(g_esp+32);g_ecx=(uint32_t)((int32_t)g_ecx>>2)+2;MEM16(g_esi)=(uint16_t)g_ecx;SET_LO16(g_ecx,MEM16(g_esp+20));while((uint16_t)g_ecx<MEM16(g_eax)){g_esi=MEM32(g_eax);MEM32(g_eax+4)=g_esi;g_eax-=4;}MEM16(g_eax+6)=(uint16_t)g_edx;g_edx=MEM32(g_esp+28);MEM16(g_eax+4)=(uint16_t)g_ecx;g_eax-=MEM32(g_edi);g_eax=(uint32_t)((int32_t)g_eax>>2)+1;POP32(g_esp,g_edi);MEM16(g_edx)=(uint16_t)g_eax;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;g_esp+=28;
}
int uc_migration_physics_broadphase_storage(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_STORAGE");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,160))return 0;
 if(entry==0x324570||entry==0x324600){uint32_t body=MEM32(g_esp+8);if(!safe(body,16)||!safe(g_edx,16))return 0;uint32_t value=MEM32(body+12);if(value&1){uint64_t header=(uint64_t)g_ecx+(value&~1u)+4;if(header>0xffffffff||!array((uint32_t)header,2))return 0;if(entry==0x324600){uint32_t data=MEM32((uint32_t)header),n=MEM32((uint32_t)header+4);if(!n||!safe(data-2,(n+1)*2))return 0;}}else if(!array(MEM32(g_esp+12),8))return 0;pair_record(entry==0x324600);return 1;}
 if(entry==0x325140){if(!array(g_ecx,4))return 0;uint32_t n=MEM32(g_ecx+4),p=MEM32(g_ecx),low=MEM16(g_esp+12),high=MEM16(g_esp+16);if(n<2||n>65533||!high||low>high||MEM16(p)!=0||MEM16(p+(n-1)*4)!=65535||!safe(MEM32(g_esp+20),2)||!safe(MEM32(g_esp+24),2))return 0;for(uint32_t i=1;i<n;i++)if(MEM16(p+(i-1)*4)>MEM16(p+i*4))return 0;insert_endpoints();return 1;}
 if(entry!=0x3250a0||!array(g_ecx,4)||!span(0x505cd4,24))return 0;uint32_t axis=MEM32(g_esp+8),map=MEM32(g_esp+12),base=MEM32(g_esp+4);if(axis>2||!safe(map,4))return 0;uint32_t data=MEM32(g_ecx),count=MEM32(g_ecx+4),mapping=MEM32(map);for(uint32_t i=0;i<count;i++){uint32_t old=MEM16(data+i*4+2);uint64_t slot=(uint64_t)mapping+old*4ull;if(slot>0xffffffff||!safe((uint32_t)slot,4))return 0;int32_t target=(int32_t)MEM32((uint32_t)slot);if(target>=0){uint32_t offset=MEM32(0x505cd4+(axis*2+(MEM8(data+i*4)&1))*4);uint64_t address=(uint64_t)base+(uint32_t)target*16ull+offset;if(address>0xffffffff||!safe((uint32_t)address,2))return 0;}}compact_endpoints();return 1;
}
#undef CALL
