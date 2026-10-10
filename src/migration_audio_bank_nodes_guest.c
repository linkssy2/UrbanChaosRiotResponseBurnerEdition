#include "migration_audio_bank_nodes_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243EE0(void);
extern void sub_00243F40(void);
extern void sub_00243F60(void);
static int bank_nodes_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_BANK_NODES");f=v&&!strcmp(v,"1");}return f;}
static int bank_nodes_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int bank_nodes_safe(uint32_t p,uint64_t n){return bank_nodes_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int bank_nodes_tree(uint32_t bank,uint32_t *cursor,unsigned depth){
    uint32_t at=*cursor,table=MEM32(bank+184);if(depth>64||!bank_nodes_safe(at,24)||!bank_nodes_safe(table,12))return 0;uint32_t index=MEM16(at+8);if(index>=MEM32(table+8)||!bank_nodes_safe(table+12u+20u*index,20))return 0;
    *cursor=at+12;if(MEM32(at+12)==0x5453494c&&MEM32(at+20)==0x6b706d61){*cursor=at+24;for(unsigned n=0;n<MEM16(at+10);n++)if(!bank_nodes_tree(bank,cursor,depth+1))return 0;}return 1;
}
static int bank_nodes_admit(uint32_t a){
    if((a!=0x24cce0&&a!=0x24d220)||!bank_nodes_enabled()||!bank_nodes_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!bank_nodes_span(g_esp-4096,4108)||!bank_nodes_safe(g_ecx,a==0x24cce0?44:192))return 0;
    if(a==0x24cce0){if(MEM32(g_ecx+16)>4096)return 0;for(unsigned n=28;n<=40;n+=4)if(!bank_nodes_safe(MEM32(g_ecx+n),8))return 0;return 1;}
    uint32_t parent=MEM32(g_esp+8),cursor=MEM32(g_esp+4);return bank_nodes_safe(parent,4)&&bank_nodes_span(MEM32(parent),40)&&bank_nodes_safe(MEM32(g_ecx+140),8)&&bank_nodes_tree(g_ecx,&cursor,0);
}
static void bank_nodes_pool(void){
    uint32_t frame=g_ebp;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_esi=MEM32(g_edi+28);frame=g_edi+28;
    if(g_esi==frame){g_edx=MEM32(g_edi+12);g_edx++;PUSH32(g_esp,g_ebx);MEM32(g_edi+12)=g_edx;PUSH32(g_esp,0x24ccfc);sub_00243EE0();g_ecx=MEM32(g_edi+16);g_edx=MEM32(g_eax);g_ecx=(g_ecx+1)*28;uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,g_ecx);g_ecx=g_eax;uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24cd0d);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;g_eax=g_esi+20;PUSH32(g_esp,g_eax);g_edx=g_edi+36;PUSH32(g_esp,g_edx);MEM32(g_esi+24)=g_eax;MEM32(g_eax)=g_eax;PUSH32(g_esp,0x24cd21);sub_00243F40();g_eax=MEM32(g_edi+16);g_esp+=8;g_ebx=0;
        if(g_eax){g_esi+=48;do{PUSH32(g_esp,g_esi);PUSH32(g_esp,frame);MEM32(g_esi+4)=g_esi;MEM32(g_esi)=g_esi;PUSH32(g_esp,0x24cd3c);sub_00243F40();g_eax=MEM32(g_edi+16);g_esp+=8;g_ebx++;g_esi+=28;}while(g_ebx<g_eax);}g_esi=MEM32(frame);POP32(g_esp,g_ebx);
    }
    PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24cd54);sub_00243F60();g_edx=MEM32(g_edi+20);g_ecx=MEM32(g_edi+24);g_esp+=4;g_edx++;g_eax=g_edx;MEM32(g_edi+20)=g_edx;int equal=g_eax==g_ecx;if(g_eax>g_ecx)MEM32(g_edi+24)=g_eax;POP32(g_esp,g_edi);g_eax=g_esi-20;POP32(g_esp,g_esi);POP32(g_esp,frame);g_recomp_zf=equal;g_esp+=4;
}
static void bank_nodes_group(void){
    uint32_t frame=g_seh_ebp;g_esp-=12;uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+32);g_eax=MEM16(g_edi+8);g_ebx=g_ecx;g_ecx=MEM32(g_ebx+184);g_edi+=8;g_eax+=g_eax*4;g_edx=MEM16(g_ecx+g_eax*4+12);g_esi=g_ecx+g_eax*4+12;g_eax=MEM32(g_ebx+140);g_ecx=g_eax+g_edx+8;g_eax=0;MEM16(g_esp+20)=LO16(g_eax);MEM16(g_esp+22)=LO16(g_eax);MEM16(g_esp+24)=LO16(g_eax);MEM32(g_esp+16)=g_ecx;SET_LO16(g_edx,MEM16(g_esi+2));MEM16(g_esp+20)=LO16(g_edx);SET_LO16(g_eax,MEM16(g_esi+4));MEM16(g_esp+22)=LO16(g_eax);SET_LO16(g_ecx,MEM16(g_esi+6));g_eax=g_esp+16;MEM16(g_esp+24)=LO16(g_ecx);g_ecx=MEM32(g_esp+36);g_edx=MEM32(g_ecx);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x4fedd4);uint32_t target=MEM32(g_edx+36);PUSH32(g_esp,0x24d293);RECOMP_ICALL_SAFE(target,saved);frame=g_eax;g_eax=g_edi+4;MEM32(g_esi+16)=frame;
    if(MEM32(g_eax)==0x5453494c&&MEM32(g_eax+8)==0x6b706d61){g_esi=0;g_eax+=12;if(MEM16(g_edi+2))do{PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);g_ecx=g_ebx;g_esi++;PUSH32(g_esp,0x24d2c1);bank_nodes_group();g_ecx=MEM16(g_edi+2);}while(g_esi<g_ecx);}
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_bank_nodes(uint32_t a){if(!bank_nodes_admit(a))return 0;if(a==0x24cce0)bank_nodes_pool();else bank_nodes_group();return 1;}
