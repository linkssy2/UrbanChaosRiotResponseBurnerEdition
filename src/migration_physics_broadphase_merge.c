/* Merge original broadphase endpoints, preserving ties and reciprocal indices. */
#include "migration_physics_broadphase_merge.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-64||(uint64_t)g_esp+24<=p);}
static int separate(uint32_t a,uint32_t an,uint32_t b,uint32_t bn){return (uint64_t)a+an<=b||(uint64_t)b+bn<=a;}
static void merge_endpoints(void){
 uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+28);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_esp+24);g_eax=0;
 while((int32_t)g_eax<(int32_t)g_ecx){g_edx=MEM32(g_edi);g_edx=MEM32(g_edx+g_eax*4);MEM32(frame+g_eax*4)=g_edx;++g_eax;}
 g_ebx=MEM32(g_edi);g_edx=frame+4;frame+=g_ecx*4-4;g_esi=g_ebx+g_ecx*4;MEM32(g_esp+24)=frame;frame=MEM32(g_esp+28);g_ecx+=frame;g_eax=g_ebx+4;g_ebx+=g_ecx*4;SET_LO16(g_ecx,MEM16(g_esi));MEM32(g_esp+36)=g_ebx;
 while(MEM16(g_edx)<(uint16_t)g_ecx){g_edx+=4;g_eax+=4;}frame=MEM32(g_esp+20);
 if(g_edx<MEM32(g_esp+24)&&g_esi<g_ebx){
  for(;;){SET_LO16(g_ecx,MEM16(g_edx));g_ebx=0;
   if((uint16_t)g_ecx<MEM16(g_esi)){
    g_ecx=MEM32(g_edx);MEM32(g_eax)=g_ecx;SET_LO8(g_ebx,MEM8(g_eax));frame=MEM32(g_edi);g_ecx=g_eax-frame;frame=MEM32(g_esp+32);g_ecx=(uint32_t)((int32_t)g_ecx>>2);g_edx+=4;g_ebx=(g_ebx&1)+frame*2;frame=MEM16(g_eax+2)*16u+MEM32(g_ebx*4+0x505cd4);g_eax+=4;g_ebx=frame;frame=MEM32(g_esp+20);MEM16(g_ebx+frame)=(uint16_t)g_ecx;
    if(g_edx>=MEM32(g_esp+24)){g_ebx=MEM32(g_esp+36);break;}
   }else{
    g_ecx=MEM32(g_esi);MEM32(g_eax)=g_ecx;SET_LO8(g_ebx,MEM8(g_eax));frame=MEM32(g_edi);g_ecx=g_eax-frame;frame=MEM32(g_esp+32);g_ecx=(uint32_t)((int32_t)g_ecx>>2);g_ebx=(g_ebx&1)+frame*2;frame=MEM16(g_eax+2)*16u+MEM32(g_ebx*4+0x505cd4);g_ebx=frame;frame=MEM32(g_esp+20);MEM16(g_ebx+frame)=(uint16_t)g_ecx;g_esi+=4;g_eax+=4;
    if(g_esi>=MEM32(g_esp+36))break;
   }
  }
 }
 if(g_edx>=MEM32(g_esp+24)){
  while(g_esi<g_ebx){g_ecx=MEM32(g_esi);MEM32(g_eax)=g_ecx;frame=MEM32(g_edi);g_ebx=0;SET_LO8(g_ebx,MEM8(g_eax));g_ecx=g_eax-frame;frame=MEM32(g_esp+32)*2;g_ecx=(uint32_t)((int32_t)g_ecx>>2);g_esi+=4;g_ebx=(g_ebx&1)+frame;frame=MEM16(g_eax+2)*16u+MEM32(g_ebx*4+0x505cd4);g_eax+=4;g_ebx=frame;frame=MEM32(g_esp+20);MEM16(g_ebx+frame)=(uint16_t)g_ecx;if(g_esi>=MEM32(g_esp+36))break;g_ebx=MEM32(g_esp+36);}
 }
 if(g_edx<=MEM32(g_esp+24)){g_ecx=MEM32(g_esp+32)*2;MEM32(g_esp+20)=g_ecx;do{g_ecx=MEM32(g_edx);MEM32(g_eax)=g_ecx;g_esi=MEM32(g_edi);g_ebx=0;SET_LO8(g_ebx,MEM8(g_eax));g_ecx=g_eax-g_esi;g_esi=MEM32(g_esp+20);g_ecx=(uint32_t)((int32_t)g_ecx>>2);g_edx+=4;g_eax+=4;g_ebx=(g_ebx&1)+g_esi;g_esi=MEM16(g_eax-2)*16u+MEM32(g_ebx*4+0x505cd4);MEM16(g_esi+frame)=(uint16_t)g_ecx;}while(g_edx<=MEM32(g_esp+24));}
 g_recomp_zf=g_edx==MEM32(g_esp+24);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=24;
}
int uc_migration_physics_broadphase_merge(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_MERGE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x323f00||!span(g_esp-64,88)||!safe(g_ecx,12)||!span(0x505cd4,24))return 0;
 uint32_t base=MEM32(g_esp+4),old=MEM32(g_esp+8),added=MEM32(g_esp+12),axis=MEM32(g_esp+16),temp=MEM32(g_esp+20),data=MEM32(g_ecx),cap=MEM32(g_ecx+8)&0x3fffffff;
 if(old<2||!added||old>65535||added>65535-old||axis>2||cap<old+added||!safe(data,(old+added)*4)||!safe(temp,old*4)||!separate(temp,old*4,data,(old+added)*4)||!separate(temp,old*4,g_ecx,12)||MEM16(data)!=0||MEM16(data+(old-1)*4)!=65535)return 0;
 for(uint32_t i=0;i<old+added;i++){if(i&&i!=old&&MEM16(data+(i-1)*4)>MEM16(data+i*4))return 0;uint32_t id=MEM16(data+i*4+2),offset=MEM32(0x505cd4+(axis*2+(MEM8(data+i*4)&1))*4);uint64_t address=(uint64_t)base+id*16ull+offset;if(address>0xffffffff||!safe((uint32_t)address,2)||!separate((uint32_t)address,2,data,(old+added)*4)||!separate((uint32_t)address,2,temp,old*4)||!separate((uint32_t)address,2,g_ecx,12))return 0;}
 merge_endpoints();return 1;
}
