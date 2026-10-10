/* Packed sweep-node index repair and endpoint lookup, in the original order. */
#include "migration_physics_broadphase_index.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-128||(uint64_t)g_esp+32<=p);}
static void repair(int remove){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-48;g_eax=MEM32(frame+16);
 SET_LO16(g_ecx,MEM16(g_eax+8)-(!remove));MEM16(g_esp+24)=(uint16_t)g_ecx;MEM16(g_esp+26)=(uint16_t)g_ecx;
 SET_LO16(g_ecx,MEM16(g_eax)-(!remove));MEM16(g_esp+16)=(uint16_t)g_ecx;MEM16(g_esp+20)=(uint16_t)g_ecx;
 SET_LO16(g_ecx,MEM16(g_eax+2)-(!remove));MEM16(g_esp+18)=(uint16_t)g_ecx;MEM16(g_esp+22)=(uint16_t)g_ecx;
 SET_LO16(g_ecx,MEM16(g_eax+10)-(remove?0:2));MEM16(g_esp+40)=(uint16_t)g_ecx;MEM16(g_esp+42)=(uint16_t)g_ecx;
 SET_LO16(g_ecx,MEM16(g_eax+4));SET_LO16(g_eax,MEM16(g_eax+6));if(!remove){SET_LO16(g_eax,(uint16_t)g_eax-2);SET_LO16(g_ecx,(uint16_t)g_ecx-2);}MEM16(g_esp+34)=(uint16_t)g_eax;MEM16(g_esp+38)=(uint16_t)g_eax;g_eax=MEM32(frame+12);MEM16(g_esp+32)=(uint16_t)g_ecx;MEM16(g_esp+36)=(uint16_t)g_ecx;
 g_ecx=MEM32(frame+8);g_eax<<=4;if(remove)g_edx=0xffffffff;g_eax+=g_ecx;if(remove){MEM32(g_esp+28)=g_edx;MEM32(g_esp+44)=g_edx;}
 if(g_ecx<g_eax){g_mmx[2]=MMX_MEM(g_esp+24);g_mmx[3]=MMX_MEM(g_esp+16);g_mmx[4]=MMX_MEM(g_esp+40);g_mmx[5]=MMX_MEM(g_esp+32);g_edx=0xffff;MEM16(g_esp)=0xffff;MEM16(g_esp+2)=0xffff;g_edx=0;MEM16(g_esp+4)=0;MEM16(g_esp+6)=0;
  do{g_mmx[0]=MMX_MEM(g_ecx+8);g_mmx[1]=MMX_PCMPGTW(g_mmx[0],g_mmx[4]);g_mmx[0]=MMX_PCMPGTW(g_mmx[0],g_mmx[2]);g_mmx[0]=MMX_PADDW(g_mmx[0],g_mmx[1]);g_mmx[1]=MMX_PAND(g_mmx[0],MMX_MEM(g_esp));g_mmx[0]=MMX_MEM(g_ecx);g_mmx[6]=MMX_PCMPGTW(g_mmx[0],g_mmx[5]);g_mmx[7]=MMX_PCMPGTW(g_mmx[0],g_mmx[3]);g_mmx[7]=MMX_PADDW(g_mmx[7],g_mmx[6]);g_mmx[0]=remove?MMX_PADDW(g_mmx[0],g_mmx[7]):MMX_PSUBW(g_mmx[0],g_mmx[7]);MMX_STORE(g_ecx,g_mmx[0]);g_mmx[0]=MMX_MEM(g_ecx+8);g_mmx[0]=remove?MMX_PADDW(g_mmx[0],g_mmx[1]):MMX_PSUBW(g_mmx[0],g_mmx[1]);MMX_STORE(g_ecx+8,g_mmx[0]);g_ecx+=16;}while(g_ecx<g_eax);
 }
 int equal=g_ecx==g_eax;g_esp=frame;POP32(g_esp,frame);g_recomp_zf=equal;g_esp+=16;
}
static void lower_bound(void){
 g_edx=MEM32(g_esp+8);g_eax=MEM32(g_esp+4);g_ecx=g_edx;PUSH32(g_esp,g_esi);g_ecx-=g_eax;g_esi=g_ecx&~3u;int wide=(int32_t)g_esi>64;SET_LO16(g_esi,MEM16(g_esp+16));
 if(wide){PUSH32(g_esp,g_edi);do{g_ecx=(uint32_t)((int32_t)g_ecx>>3);int before=MEM16(g_eax+g_ecx*4)<(uint16_t)g_esi;g_ecx=g_eax+g_ecx*4;if(before)g_eax=g_ecx;else g_edx=g_ecx;g_ecx=g_edx-g_eax;g_edi=g_ecx&~3u;}while((int32_t)g_edi>64);POP32(g_esp,g_edi);}
 while(MEM16(g_eax)<(uint16_t)g_esi)g_eax+=4;int equal=MEM16(g_eax)==(uint16_t)g_esi;POP32(g_esp,g_esi);g_recomp_zf=equal;g_esp+=16;
}
static void remove_endpoints(void){
 g_eax=MEM32(g_esp+4);g_edx=g_ecx;g_ecx=MEM32(g_edx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);g_eax=g_ecx+g_eax*4;g_ecx+=g_esi*4-4;PUSH32(g_esp,g_edi);
 if(g_eax<g_ecx){g_ecx=((g_ecx-g_eax-1)>>2)+1;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_esi=g_eax+4;g_edi=g_eax;g_eax+=g_ebx*4;memmove((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;POP32(g_esp,g_ebx);}
 g_ecx=MEM32(g_edx+4);g_esi=g_ecx-2;g_ecx=MEM32(g_edx)+g_esi*4;MEM32(g_edx+4)=g_esi;
 if(g_eax<g_ecx){g_ecx=((g_ecx-g_eax-1)>>2)+1;g_esi=g_eax+8;g_edi=g_eax;memmove((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=0;}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=12;
}
int uc_migration_physics_broadphase_index(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_INDEX");enabled=v&&!strcmp(v,"1");}if(!enabled||!span(g_esp-128,160))return 0;
 uint32_t a=MEM32(g_esp+4),b=MEM32(g_esp+8),c=MEM32(g_esp+12);
 if(entry==0x323ca0||entry==0x323da0){if(b>65535||!safe(c,12)||(b&&!safe(a,b*16)))return 0;repair(entry==0x323da0);return 1;}
 if(entry==0x323ea0){if(b<a||(b-a)%4||(b-a)>65535*4||!safe(a,b-a+4)||MEM16(b)<(uint16_t)c)return 0;lower_bound();return 1;}
 if(entry==0x3240a0){if(!safe(g_ecx,8))return 0;uint32_t n=MEM32(g_ecx+4);if(n<2||n>65535||a>=b||b>=n||!safe(MEM32(g_ecx),n*4))return 0;remove_endpoints();return 1;}
 if(entry==0x3242b0){if(!safe(g_ecx,0x74))return 0;g_eax=(MEM32(g_ecx+0x44)-MEM32(g_ecx+0x70))*24+36;g_recomp_zf=g_eax==0;g_esp+=4;return 1;}
 return 0;
}
