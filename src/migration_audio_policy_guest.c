#include "migration_audio_policy_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024B6B0(void);
extern void sub_002488F0(void);
extern void sub_00245D90(void);
extern void sub_0024B600(void);
extern void sub_0023FDF0(void);
extern void sub_00248E80(void);
extern void sub_0024A9F0(void);
extern void sub_0024A900(void);
static int policy_enabled(void){static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_POLICY");enabled=v&&!strcmp(v,"1");}return enabled;}
static int policy_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int policy_safe(uint32_t p,uint32_t n){return policy_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+24<=p);}
static int policy_admit(uint32_t address){
    if(address!=0x240080||!policy_enabled()||!policy_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!policy_span(g_esp-4096,4120))return 0;
    if(!policy_safe(g_ecx,48)||!policy_safe(MEM32(g_esp+4),64)||!policy_safe(MEM32(g_esp+8),2)||!policy_safe(MEM32(g_esp+12),24)||!policy_safe(MEM32(g_esp+16),4))return 0;
    uint32_t head=g_ecx+40,p=MEM32(head),previous=head,count=0;
    /* ponytail: admission walks active voices in O(n); share validated list ownership when all publishers are native. */
    while(p!=head){if(++count>65535||p<24||!policy_safe(p-24,104)||MEM32(p+4)!=previous||!policy_span(MEM32(p-24),36)||!policy_safe(MEM32(p+56),24))return 0;previous=p;p=MEM32(p);}
    return MEM32(head+4)==previous;
}
/* The three retail scans differ in callback PCs and volatile register choices. */
static int policy_match(uint32_t*frame,unsigned pass){
    static const uint32_t pcs[3][7]={{0x240104,0x24010d,0x240123,0x240139,0x24014a,0x240153,0x24015b},{0x2401b8,0x2401c1,0x2401d7,0x2401ed,0x2401fe,0x240207,0x24020f},{0x240265,0x24026e,0x240284,0x24029d,0x2402ae,0x2402b7,0x2402bf}};
    if(pass==0)g_edx=MEM32(*frame-24);else g_eax=MEM32(*frame-24);g_esi=*frame-24;*frame=MEM32(*frame);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe7f0);g_ecx=g_esi;uint32_t target=MEM32(pass==0?g_edx:g_eax);PUSH32(g_esp,pcs[pass][0]);RECOMP_ICALL_SAFE(target,saved);
    g_edi=g_eax;if(pass==0)g_eax=MEM32(g_edi);else g_edx=MEM32(g_edi);g_ecx=g_edi;saved=g_esp;target=MEM32((pass==0?g_eax:g_edx)+32);PUSH32(g_esp,pcs[pass][1]);RECOMP_ICALL_SAFE(target,saved);
    g_ecx=MEM32(0x4fe848);if(g_ecx!=MEM32(g_eax))return 0;
    g_edx=MEM32(g_esp+24);PUSH32(g_esp,g_edx);g_ecx=g_esi;PUSH32(g_esp,pcs[pass][2]);sub_002488F0();if(!LO8(g_eax))return 0;
    SET_LO8(g_eax,MEM8(g_esp+19));if(pass==2)g_ebx=MEM32(g_esi+80);
    if(LO8(g_eax)){g_ecx=MEM32(g_esp+52);PUSH32(g_esp,g_esi);PUSH32(g_esp,pcs[pass][3]);sub_00245D90();if(!LO8(g_eax))return 0;}
    g_eax=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fe848);g_ecx=g_edi;target=MEM32(g_eax);PUSH32(g_esp,pcs[pass][4]);RECOMP_ICALL_SAFE(target,saved);
    g_edx=MEM32(g_esi);g_ecx=g_esi;g_edi=g_eax;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,pcs[pass][5]);RECOMP_ICALL_SAFE(target,saved);
    PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,pcs[pass][6]);sub_0024B600();return 1;
}
#define POLICY_RETURN(value) do{POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);SET_LO8(g_eax,value);POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=24;return 1;}while(0)
#define POLICY_FLAG_RETURN(mask) do{g_eax=MEM32(frame);POP32(g_esp,g_edi);g_eax|=(mask);POP32(g_esp,g_esi);MEM32(frame)=g_eax;POP32(g_esp,frame);SET_LO8(g_eax,1);POP32(g_esp,g_ebx);g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=24;return 1;}while(0)
int uc_migration_audio_policy(uint32_t address){
    if(!policy_admit(address))return 0;uint32_t frame=g_seh_ebp;g_esp-=16;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+40);g_eax=MEM16(g_esi+14);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_ecx=MEM32(g_esp+36);MEM32(g_esp+20)=g_edi;g_eax&=0x3fff;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2400a6);sub_0024B6B0();MEM32(g_esp+24)=g_eax;g_eax=MEM32(g_esp+52);MEM8(g_esp+19)=g_eax&&(MEM8(g_esi+21)&4);
    g_ecx=MEM32(g_esp+40);SET_LO16(g_ebx,MEM16(g_esi+14));SET_LO8(g_eax,MEM8(g_ecx));SET_LO16(g_ebx,LO16(g_ebx)>>14);
    if((int8_t)LO8(g_eax)<0){frame=MEM32(g_edi+40);g_eax=g_edi+40;MEM32(g_esp+28)=g_eax;
        if(frame!=g_eax){do{if(policy_match(&frame,0)&&g_eax==MEM32(g_esp+40)){if(!LO16(g_ebx)){POLICY_RETURN(0);}g_ebx+=0xffff;}}while(frame!=MEM32(g_esp+28));g_esi=MEM32(g_esp+44);}}
    g_ebx=MEM16(g_esi+14);SET_LO16(g_ebx,LO16(g_ebx)>>14);
    if(LO16(g_ebx)){g_eax=MEM32(g_esp+20);frame=MEM32(g_eax+40);g_eax+=40;g_ebx++;int quota=0;
        if(frame!=g_eax){for(;;){if(policy_match(&frame,1)&&g_eax==MEM32(g_esp+40)){if(!LO16(g_ebx)){quota=1;break;}g_ebx+=0xffff;}g_eax=MEM32(g_esp+20);g_eax+=40;if(frame==g_eax)break;}}
        if(!quota&&LO16(g_ebx)){POLICY_RETURN(1);}}
    g_eax=MEM32(g_esp+20);frame=MEM32(g_eax+40);g_eax+=40;
    for(;;){g_eax=MEM32(g_esp+20);g_eax+=40;if(frame==g_eax){POLICY_RETURN(1);}if(policy_match(&frame,2))break;}
    SET_LO8(g_edx,MEM8(g_ebx+8));MEM32(g_esp+52)=g_eax;g_eax=MEM32(g_esp+44);SET_LO8(g_ecx,MEM8(g_eax+8));if(LO8(g_ecx)<LO8(g_edx)){POLICY_RETURN(0);}
    g_edi=MEM32(g_esp+40);frame=MEM32(g_esp+48);if(MEM8(g_edi)&2)MEM32(frame)|=0x2000;
    SET_LO8(g_edx,MEM8(g_eax+8));if(LO8(g_edx)>MEM8(g_ebx+8)){
        g_eax=MEM16(g_eax+14);g_ecx=MEM32(g_esp+36);g_eax&=0x3fff;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24030c);sub_0024B6B0();g_ebx=MEM32(g_esp+20);g_edi=g_eax;PUSH32(g_esp,g_edi);g_ecx=g_ebx;PUSH32(g_esp,0x24031a);sub_0023FDF0();
        while(g_eax){g_edx=MEM32(g_eax);uint32_t saved=g_esp;PUSH32(g_esp,0);g_ecx=g_eax;uint32_t target=MEM32(g_edx+20);PUSH32(g_esp,0x240329);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_edi);g_ecx=g_ebx;PUSH32(g_esp,0x240331);sub_0023FDF0();}g_edi=MEM32(g_esp+40);
    }
    if(MEM8(g_edi)&1)MEM32(frame)|=0x8000;g_eax=MEM32(g_esp+52);g_ecx=g_esi;
    if(MEM8(g_eax)&4){g_edx=MEM32(g_esi);uint32_t saved=g_esp,target=MEM32(g_edx+28);PUSH32(g_esp,0x240355);RECOMP_ICALL_SAFE(target,saved);
        if(!(HI8(g_eax)&0x40)){PUSH32(g_esp,0x4000);g_ecx=g_esi;PUSH32(g_esp,0x240366);sub_00248E80();}
        if(!(MEM8(g_edi)&2)){POLICY_FLAG_RETURN(0x8000);}POLICY_RETURN(1);
    }
    if(MEM8(g_edi)&0x78){g_eax=MEM32(g_esi);uint32_t saved=g_esp,target=MEM32(g_eax+24);PUSH32(g_esp,0x240390);RECOMP_ICALL_SAFE(target,saved);
        if(g_eax!=4){g_ecx=MEM16(g_edi);PUSH32(g_esp,g_ecx);g_ecx=g_esi;PUSH32(g_esp,0x2403a2);sub_0024A9F0();if(LO8(g_eax)){POLICY_FLAG_RETURN(0x10000);}}
        POLICY_FLAG_RETURN(0x18000);
    }
    g_edx=MEM32(g_esi);uint32_t saved=g_esp,target=MEM32(g_edx+28);PUSH32(g_esp,0x2403d9);RECOMP_ICALL_SAFE(target,saved);
    if(!(HI8(g_eax)&0x40)){g_eax=MEM32(frame);if((int8_t)HI8(g_eax)>=0){PUSH32(g_esp,2);PUSH32(g_esp,0);PUSH32(g_esp,0);g_ecx=g_esi;PUSH32(g_esp,0x2403f2);sub_0024A900();}}
    POLICY_RETURN(1);
}
#undef POLICY_RETURN
#undef POLICY_FLAG_RETURN
