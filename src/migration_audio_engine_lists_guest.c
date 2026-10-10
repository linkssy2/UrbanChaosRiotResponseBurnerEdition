#include "migration_audio_engine_lists_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_00247140(void);
extern void sub_002D5CCD(void);
extern void sub_00245D90(void);
static int engine_lists_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_ENGINE_LISTS");f=v&&!strcmp(v,"1");}return f;}
static int engine_lists_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int engine_lists_safe(uint32_t p,uint64_t n){return engine_lists_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int engine_lists_ring(uint32_t head,unsigned offset,int virtuals){
    /* ponytail: bounded O(n) admission; specialize only if measured cost warrants it. */
    uint32_t p=MEM32(head);unsigned budget=65536;while(p!=head){if(!budget--||p<offset||!engine_lists_safe(p-offset,96)||!engine_lists_safe(MEM32(p+4),8)||(virtuals&&!engine_lists_span(MEM32(p-offset),8)))return 0;p=MEM32(p);}return 1;
}
static int engine_lists_admit(uint32_t a){
    if((a!=0x240400&&a!=0x240620&&a!=0x240660&&a!=0x240810&&a!=0x240850&&a!=0x240870&&a!=0x240890&&a!=0x2408d0&&a!=0x240910)||!engine_lists_enabled()||!engine_lists_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!engine_lists_span(g_esp-4096,4108)||!engine_lists_safe(g_ecx,180))return 0;
    if(a==0x240660)return 1;
    if(a==0x240620)return engine_lists_ring(g_ecx+40,24,1);
    if(a==0x240810)return engine_lists_ring(g_ecx+64,0,0);
    if(a==0x240890||a==0x240910)return engine_lists_ring(g_ecx+(a==0x240890?72:88),4,0);
    uint32_t object=MEM32(g_esp+4);unsigned offset=a==0x240400?8:4,head=a==0x240400?56:a==0x2408d0?80:72;
    return engine_lists_safe(object,96)&&engine_lists_safe(MEM32(object+offset),8)&&engine_lists_safe(MEM32(object+offset+4),8)&&(a==0x240870||engine_lists_safe(MEM32(g_ecx+head+4),8))&&(a!=0x240400||engine_lists_span(MEM32(object),8));
}
static void engine_lists_add_source(void){
    uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=g_esi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24040f);RECOMP_ICALL_SAFE(target,saved);g_esi+=8;PUSH32(g_esp,g_esi);g_edi+=56;PUSH32(g_esp,g_edi);PUSH32(g_esp,0x24041c);sub_00243F40();g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void engine_lists_count(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+40);PUSH32(g_esp,g_edi);g_edi=g_ecx+40;frame=0;
    if(g_esi!=g_edi){PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+20);do{g_eax=MEM32(g_esi-24);g_ecx=g_esi-24;g_esi=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f4);uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x240645);RECOMP_ICALL_SAFE(target,saved);if(g_eax==g_ebx)frame++;}while(g_esi!=g_edi);POP32(g_esp,g_ebx);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_eax=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=8;
}
static void engine_lists_remove_loops(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=g_ecx+64;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx);
    if(g_esi!=g_ebx){PUSH32(g_esp,frame);frame=MEM32(g_esp+16);PUSH32(g_esp,g_edi);do{g_edi=g_esi;int match=MEM32(g_edi+28)==frame;g_esi=MEM32(g_esi);if(match){g_ecx=g_edi;PUSH32(g_esp,0x240831);sub_00247140();PUSH32(g_esp,g_edi);PUSH32(g_esp,0x240837);sub_002D5CCD();g_esp+=4;}}while(g_esi!=g_ebx);POP32(g_esp,g_edi);POP32(g_esp,frame);}
    POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
static void engine_lists_find_event(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_ebx=g_ecx+72;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx);PUSH32(g_esp,g_edi);int found=0;
    if(g_esi!=g_ebx){frame=MEM32(g_esp+20);do{g_edi=g_esi-4;PUSH32(g_esp,frame);g_ecx=g_edi;PUSH32(g_esp,0x2408ac);sub_00245D90();if(LO8(g_eax)){found=1;break;}g_esi=MEM32(g_esi);}while(g_esi!=g_ebx);}
    if(found){g_eax=g_edi;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=0;}else{POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;POP32(g_esp,g_ebx);g_recomp_zf=1;}g_esp+=8;
}
static void engine_lists_find_id(void){
    g_edx=g_ecx+88;g_ecx=MEM32(g_edx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);int found=0;
    if(g_ecx!=g_edx){g_esi=MEM32(g_esp+20);g_edi=MEM32(g_esp+16);do{g_ebx=MEM32(g_ecx+16);g_eax=g_ecx-4;if(g_ebx==g_edi&&(!g_esi||MEM32(g_eax+52)==g_esi)){found=1;break;}g_ecx=MEM32(g_ecx);}while(g_ecx!=g_edx);}
    if(!found)g_eax=0;POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=12;
}
int uc_migration_audio_engine_lists(uint32_t a){
    if(!engine_lists_admit(a))return 0;
    switch(a){case 0x240400:engine_lists_add_source();break;case 0x240620:engine_lists_count();break;
    case 0x240660:{SET_LO8(g_eax,MEM8(g_esp+8));unsigned mode=LO8(g_eax);g_eax=MEM32(g_ecx+176);int claimed=0;if(mode){g_edx=MEM32(g_esp+4);if(g_eax!=g_edx&&!g_eax){MEM32(g_ecx+176)=g_edx;claimed=1;}}else if(g_eax==MEM32(g_esp+4))MEM32(g_ecx+176)=0;SET_LO8(g_eax,claimed);g_recomp_zf=1;g_esp+=12;break;}
    case 0x240810:engine_lists_remove_loops();break;case 0x240890:engine_lists_find_event();break;case 0x240910:engine_lists_find_id();break;
    case 0x240870:g_eax=MEM32(g_esp+4)+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24087d);sub_00243F60();POP32(g_esp,g_ecx);g_recomp_zf=g_eax==0;g_esp+=8;break;
    default:g_eax=MEM32(g_esp+4);if(a==0x2408d0)MEM32(g_eax+12)++;g_eax+=4;PUSH32(g_esp,g_eax);g_ecx+=a==0x2408d0?80:72;PUSH32(g_esp,g_ecx);PUSH32(g_esp,a==0x2408d0?0x2408e4:0x240861);sub_00243F40();g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;break;}return 1;
}
