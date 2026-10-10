/* Original batch broadphase removal, remapping and temporary storage lifetime. */
#include "migration_physics_broadphase_remove.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
void sub_00327BF0(void);void sub_003250A0(void);void sub_002E00E0(void);
#define CALL(ret,fn) do{g_ebp=frame;PUSH32(g_esp,ret);fn();}while(0)
static int span(uint32_t p,uint32_t n){return p>=0x10000&&p<0x80000000&&(uint64_t)p+n<=g_xbox_total_ram;}
static int safe(uint32_t p,uint32_t n){return span(p,n)&&((uint64_t)p+n<=g_esp-2048||(uint64_t)g_esp+12<=p);}
static int array(uint32_t p,unsigned stride){if(!safe(p,12))return 0;uint32_t n=MEM32(p+4),cap=MEM32(p+8)&0x3fffffff;return n<=cap&&cap<=65535&&(!cap||safe(MEM32(p),cap*stride));}
static void remove_batch(void){
 uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-36;g_eax=MEM32(frame+8);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;g_ecx=MEM32(g_eax+4);g_eax=MEM32(g_ebx+68);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_eax;MEM32(g_esp+32)=g_eax;g_eax=(uint32_t)((int32_t)g_eax>>5);MEM32(g_esp+20)=g_ecx;g_ecx=MEM32(0x51e6f4);g_esi=MEM32(g_ecx+32);g_eax=(g_eax*4+48)&~15u;g_edx=g_esi+g_eax;g_edi=(uint32_t)((int32_t)g_edi>>3);MEM32(g_esp+36)=g_ebx;
 if(g_edx<=MEM32(g_ecx+44))MEM32(g_ecx+32)=g_edx;else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x329131);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;}
 g_edi=(uint32_t)((int32_t)g_edi>>4);g_xmm0=XMM_ZERO();g_eax=g_esi;if((int32_t)g_edi>=0){g_edx=g_edi+1;do{g_ecx=g_eax;g_eax+=16;--g_edx;XMM_STORE(g_ecx,g_xmm0);}while(g_edx);}
 g_eax=MEM32(g_esp+20);g_edx=0;if((int32_t)g_eax>0){do{g_eax=MEM32(frame+8);g_ecx=MEM32(g_eax);g_eax=MEM32(g_ecx+g_edx*4);g_ecx=MEM32(g_eax);g_eax=g_ecx;g_ecx&=31;g_eax=(uint32_t)((int32_t)g_eax>>5);g_edi=1u<<(uint8_t)g_ecx;MEM32(g_esi+g_eax*4)^=g_edi;g_eax=MEM32(g_esp+20);++g_edx;}while((int32_t)g_edx<(int32_t)g_eax);}
 PUSH32(g_esp,g_ecx);g_ecx=MEM32(frame+12);g_eax=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_ecx=g_ebx;MEM8(g_eax)=0;CALL(0x32918e,sub_00327BF0);
 g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx+40);MEM32(g_ecx+32)=g_esi;if(!g_eax||g_esi==g_eax){g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_edx+52);PUSH32(g_esp,0x3291a8);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+20);g_eax=0;MEM32(g_esp+24)=0;if((int32_t)g_ecx>0){do{g_ecx=MEM32(frame+8);g_edx=MEM32(g_ecx);g_edx=MEM32(g_edx+g_eax*4);g_edx=MEM32(g_edx);g_esi=MEM32(g_ebx+64);g_edx<<=4;g_edi=g_esp+24;MEM32(g_edx+g_esi+12)=g_edi;g_ecx=MEM32(g_ecx);g_edx=MEM32(g_ecx+g_eax*4);g_ecx=MEM32(g_esp+20);++g_eax;MEM32(g_edx)=0;}while((int32_t)g_eax<(int32_t)g_ecx);}
 g_esi=MEM32(g_esp+32);g_ecx=MEM32(0x51e6f4);g_edi=MEM32(g_ecx+32);g_eax=(g_esi*4+16)&~15u;g_edx=g_edi+g_eax;if(g_edx<=MEM32(g_ecx+44))MEM32(g_ecx+32)=g_edx;else{g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+48);PUSH32(g_esp,0x329216);RECOMP_ICALL_SAFE(target,saved);g_edi=g_eax;}
 g_edx=g_esi-1;g_ecx=0;MEM32(g_esp+28)=g_edi;MEM32(g_esp+40)=g_edi;
 if((int32_t)g_edx>=0){g_esi=g_edx<<4;MEM32(g_esp+16)=g_esi;MEM32(g_esp+32)=g_ecx;for(;;){g_eax=MEM32(g_ebx+64)+g_esi;g_esi=g_esp+24;
   if(MEM32(g_eax+12)==g_esi){g_eax=MEM32(g_esp+16);MEM32(g_edi+g_edx*4)=0xffffffff;--g_edx;g_eax-=16;MEM32(g_esp+16)=g_eax;}
   else{g_edi=MEM32(g_esp+32);g_esi=MEM32(g_ebx+64)+g_edi;g_edi=g_esp+24;
    if(MEM32(g_esi+12)==g_edi){g_ebx=MEM32(g_eax);g_edi=g_esi;MEM32(g_edi)=g_ebx;g_ebx=MEM32(g_eax+4);MEM32(g_edi+4)=g_ebx;g_ebx=MEM32(g_eax+8);MEM32(g_edi+8)=g_ebx;g_eax=MEM32(g_eax+12);g_ebx=MEM32(g_esp+36);MEM32(g_edi+12)=g_eax;g_edi=MEM32(g_esp+28);MEM32(g_edi+g_edx*4)=g_ecx;MEM32(g_edi+g_ecx*4)=0xffffffff;g_eax=MEM32(g_esi+12);if(!(g_eax&1))MEM32(g_eax)=g_ecx;else{g_esi=MEM32(g_ebx+120);g_eax&=~1u;MEM16(g_eax+g_esi)=(uint16_t)g_ecx;}g_eax=MEM32(g_esp+16);--g_edx;g_eax-=16;MEM32(g_esp+16)=g_eax;}
    else{g_edi=MEM32(g_esp+28);MEM32(g_edi+g_ecx*4)=g_ecx;}
    g_eax=MEM32(g_esp+32);++g_ecx;g_eax+=16;MEM32(g_esp+32)=g_eax;
   }
   if((int32_t)g_ecx>(int32_t)g_edx)break;g_esi=MEM32(g_esp+16);
  }}
 g_eax=MEM32(g_ebx+72)&0x3fffffff;g_esi=g_ebx+64;++g_edx;MEM32(g_esp+32)=g_edx;if((int32_t)g_eax<(int32_t)g_edx){g_eax*=2;if((int32_t)g_edx>=(int32_t)g_eax)g_eax=g_edx;PUSH32(g_esp,16);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);CALL(0x329311,sub_002E00E0);g_edx=MEM32(g_esp+44);g_esp+=12;}
 g_ecx=g_esp+40;PUSH32(g_esp,g_ecx);MEM32(g_esi+4)=g_edx;g_edx=MEM32(g_esi);PUSH32(g_esp,0);PUSH32(g_esp,g_edx);g_ecx=g_ebx+76;CALL(0x32932d,sub_003250A0);
 g_ecx=MEM32(g_esi);g_eax=g_esp+40;PUSH32(g_esp,g_eax);PUSH32(g_esp,1);PUSH32(g_esp,g_ecx);g_ecx=g_ebx+88;CALL(0x32933f,sub_003250A0);
 g_eax=MEM32(g_esi);g_edx=g_esp+40;PUSH32(g_esp,g_edx);PUSH32(g_esp,2);PUSH32(g_esp,g_eax);g_ecx=g_ebx+100;CALL(0x329351,sub_003250A0);
 g_eax=MEM32(g_ebx+112);g_ecx=0;if(g_eax){g_edx=MEM32(g_esp+20)*2;MEM32(g_esp+20)=g_edx;MEM32(g_esp+28)=g_ecx;if((int32_t)g_eax>0){MEM32(g_esp+32)=g_ecx;do{g_ecx=MEM32(g_ebx+120)+MEM32(g_esp+32);g_eax=MEM16(g_ecx)*16u+MEM32(g_esi);MEM16(g_eax+4)-=(uint16_t)g_edx;MEM16(g_eax+6)-=(uint16_t)g_edx;g_eax=0;if((int32_t)MEM32(g_ecx+8)>0){do{g_edx=MEM32(g_ecx+4);g_ebx=MEM16(g_edx+g_eax*2);SET_LO16(g_ebx,MEM16(g_edi+g_ebx*4));g_edx+=g_eax*2;MEM16(g_edx)=(uint16_t)g_ebx;g_edx=MEM32(g_ecx+8);++g_eax;}while((int32_t)g_eax<(int32_t)g_edx);g_ebx=MEM32(g_esp+36);g_edx=MEM32(g_esp+20);}g_ecx=MEM32(g_esp+32);g_eax=MEM32(g_esp+28);g_ecx+=16;++g_eax;MEM32(g_esp+32)=g_ecx;MEM32(g_esp+28)=g_eax;}while((int32_t)g_eax<(int32_t)MEM32(g_ebx+112));}}
 g_ecx=MEM32(0x51e6f4);g_eax=MEM32(g_ecx+40);MEM32(g_ecx+32)=g_edi;if(!g_eax||g_edi==g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_eax+52);PUSH32(g_esp,0x3293e4);RECOMP_ICALL_SAFE(target,saved);}
 POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_esp+=12;
}
int uc_migration_physics_broadphase_remove(unsigned entry){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_BROADPHASE_REMOVE");enabled=v&&!strcmp(v,"1");}if(!enabled||entry!=0x3290e0||!span(g_esp-2048,2060)||!safe(g_ecx,124)||!span(0x51e6f4,4))return 0;
 uint32_t object=g_ecx,handles=MEM32(g_esp+4),pairs=MEM32(g_esp+8),allocator=MEM32(0x51e6f4);if(!array(handles,4)||!array(pairs,8)||!array(object+64,16)||!safe(allocator,48)||!span(MEM32(allocator),56))return 0;
 uint32_t count=MEM32(object+68),records=MEM32(object+64),n=MEM32(handles+4);if(n>count)return 0;uint32_t scratch=MEM32(allocator+32),bytes=count*4+64;if(!safe(scratch,bytes))return 0;
 for(uint32_t i=0;i<n;i++){uint32_t p=MEM32(MEM32(handles)+i*4);if(!safe(p,4)||MEM32(p)>=count)return 0;}
 for(unsigned axis=0;axis<3;axis++){uint32_t h=object+76+axis*12;if(!array(h,4))return 0;for(uint32_t i=0;i<MEM32(h+4);i++)if(MEM16(MEM32(h)+i*4+2)>=count)return 0;}
 for(uint32_t i=0;i<count;i++){uint32_t value=MEM32(records+i*16+12);if(value&1){uint64_t p=(uint64_t)MEM32(object+120)+(value&~1u);if(p>0xffffffff||!safe((uint32_t)p,16))return 0;}else if(!safe(value,4))return 0;}
 uint32_t partitions=MEM32(object+112),p=MEM32(object+120);if(partitions>65535||!safe(p,partitions*16))return 0;for(uint32_t i=0;i<partitions;i++){uint32_t h=p+i*16;if(MEM16(h)>=count||!array(h+4,2))return 0;for(uint32_t j=0;j<MEM32(h+8);j++)if(MEM16(MEM32(h+4)+j*2)>=count)return 0;}
 remove_batch();return 1;
}
#undef CALL
