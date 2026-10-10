#include "migration_audio_resolve_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024C160(void);
extern void sub_00244010(void);
extern void sub_00241880(void);
extern void sub_00240080(void);
static int resolve_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_RESOLVE");f=v&&!strcmp(v,"1");}return f;}
static int resolve_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int resolve_heap(uint32_t p,uint32_t n){return resolve_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+24<=p);}
static int resolve_admit(uint32_t a){
    if(a!=0x24c650||!resolve_enabled()||!resolve_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!resolve_span(g_esp-4096,4120)||!resolve_heap(g_ecx,188))return 0;
    uint32_t index=MEM32(g_esp+4);if(index==0xffffffffu)return 1;if(index>65535)return 0;
    if(!resolve_heap(MEM32(g_esp+8),4)||!resolve_heap(MEM32(g_esp+12),4))return 0;
    uint32_t group=MEM32(g_esp+16);if(group&&!resolve_heap(group,4))return 0;
    uint64_t event=(uint64_t)MEM32(g_ecx+116)+12+index*16;
    if(event>0x7fffffffu||!resolve_heap((uint32_t)event,16))return 0;
    uint32_t sound=MEM16((uint32_t)event+2);if(sound!=0xffff){uint64_t p=(uint64_t)MEM32(g_ecx+120)+12+sound*28;if(p>0x7fffffffu||!resolve_heap((uint32_t)p,28))return 0;}
    return 1;
}
static void resolve_finish(uint32_t frame,int accepted){POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);SET_LO8(g_eax,accepted);POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;}
static void resolve_request(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(g_esp+4);g_esp-=12;PUSH32(g_esp,g_esi);g_esi=g_ecx;
    if(g_eax==0xffffffffu){SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;return;}
    g_ecx=MEM32(g_esi+116);g_eax<<=4;PUSH32(g_esp,g_ebx);g_ebx=g_ecx+g_eax+12;
    PUSH32(g_esp,frame);frame=MEM32(g_esp+32);g_edx=MEM32(frame);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,0x24c692);sub_0024C160();SET_LO16(g_eax,MEM16(g_ebx+2));
    if(LO16(g_eax)==0xffff){POP32(g_esp,frame);POP32(g_esp,g_ebx);SET_LO8(g_eax,0);POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=24;return;}
    PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+40);g_ecx=MEM32(g_esi+120);g_eax=LO16(g_eax)*28;g_edx=g_eax+g_ecx+12;MEM32(g_edi)=g_edx;
    g_eax=MEM32(g_esi+160);
    if(g_eax){g_eax=MEM32(g_edi);SET_LO16(g_eax,MEM16(g_eax+18));if(LO16(g_eax)!=0xffff){
        g_ecx=MEM32(g_esi+160);g_edx=LO16(g_eax);g_eax=MEM32(g_esi+140);g_edx=g_eax+g_edx+8;g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24c6ee);RECOMP_ICALL_SAFE(target,saved);
        PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24c6f4);sub_00244010();g_esp+=8;if(g_eax){resolve_finish(frame,0);return;}
    }}
    g_ecx=MEM32(g_edi);
    if(MEM16(g_ecx+14)!=0xffff){PUSH32(g_esp,0);PUSH32(g_esp,0x24c718);sub_00241880();g_edx=MEM32(g_esp+52);g_ecx=MEM32(g_edi);g_esp+=4;PUSH32(g_esp,g_edx);PUSH32(g_esp,frame);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ecx=g_eax;PUSH32(g_esp,0x24c72d);sub_00240080();if(!LO8(g_eax)){resolve_finish(frame,0);return;}}
    g_ebx=MEM32(g_esp+44);
    if(g_ebx&&!MEM32(g_ebx)){
        g_eax=MEM32(g_edi);
        if(MEM8(g_eax+21)&2){g_eax=MEM16(g_eax+16);g_edx=g_eax+g_eax*4;g_eax=MEM32(g_esi+184);g_ecx=MEM32(g_eax+g_edx*4+28);MEM32(g_ebx)=g_ecx;resolve_finish(frame,1);return;}
        PUSH32(g_esp,0);PUSH32(g_esp,0x24c774);sub_00241880();g_edi=MEM32(g_edi);g_esp+=4;g_ecx=g_eax;
        if(MEM16(g_edi+16)==0xffff){g_edx=MEM32(g_ecx);g_eax=0;MEM16(g_esp+20)=0;MEM16(g_esp+22)=0;MEM16(g_esp+24)=0;g_eax=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4fedd4);MEM32(g_esp+24)=0x4806b4;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x24c7ab);RECOMP_ICALL_SAFE(target,saved);}
        else{g_edx=MEM16(g_edi+16);g_eax=MEM32(g_esi+140);g_edx=g_eax+g_edx+8;g_eax=0;MEM32(g_esp+16)=g_edx;g_edx=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_edx);MEM16(g_esp+24)=0;MEM16(g_esp+26)=0;MEM16(g_esp+28)=0;g_eax=MEM32(g_ecx);PUSH32(g_esp,0x4fedd4);uint32_t target=MEM32(g_eax+12);PUSH32(g_esp,0x24c7eb);RECOMP_ICALL_SAFE(target,saved);}
        MEM32(g_ebx)=g_eax;
    }
    resolve_finish(frame,1);
}
int uc_migration_audio_resolve(uint32_t a){if(!resolve_admit(a))return 0;resolve_request();return 1;}
