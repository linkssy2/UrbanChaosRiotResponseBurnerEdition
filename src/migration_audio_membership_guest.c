#include "migration_audio_membership_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_00248960(void);
extern void sub_00244010(void);
static int membership_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_MEMBERSHIP");f=v&&!strcmp(v,"1");}return f;}
static int membership_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int membership_safe(uint32_t p,uint64_t n){return membership_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+8<=p);}
static int membership_object(uint32_t p){return membership_safe(p,96)&&membership_span(MEM32(p),12);}
static int membership_ring(uint32_t head,unsigned offset,int names){
    uint32_t p=MEM32(head);unsigned budget=65536;while(p!=head){if(!budget--||p<offset||!membership_object(p-offset)||!membership_safe(MEM32(p+4),8))return 0;if(names){uint32_t def=MEM32(p-offset+80),bank=MEM32(p-offset+60);if(def&&!membership_safe(def,16))return 0;if(bank&&(!membership_safe(bank,144)||!membership_span(MEM32(bank),36)))return 0;}p=MEM32(p);}return 1;
}
static int membership_admit(uint32_t a){
    if((a!=0x23f950&&a!=0x23f960&&a!=0x23f980&&a!=0x23fd10&&a!=0x23fd40&&a!=0x23fd60&&a!=0x23fdf0&&a!=0x23fe50&&a!=0x23fe80)||!membership_enabled()||!membership_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!membership_span(g_esp-4096,4104)||!membership_safe(g_ecx,64))return 0;
    if(a==0x23f950)return 1;if(a==0x23f960)return membership_span(MEM32(g_ecx),8);if(a==0x23f980)return membership_ring(g_ecx+24,16,0);
    if(a==0x23fd60||a==0x23fdf0){uint32_t name=MEM32(g_esp+4);if(name&&!membership_safe(name,1))return 0;return membership_ring(g_ecx+32,8,1)&&(a==0x23fdf0||membership_ring(g_ecx+40,24,1));}
    uint32_t object=MEM32(g_esp+4);unsigned offset=a==0x23fe50||a==0x23fe80?24:8;if(!membership_object(object)||!membership_safe(MEM32(object+offset),8)||!membership_safe(MEM32(object+offset+4),8))return 0;
    return a==0x23fd40||a==0x23fe80||membership_safe(MEM32(g_ecx+(offset==8?36:44)),8);
}
static void membership_add(unsigned offset,unsigned head,uint32_t retain_pc,uint32_t link_pc){
    uint32_t saved=g_esp;PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=g_esi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,retain_pc);RECOMP_ICALL_SAFE(target,saved);g_esi+=offset;PUSH32(g_esp,g_esi);g_edi+=head;PUSH32(g_esp,g_edi);PUSH32(g_esp,link_pc);sub_00243F40();g_esp+=8;POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void membership_remove(unsigned offset,uint32_t unlink_pc,uint32_t release_pc){
    PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+8);g_eax=g_esi+offset;PUSH32(g_esp,g_eax);PUSH32(g_esp,unlink_pc);sub_00243F60();g_edx=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,release_pc);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_esi);g_recomp_zf=g_esp==0;g_esp+=8;
}
static void membership_clear(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_ecx+24;PUSH32(g_esp,g_esi);g_esi=MEM32(frame);
    if(g_esi!=frame){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edi);do{g_ebx=MEM32(g_esi);PUSH32(g_esp,g_esi);g_edi=g_esi-16;PUSH32(g_esp,0x23f99b);sub_00243F60();g_esp+=4;g_esi=g_ebx;if(g_edi){g_eax=MEM32(g_edi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_edi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x23f9ad);RECOMP_ICALL_SAFE(target,saved);}}while(MEM32(frame)!=frame);POP32(g_esp,g_edi);POP32(g_esp,g_ebx);}
    POP32(g_esp,g_esi);POP32(g_esp,frame);g_esp+=4;
}
static void membership_named_active(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=g_ecx;g_ebx=frame+40;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx);MEM32(g_esp+12)=0;
    if(g_esi!=g_ebx){PUSH32(g_esp,g_edi);do{g_eax=MEM32(g_esi+56);g_edi=g_esi-24;if(g_eax&&MEM16(g_eax+14)!=0xffff){g_eax=MEM32(g_esp+24);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x23fd96);sub_00248960();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23fd9c);sub_00244010();g_esp+=8;if(!g_eax){MEM32(g_esp+16)=g_edi;break;}}g_esi=MEM32(g_esi);}while(g_esi!=g_ebx);POP32(g_esp,g_edi);}
    g_eax=MEM32(frame+32);g_ecx=frame+32;int keep=0;if(g_eax!=g_ecx){g_ecx=MEM32(g_esp+12);g_esi=g_eax-8;if(g_ecx==g_esi){PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23fdcb);sub_00243F60();g_edx=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_edx+8);PUSH32(g_esp,0x23fdd5);RECOMP_ICALL_SAFE(target,saved);keep=g_eax!=0;}}
    if(keep){g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=0;}
    else{POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;}g_esp+=8;
}
static void membership_named_pending(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ebx=g_ecx+32;PUSH32(g_esp,g_edi);g_edi=MEM32(g_ebx);int found=0;
    if(g_edi!=g_ebx){frame=MEM32(g_esp+20);do{g_eax=MEM32(g_edi+72);g_esi=g_edi-8;if(g_eax&&MEM16(g_eax+14)!=0xffff){PUSH32(g_esp,frame);g_ecx=g_esi;PUSH32(g_esp,0x23fe1b);sub_00248960();PUSH32(g_esp,g_eax);PUSH32(g_esp,0x23fe21);sub_00244010();g_esp+=8;if(!g_eax){found=1;break;}}g_edi=MEM32(g_edi);}while(g_edi!=g_ebx);}
    if(found){PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23fe3d);sub_00243F60();g_eax=MEM32(g_esi);g_esp+=4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x23fe47);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_esp==0;}
    else{POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);g_eax=0;POP32(g_esp,g_ebx);g_recomp_zf=1;}g_esp+=8;
}
int uc_migration_audio_membership(uint32_t a){
    if(!membership_admit(a))return 0;switch(a){case 0x23f950:g_eax=MEM32(g_ecx+32)+1;MEM32(g_ecx+32)=g_eax;g_recomp_zf=g_eax==0;g_esp+=4;break;case 0x23f960:g_eax=MEM32(g_ecx+12)-1;MEM32(g_ecx+12)=g_eax;if(!g_eax){g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x23f970);RECOMP_ICALL_SAFE(target,saved);g_eax=0;}g_esp+=4;break;case 0x23f980:membership_clear();break;case 0x23fd10:membership_add(8,32,0x23fd1f,0x23fd2c);break;case 0x23fd40:membership_remove(8,0x23fd4e,0x23fd58);break;case 0x23fe50:membership_add(24,40,0x23fe5f,0x23fe6c);break;case 0x23fe80:membership_remove(24,0x23fe8e,0x23fe98);break;case 0x23fd60:membership_named_active();break;default:membership_named_pending();break;}return 1;
}
