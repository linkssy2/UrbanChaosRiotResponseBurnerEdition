#include "migration_audio_spatial_maintenance_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_002519C0(void);
extern void sub_00243F60(void);
extern void sub_002D5CCD(void);
extern void sub_00240D20(void);
extern void sub_002537A0(void);
static int spatial_maintenance_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_SPATIAL_MAINTENANCE");f=v&&!strcmp(v,"1");}return f;}
static int spatial_maintenance_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int spatial_maintenance_safe(uint32_t p,uint64_t n){return spatial_maintenance_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+8<=p);}
static int spatial_maintenance_virtual(uint32_t p,unsigned n){return spatial_maintenance_safe(p,96)&&spatial_maintenance_span(MEM32(p),n);}
static int spatial_maintenance_ring(uint32_t head,unsigned offset,int kind){
    uint32_t p=MEM32(head);unsigned budget=65536;while(p!=head){if(!budget--||p<offset||!spatial_maintenance_safe(p-offset,96)||!spatial_maintenance_safe(MEM32(p+4),8))return 0;uint32_t object=p-offset;
        if(kind==1){uint32_t voice=MEM32(object+60);if(voice&&!spatial_maintenance_safe(voice,44))return 0;}
        if(kind==2){uint32_t child=MEM32(p+8),other=MEM32(p+44);if((child&&!spatial_maintenance_virtual(child,28))||(other&&!spatial_maintenance_virtual(other,12)))return 0;}
        if(kind==3){uint32_t spatial=MEM32(object+64);if(!spatial_maintenance_virtual(object,12)||!spatial_maintenance_ring(object+40,0,0)||(spatial&&!spatial_maintenance_safe(spatial,80)))return 0;}
        p=MEM32(p);}return 1;
}
static int spatial_maintenance_admit(uint32_t a){
    if((a!=0x2459c0&&a!=0x246360&&a!=0x246e70)||!spatial_maintenance_enabled()||!spatial_maintenance_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!spatial_maintenance_span(g_esp-4096,4104)||!spatial_maintenance_safe(g_ecx,96))return 0;
    if(a==0x2459c0){uint32_t voice=MEM32(g_ecx+60);return !voice||spatial_maintenance_safe(voice,44);}
    if(a==0x246360)return spatial_maintenance_virtual(g_ecx,12)&&spatial_maintenance_ring(g_ecx+24,8,1);
    return spatial_maintenance_ring(g_ecx+20,0,2)&&spatial_maintenance_ring(g_ecx+12,32,3);
}
static void spatial_maintenance_counter(void){
    PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_ecx=MEM32(g_esi+60);MEM32(g_esp+4)=0;int playing=0;if(g_ecx){g_eax=g_esp+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2459dd);sub_002519C0();g_eax=MEM32(g_esp+4);playing=g_eax!=0;}
    if(playing){g_eax=MEM32(g_esi+80)-1;MEM32(g_esi+80)=g_eax;g_recomp_zf=g_eax==0;}else{MEM32(g_esi+80)=0;g_eax=MEM32(g_esi+80);}POP32(g_esp,g_esi);POP32(g_esp,g_ecx);g_esp+=4;
}
static void spatial_maintenance_sources(void){
    PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_ebx=g_ecx;g_esi=MEM32(g_ebx+24);PUSH32(g_esp,g_edi);g_edi=g_ebx+24;
    if(g_esi!=g_edi){do{g_ecx=MEM32(g_esi+52);g_eax=g_esi-8;g_esi=MEM32(g_esi);MEM32(g_esp+12)=0;int playing=0;if(g_ecx){g_eax=g_esp+12;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24638e);sub_002519C0();g_eax=MEM32(g_esp+12);playing=g_eax!=0;}if(!playing){g_edx=MEM32(g_ebx);g_ecx=g_ebx;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x24639d);RECOMP_ICALL_SAFE(target,saved);}}while(g_esi!=g_edi);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=8;
}
static void spatial_maintenance_sweep(void){
    uint32_t frame=g_seh_ebp;g_esp-=32;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_ebx=g_ecx;frame=g_ebx+20;PUSH32(g_esp,g_esi);g_esi=MEM32(frame);PUSH32(g_esp,g_edi);MEM32(g_esp+20)=g_ebx;
    if(g_esi!=frame){do{g_ecx=MEM32(g_esi+8);g_edi=MEM32(g_esi);if(g_ecx){g_eax=MEM32(g_ecx);uint32_t saved=g_esp,target=MEM32(g_eax+24);PUSH32(g_esp,0x246e95);RECOMP_ICALL_SAFE(target,saved);if((int32_t)g_eax>=7){g_eax=MEM32(g_esi+8);if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,0);target=MEM32(g_edx+20);PUSH32(g_esp,0x246eaa);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esi+8);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+8);PUSH32(g_esp,0x246eb2);RECOMP_ICALL_SAFE(target,saved);}g_eax=MEM32(g_esi+44);if(g_eax){g_ecx=g_eax;g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x246ec0);RECOMP_ICALL_SAFE(target,saved);}PUSH32(g_esp,g_esi);PUSH32(g_esp,0x246ec6);sub_00243F60();PUSH32(g_esp,g_esi);PUSH32(g_esp,0x246ecc);sub_002D5CCD();g_esp+=8;}}g_esi=g_edi;}while(g_edi!=frame);}
    g_edx=MEM32(g_ebx+12);g_ebx+=12;MEM32(g_esp+16)=0;int need_query=0;
    if(g_edx!=g_ebx){do{frame=MEM32(g_edx);g_esi=g_edx-32;g_eax=MEM32(g_esi+40);g_ecx=g_esi+40;g_edi=0;while(g_eax!=g_ecx){g_eax=MEM32(g_eax);g_edi++;}if(!g_edi){PUSH32(g_esp,g_edx);PUSH32(g_esp,0x246f12);sub_00243F60();g_eax=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x246f1c);RECOMP_ICALL_SAFE(target,saved);}else if(MEM8(g_esi+68)&2)MEM32(g_esp+16)++;g_edx=frame;}while(frame!=g_ebx);g_eax=MEM32(g_esp+16);need_query=g_eax!=0;}
    if(need_query){g_edx=MEM32(g_esp+20);PUSH32(g_esp,1);g_ecx=g_esp+28;PUSH32(g_esp,g_ecx);g_edx+=48;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x246f4a);sub_00240D20();g_ecx=g_eax;PUSH32(g_esp,0x246f51);sub_002537A0();if(LO8(g_eax)){g_eax=MEM32(g_ebx);if(g_eax!=g_ebx){do{if(MEM8(g_eax+36)&2){g_ecx=MEM32(g_eax+32);if(g_ecx){g_edx=MEM32(g_esp+24);MEM32(g_ecx+56)=g_edx;g_edx=MEM32(g_esp+28);MEM32(g_ecx+60)=g_edx;g_xmm0=XMM_SCALAR(MEMF(g_esp+32));MEMF(g_ecx+64)=g_xmm0.f[0];g_edx=MEM32(g_esp+36);MEM32(g_ecx+68)=g_edx;g_xmm0=XMM_SCALAR(MEMF(g_esp+40));MEMF(g_ecx+72)=g_xmm0.f[0];g_xmm0=XMM_SCALAR(MEMF(g_esp+44));MEMF(g_ecx+76)=g_xmm0.f[0];}}g_eax=MEM32(g_eax);}while(g_eax!=g_ebx);}}}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=32;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_audio_spatial_maintenance(uint32_t a){if(!spatial_maintenance_admit(a))return 0;if(a==0x2459c0)spatial_maintenance_counter();else if(a==0x246360)spatial_maintenance_sources();else spatial_maintenance_sweep();return 1;}
