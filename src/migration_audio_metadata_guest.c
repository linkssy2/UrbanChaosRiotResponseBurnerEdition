#include "migration_audio_metadata_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00244010(void);
static int metadata_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_METADATA");f=v&&!strcmp(v,"1");}return f;}
static int metadata_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int metadata_safe(uint32_t p,uint32_t n){return metadata_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int metadata_string(uint32_t p){for(unsigned n=0;n<4096;n++){if(!metadata_safe(p+n,1))return 0;if(!MEM8(p+n))return 1;}return 0;}
static int metadata_admit(uint32_t a){
    if(!metadata_enabled()||!metadata_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!metadata_span(g_esp-4096,4108))return 0;
    if(a==0x24d190){
        if(!metadata_safe(g_ecx,80))return 0;uint32_t definition=MEM32(g_ecx+52),bank=MEM32(g_ecx+48);if(!metadata_safe(definition,4)||!metadata_safe(bank,156))return 0;
        if(MEM8(g_ecx+73)&2)return 1;
        uint64_t command=(uint64_t)MEM32(bank+152)+MEM16(definition+2)+MEM16(g_ecx+76)+12;uint32_t index=MEM8(g_ecx+79),count=MEM16(definition);
        do{if(command>0x7fffffffu||!metadata_safe((uint32_t)command,12))return 0;uint32_t opcode=MEM8((uint32_t)command+4);if(opcode<=MEM32(g_esp+8)&&opcode>=MEM32(g_esp+4))return 1;command+=12+MEM8((uint32_t)command+5);}while(++index<count);return 1;
    }
    switch(a){case 0x24b600:case 0x24b620:case 0x24b650:case 0x24b670:case 0x24b690:case 0x24b6b0:case 0x24cb20:case 0x24b950:case 0x24bd20:case 0x24bd60:case 0x24bda0:break;default:return 0;}
    if(!metadata_safe(g_ecx,184))return 0;
    if(a==0x24b950){uint32_t head=g_ecx+104,node=MEM32(head),previous=head;for(unsigned n=0;node!=head;n++){if(n>=4096||node<4||!metadata_safe(node-4,44)||MEM32(node+4)!=previous||!metadata_span(MEM32(node-4),12))return 0;previous=node;node=MEM32(node);}return MEM32(head+4)==previous;}
    if(a==0x24bd20||a==0x24bd60){uint64_t p=(uint64_t)MEM32(g_ecx+152)+MEM32(g_esp+4)+12;uint32_t remaining=MEM32(g_esp+8);for(unsigned n=0;n<4096;n++){if(p>0x7fffffffu||!metadata_safe((uint32_t)p,8))return 0;if(MEM32((uint32_t)p)!=0x74706d61)return 1;if(a==0x24bd20&&!remaining--)return 1;if(a==0x24bd60&&p+8==remaining)return 1;p+=MEM32((uint32_t)p+4)+8ull;}return 0;}
    if(a==0x24bda0){uint32_t table=MEM32(g_ecx+128);if(!table)return 1;if(!metadata_safe(table,12))return 0;uint32_t count=MEM32(table+8);if(count>4096||!metadata_safe(table+12,count*12)||!metadata_string(MEM32(g_esp+8)))return 0;for(uint32_t n=0;n<count;n++){uint64_t text=(uint64_t)MEM32(g_ecx+140)+8+MEM16(table+12+n*12+8);if(text>0x7fffffffu||!metadata_string((uint32_t)text))return 0;}return 1;}
    return 1;
}
static void metadata_find_source(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_ebx=g_ecx+104;PUSH32(g_esp,g_edi);g_edi=MEM32(g_ebx);int found=0;
    if(g_edi!=g_ebx){frame=MEM32(g_esp+20);do{g_eax=MEM32(g_edi-4);g_esi=g_edi-4;g_ecx=g_esi;uint32_t saved=g_esp,target=MEM32(g_eax+8);PUSH32(g_esp,0x24b96c);RECOMP_ICALL_SAFE(target,saved);if(g_eax==frame){g_ecx=MEM32(g_esp+24);if(MEM32(g_esi+40)==g_ecx){found=1;break;}}g_edi=MEM32(g_edi);}while(g_edi!=g_ebx);}
    POP32(g_esp,g_edi);if(found)g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,frame);if(!found)g_eax=0;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=12;
}
static void metadata_find_curve(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);g_ebx=g_ecx;g_eax=MEM32(g_ebx+128);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);int found=0;
    if(g_eax){g_ecx=MEM32(g_eax+8);g_esi=g_eax+12;g_edi=0;MEM32(g_esp+16)=g_esi;if(g_ecx){frame=MEM32(g_esp+28);for(;;){SET_LO16(g_eax,MEM16(g_esp+24));if(MEM16(g_esi)==LO16(g_eax)){g_ecx=MEM16(g_esi+8);g_edx=MEM32(g_ebx+140);g_eax=g_ecx+g_edx+8;PUSH32(g_esp,frame);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x24bde4);sub_00244010();g_esp+=8;if(!g_eax){found=1;break;}}g_ecx=MEM32(g_ebx+128);g_eax=MEM32(g_ecx+8);g_edi++;g_esi+=12;if(g_edi>=g_eax)break;}}}
    if(found){g_eax=MEM32(g_esp+16);g_edx=g_edi+g_edi*2;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);if(found)g_eax+=g_edx*4;else g_eax=0;POP32(g_esp,g_ebx);POP32(g_esp,g_ecx);g_recomp_zf=1;g_esp+=12;
}
static void metadata_chunk(uint32_t a){
    if(a==0x24bd20){g_eax=MEM32(g_ecx+152);g_ecx=MEM32(g_esp+4);g_eax+=g_ecx+12;g_ecx=MEM32(g_esp+8);int valid=1;while(g_ecx){g_edx=MEM32(g_eax);g_ecx--;if(g_edx!=0x74706d61){valid=0;break;}g_edx=MEM32(g_eax+4);g_eax+=g_edx+8;}if(!valid||MEM32(g_eax)!=0x74706d61)g_eax=0;else g_eax+=8;g_recomp_zf=g_eax==0;}
    else{g_edx=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+152);g_ecx+=g_edx+12;g_edx=MEM32(g_ecx);g_eax=0xffffffffu;PUSH32(g_esp,g_esi);int found=0;if(g_edx==0x74706d61){g_edx=MEM32(g_esp+12);for(;;){g_esi=g_ecx+8;g_eax++;if(g_edx==g_esi){found=1;break;}g_esi=MEM32(g_ecx+4);g_ecx+=g_esi+8;if(MEM32(g_ecx)!=0x74706d61)break;}}if(!found)g_eax=0xffffffffu;POP32(g_esp,g_esi);}g_esp+=12;
}
static void metadata_find_command(void){
    int direct=(MEM8(g_ecx+73)&2)!=0;g_eax=MEM32(g_ecx+52);g_edx=MEM16(g_ecx+76);g_eax=MEM16(g_eax+2);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM8(g_ecx+79);
    if(direct){g_ecx=MEM32(g_ecx+48);g_eax+=MEM32(g_ecx+152);POP32(g_esp,g_edi);g_eax+=g_edx+12;POP32(g_esp,g_esi);g_recomp_zf=g_eax==0;}
    else{g_esi=MEM32(g_ecx+48);g_esi=MEM32(g_esi+152);PUSH32(g_esp,g_ebx);g_ebx=g_eax+g_esi;for(;;){g_esi=MEM8(g_ebx+g_edx+16);g_eax=g_ebx+g_edx+12;if(g_esi<=MEM32(g_esp+20)&&g_esi>=MEM32(g_esp+16))break;g_eax=MEM8(g_eax+5);g_edx+=g_eax+12;g_eax=MEM32(g_ecx+52);g_eax=MEM16(g_eax);g_edi++;if(g_edi>=g_eax){g_eax=0;break;}}POP32(g_esp,g_ebx);POP32(g_esp,g_edi);POP32(g_esp,g_esi);}g_esp+=12;
}
int uc_migration_audio_metadata(uint32_t a){
    if(!metadata_admit(a))return 0;
    if(a==0x24b950)metadata_find_source();else if(a==0x24bda0)metadata_find_curve();else if(a==0x24bd20||a==0x24bd60)metadata_chunk(a);else if(a==0x24d190)metadata_find_command();
    else{
        switch(a){
        case 0x24b600:g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+116);g_eax<<=4;g_eax+=g_ecx+12;g_recomp_zf=g_eax==0;break;
        case 0x24b620:g_eax=MEM32(g_ecx+180);g_ecx=MEM32(g_esp+4);if(!g_eax&&g_ecx>=MEM32(8)){g_eax=0;g_recomp_zf=1;}else{g_ecx=g_ecx+g_ecx*2+3;g_eax+=g_ecx*4;}break;
        case 0x24b650:g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+124);g_eax+=g_eax*4;g_eax=g_ecx+g_eax*8+12;break;
        case 0x24b670:g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+132);g_eax=g_eax+g_eax*2+3;g_eax=g_ecx+g_eax*4;break;
        case 0x24b690:g_eax=MEM32(g_ecx+136);g_ecx=MEM32(g_esp+4);g_eax+=g_ecx*4+12;break;
        case 0x24b6b0:g_eax=MEM32(g_ecx+140);g_ecx=MEM32(g_esp+4);g_eax+=g_ecx+8;break;
        default:g_eax=MEM32(g_esp+4);g_ecx=MEM32(g_ecx+120);g_eax*=28;g_eax+=g_ecx+12;break;
        }g_esp+=8;
    }return 1;
}
