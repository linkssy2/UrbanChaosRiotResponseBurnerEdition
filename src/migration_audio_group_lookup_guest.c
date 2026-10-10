#include "migration_audio_group_lookup_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241880(void);
extern void sub_00244010(void);
extern void sub_00244090(void);
static int group_lookup_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_GROUP_LOOKUP");f=v&&!strcmp(v,"1");}return f;}
static int group_lookup_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int group_lookup_safe(uint32_t p,uint64_t n){return group_lookup_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int group_lookup_admit(uint32_t a){
    if((a!=0x247d60&&a!=0x247e70)||!group_lookup_enabled()||!group_lookup_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!group_lookup_span(g_esp-4096,4108))return 0;
    uint32_t name=MEM32(g_esp+4),parent=MEM32(g_esp+8);if(parent&&(!group_lookup_safe(parent,4)||!group_lookup_span(MEM32(parent),44)))return 0;
    /* ponytail: bounded name scan protects the retail 256-byte segment buffer and recursive stack footprint. */
    unsigned separators=0;for(unsigned n=0;n<240;n++){if(!group_lookup_safe(name+n,1))return 0;unsigned ch=MEM8(name+n);if(!ch)return 1;if(ch==MEM8(0x4807c4)&&++separators>8)return 0;}return 0;
}
static void group_lookup_enumerate(uint32_t return_pc,unsigned slot){
    g_eax=MEM32(g_edi);g_ecx=g_esp+12;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4fecac);g_ecx=g_edi;uint32_t target=MEM32(g_eax+slot);PUSH32(g_esp,return_pc);RECOMP_ICALL_SAFE(target,saved);
}
static void group_lookup_find(void){
    g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+28);MEM32(g_esp+12)=0;MEM32(g_esp+16)=0;int root=g_edi==0;
    if(root){PUSH32(g_esp,0);PUSH32(g_esp,0x247df4);sub_00241880();g_esp+=4;g_ecx=g_esp+12;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+20;g_edi=g_eax;g_eax=MEM32(g_edi);PUSH32(g_esp,g_edx);PUSH32(g_esp,0x4fecac);g_ecx=g_edi;uint32_t target=MEM32(g_eax+28);PUSH32(g_esp,0x247e0f);RECOMP_ICALL_SAFE(target,saved);}
    else group_lookup_enumerate(0x247d94,40);
    int found=0;if(LO8(g_eax)){g_ebx=MEM32(g_esp+24);do{
        g_ecx=MEM32(g_esp+12);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fecac);uint32_t target=MEM32(g_eax);PUSH32(g_esp,root?0x247e26:0x247daf);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_ebx);g_ecx=g_esi;target=MEM32(g_edx+8);PUSH32(g_esp,root?0x247e30:0x247db9);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);PUSH32(g_esp,root?0x247e36:0x247dbf);sub_00244010();g_esp+=8;if(!g_eax){found=1;break;}group_lookup_enumerate(root?0x247e53:0x247de0,root?28:40);
    }while(LO8(g_eax));}
    POP32(g_esp,g_edi);if(found)g_eax=g_esi;POP32(g_esp,g_esi);if(!found)g_eax=0;POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void group_lookup_path(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp&=0xfffffff8;g_esp-=260;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(frame+8);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x4807c4);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x247e8d);sub_00244090();g_ebx=g_eax;g_esp+=8;
    if(g_ebx){g_eax-=g_esi;g_ecx=g_esp+16;if(g_eax){g_ecx=g_eax;g_edx=g_ecx;g_ecx>>=2;g_edi=g_esp+16;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx*4);g_esi+=g_ecx*4;g_edi+=g_ecx*4;g_ecx=g_edx&3;memcpy((void*)XBOX_PTR(g_edi),(void*)XBOX_PTR(g_esi),g_ecx);g_esi+=g_ecx;g_edi+=g_ecx;g_ecx=g_esp+g_eax+16;}g_eax=MEM32(frame+12);MEM8(g_ecx)=0;PUSH32(g_esp,g_eax);g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);g_ebp=frame;PUSH32(g_esp,0x247ec7);group_lookup_find();g_esp+=8;
        if(!g_eax){POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=1;g_esp+=4;return;}
        PUSH32(g_esp,g_eax);g_ebx++;PUSH32(g_esp,g_ebx);g_ebp=frame;PUSH32(g_esp,0x247edd);group_lookup_path();g_esp+=8;
    }else{g_edx=MEM32(frame+12);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_esi);g_ebp=frame;PUSH32(g_esp,0x247ef1);group_lookup_find();g_esp+=8;}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_migration_audio_group_lookup(uint32_t a){if(!group_lookup_admit(a))return 0;if(a==0x247d60)group_lookup_find();else group_lookup_path();return 1;}
