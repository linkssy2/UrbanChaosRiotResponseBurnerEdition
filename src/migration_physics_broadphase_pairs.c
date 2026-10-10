/* Original sweep pair enumeration and partition membership mutation. */
#include "migration_physics_broadphase_pairs.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_002E0150(void);
#define GROW(ret) do{PUSH32(g_esp,ret);sub_002E0150();g_esp+=8;}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-2048||(uint64_t)g_esp+16<=p);}
static int array(uint32_t p,unsigned stride){if(!safe(p,12))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff;return n<=cap&&cap<=65535&&(!cap||safe(MEM32(p),cap*stride));}
static void compare_active(uint32_t frame,int second){
 unsigned header=second?40:564;
 do{g_edx=MEM32(g_esp+20);g_eax=MEM32(g_edx+64);g_edx=MEM32(g_esp+header);g_esi=MEM32(g_esp+16);g_esi=MEM16(g_edx+g_esi*2);g_edi=MEM32(frame);g_esi<<=4;g_edx=MEM32(g_esi+g_eax+4);g_esi+=g_eax;g_edx-=g_edi;g_edi=MEM32(frame+4)-MEM32(g_esi);g_ecx=frame;g_edx|=g_edi;
  if(!(g_edx&0x80008000u)){
   if(!(MEM8(frame+12)&1)&&!(MEM8(g_esi+12)&1)){g_eax=MEM32(g_ebx+8)&0x3fffffff;g_ecx=MEM32(g_ebx+4);if(g_ecx==g_eax){PUSH32(g_esp,8);PUSH32(g_esp,g_ebx);GROW(second?0x327e99:0x327d5c);}g_eax=MEM32(g_ebx+4);g_ecx=MEM32(g_ebx)+g_eax*8;++g_eax;MEM32(g_ebx+4)=g_eax;g_edx=MEM32(g_esi+12);MEM32(g_ecx)=g_edx;g_eax=MEM32(frame+12);MEM32(g_ecx+4)=g_eax;}
   else{
    if(!(MEM8(frame+12)&1)){g_ecx=g_esi;g_esi=frame;}g_esi-=g_eax;g_eax=MEM32(g_ecx+12);g_ecx=MEM32(g_esp+20);g_edx=MEM32(g_ecx+120);SET_LO8(g_ecx,MEM8(g_esp+1100));g_eax&=~1u;g_esi=(uint32_t)((int32_t)g_esi>>4);g_eax+=g_edx;
    if((uint8_t)g_ecx){g_edx=MEM32(g_eax+12)&0x3fffffff;g_edi=g_eax+4;g_eax=MEM32(g_edi+4);if(g_eax==g_edx){PUSH32(g_esp,2);PUSH32(g_esp,g_edi);GROW(second?0x327efd:0x327db9);}g_eax=MEM32(g_edi+4);g_ecx=MEM32(g_edi);MEM16(g_ecx+g_eax*2)=(uint16_t)g_esi;++MEM32(g_edi+4);}
    else{g_edi=MEM32(g_eax+8);g_ecx=0;int found=0;if((int32_t)g_edi>0){g_edx=MEM32(g_eax+4);do{if(MEM16(g_edx)==(uint16_t)g_esi){found=1;break;}++g_ecx;g_edx+=2;}while((int32_t)g_ecx<(int32_t)g_edi);}
     if(second){if(!found)g_ecx=0xffffffff;g_edx=MEM32(g_eax+8)-1;MEM32(g_eax+8)=g_edx;g_eax=MEM32(g_eax+4);SET_LO16(g_edx,MEM16(g_eax+g_edx*2));MEM16(g_eax+g_ecx*2)=(uint16_t)g_edx;}
     else{g_edx=found?g_ecx:0xffffffff;g_ecx=MEM32(g_eax+8)-1;MEM32(g_eax+8)=g_ecx;g_eax=MEM32(g_eax+4);SET_LO16(g_ecx,MEM16(g_eax+g_ecx*2));MEM16(g_eax+g_edx*2)=(uint16_t)g_ecx;}
    }
   }
  }
  g_eax=MEM32(g_esp+16);g_ecx=MEM32(g_esp+header+4);++g_eax;MEM32(g_esp+16)=g_eax;
 }while((int32_t)g_eax<(int32_t)g_ecx);
}
static void enumerate_pairs(void){
 uint32_t frame=g_seh_ebp;g_esp-=1072;g_edx=g_ecx;g_eax=g_esp+560;MEM32(g_esp+548)=g_eax;g_eax=0;g_ecx=0x80000100;PUSH32(g_esp,g_esi);MEM32(g_esp+560)=g_ecx;g_esi=g_esp+40;MEM32(g_esp+36)=g_ecx;g_ecx=MEM32(g_edx+80);MEM32(g_esp+556)=g_eax;MEM32(g_esp+32)=g_eax;g_eax=MEM32(g_edx+76);MEM32(g_esp+28)=g_esi;g_ecx=g_eax+g_ecx*4-4;g_esi=g_eax+4;MEM32(g_esp+8)=g_edx;MEM32(g_esp+20)=g_ecx;MEM32(g_esp+12)=g_esi;
 if(g_esi<g_ecx){PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+1088);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
  for(;;){g_edi=MEM16(g_esi+2);frame=MEM32(g_esp+1092);g_ecx=g_edi&31;g_eax=1u<<(uint8_t)g_ecx;g_ecx=(uint32_t)((int32_t)g_edi>>5);MEM32(g_esp+28)=g_edi;g_eax&=MEM32(frame+g_ecx*4);MEM32(g_esp+36)=g_eax;
   if(MEM8(g_esi)&1){g_eax=g_esp+(g_eax?564:40);g_esi=MEM32(g_eax+4);g_ecx=0;int found=0;if((int32_t)g_esi>0){g_edx=MEM32(g_eax);do{if(MEM16(g_edx)==(uint16_t)g_edi){found=1;break;}frame=MEM32(g_eax+4);++g_ecx;g_edx+=2;}while((int32_t)g_ecx<(int32_t)frame);}g_edx=found?g_ecx:0xffffffff;g_ecx=g_esi-1;MEM32(g_eax+4)=g_ecx;g_eax=MEM32(g_eax);SET_LO16(g_ecx,MEM16(g_eax+g_ecx*2));MEM16(g_eax+g_edx*2)=(uint16_t)g_ecx;}
   else{g_esi=MEM32(g_edx+64);g_ecx=MEM32(g_esp+568);frame=(g_edi<<4)+g_esi;MEM32(g_esp+16)=0;
    if((int32_t)g_ecx>0){compare_active(frame,0);g_eax=MEM32(g_esp+36);g_edi=MEM32(g_esp+28);}
    if(g_eax){g_eax=MEM32(g_esp+44);MEM32(g_esp+16)=0;if((int32_t)g_eax>0){compare_active(frame,1);g_edi=MEM32(g_esp+28);}g_eax=MEM32(g_esp+572)&0x3fffffff;g_ecx=MEM32(g_esp+568);if(g_ecx==g_eax){g_eax=g_esp+564;PUSH32(g_esp,2);PUSH32(g_esp,g_eax);GROW(0x327f81);}g_edx=MEM32(g_esp+568);g_ecx=MEM32(g_esp+564);MEM16(g_ecx+g_edx*2)=(uint16_t)g_edi;++MEM32(g_esp+568);}
    else{g_eax=MEM32(g_esp+48)&0x3fffffff;g_ecx=MEM32(g_esp+44);if(g_ecx==g_eax){g_eax=g_esp+40;PUSH32(g_esp,2);PUSH32(g_esp,g_eax);GROW(0x327fbc);}g_edx=MEM32(g_esp+44);g_ecx=MEM32(g_esp+40);MEM16(g_ecx+g_edx*2)=(uint16_t)g_edi;++MEM32(g_esp+44);}
   }
   g_esi=MEM32(g_esp+24);g_eax=MEM32(g_esp+32);g_esi+=4;MEM32(g_esp+24)=g_esi;if(g_esi>=g_eax)break;g_edx=MEM32(g_esp+20);
  }POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
 }
 g_eax=MEM32(g_esp+36);POP32(g_esp,g_esi);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,20);g_eax<<=1;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+32);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x32800c);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esp+556);if((int32_t)g_eax>=0){g_ecx=MEM32(0x51e6f4);g_edx=MEM32(g_ecx);g_eax&=0x3fffffff;PUSH32(g_esp,20);g_eax<<=1;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+556);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x328034);RECOMP_ICALL_SAFE(target,saved);}
 g_esp+=1072;g_recomp_zf=g_esp==0;g_esp+=16;
}
int uc_migration_physics_broadphase_pairs(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_PAIRS");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x327bf0||!span(g_esp-2048,2064)||!safe(g_ecx,124)||!array(g_ecx+64,16)||!array(g_ecx+76,4)||!array(MEM32(g_esp+8),8)||!span(0x51e6f4,4))return 0;
 uint32_t records=MEM32(g_ecx+64),count=MEM32(g_ecx+68),endpoints=MEM32(g_ecx+76),n=MEM32(g_ecx+80),mask=MEM32(g_esp+4),allocator=MEM32(0x51e6f4);if(n<2||!safe(mask,((count+31)/32)*4)||!safe(allocator,4)||!span(MEM32(allocator),24))return 0;
 /* Validate matched intervals before the original tail-swap active-list removals. */
 unsigned char active[65536]={0};for(uint32_t i=1;i+1<n;i++){uint32_t id=MEM16(endpoints+i*4+2);if(id>=count)return 0;unsigned closing=MEM8(endpoints+i*4)&1;if(active[id]!=closing)return 0;active[id]=(unsigned char)!closing;}
 for(uint32_t i=0;i<count;i++){uint32_t tag=MEM32(records+i*16+12);if(tag&1){uint64_t p=(uint64_t)MEM32(g_ecx+120)+(tag&~1u)+4;if(p>0xffffffff||!array((uint32_t)p,2))return 0;uint32_t data=MEM32((uint32_t)p),length=MEM32((uint32_t)p+4);if(!MEM8(g_esp+12)&&(!length||!safe(data-2,(length+1)*2)))return 0;}}
 enumerate_pairs();return 1;
}
#undef GROW
