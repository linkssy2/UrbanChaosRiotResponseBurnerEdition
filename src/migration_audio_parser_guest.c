#include "migration_audio_parser_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024F1B0(void);
extern void sub_0024F220(void);
static int parser_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PARSER");f=v&&!strcmp(v,"1");}return f;}
static int parser_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int parser_safe(uint32_t p,uint64_t n){return parser_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int parser_admit(uint32_t a){
    if((a!=0x24f380&&a!=0x24f560&&a!=0x24e590)||!parser_enabled()||!parser_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!parser_span(g_esp-4096,4108)||!parser_safe(g_ecx,a==0x24e590?16:44))return 0;
    if(a==0x24e590){uint32_t count=MEM32(g_ecx+12);return count&&count<=65536&&parser_safe(MEM32(g_ecx+8),(uint64_t)count*8)&&parser_safe(MEM32(g_esp+4),4)&&parser_safe(MEM32(g_esp+8),4);}
    uint32_t input=MEM32(g_esp+4);if(!parser_safe(input,4)||!parser_span(MEM32(input),36)||(a==0x24f560&&!parser_safe(MEM32(g_esp+8),12)))return 0;
    uint32_t owner=MEM32(g_ecx+40);if(owner){if(!parser_safe(owner,36))return 0;for(unsigned offset=12;offset<=28;offset+=16){uint32_t count=MEM32(owner+offset+4),table=MEM32(owner+offset);if(!count||count>65536||!parser_safe(table,(uint64_t)count*8))return 0;}}
    return 1;
}
static void parser_stream(void);
static void parser_chunk(void);
static void parser_lookup(void){g_edx=MEM32(g_ecx+8);g_eax=MEM32(g_ecx+12);g_eax=g_edx+g_eax*8-8;if(g_edx!=g_eax){g_ecx=MEM32(g_esp+8);g_ecx=MEM32(g_ecx);while(g_ecx!=MEM32(g_edx)){g_edx+=8;if(g_edx==g_eax)break;}}g_eax=MEM32(g_esp+4);MEM32(g_eax)=g_edx;g_esp+=12;}
static void parser_stream_handler(void){
    g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_ecx);g_eax=g_esp+16;uint32_t saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_edi);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24f427);RECOMP_ICALL_SAFE(target,saved);
}
static void parser_stream(void){
    uint32_t frame=g_seh_ebp,saved=g_esp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39eae8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=60;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+88);g_eax=MEM32(g_edi);g_ebx=g_ecx;PUSH32(g_esp,1);PUSH32(g_esp,4);g_ecx=g_esp+24;PUSH32(g_esp,g_ecx);g_ecx=g_edi;MEM32(g_esp+24)=g_ebx;uint32_t target=MEM32(g_eax+32);PUSH32(g_esp,0x24f3b5);RECOMP_ICALL_SAFE(target,saved);
    g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,4);g_eax=g_esp+28;PUSH32(g_esp,g_eax);g_ecx=g_edi;target=MEM32(g_edx+32);PUSH32(g_esp,0x24f3c5);RECOMP_ICALL_SAFE(target,saved);g_esi=MEM32(g_esp+20);g_eax=MEM32(g_esp+16);g_esi=(g_esi+1)&0xfffffffe;MEM32(g_esp+20)=g_esi;MEM32(g_ebx+36)=g_esi;
    if(g_eax!=0x46464952&&g_eax!=0x5453494c){
        g_ebx=MEM32(g_ebx+40);if(g_ebx){g_ecx=g_esp+16;PUSH32(g_esp,g_ecx);g_edx=g_esp+92;PUSH32(g_esp,g_edx);g_ecx=g_ebx+20;PUSH32(g_esp,0x24f3ff);parser_lookup();g_eax=MEM32(g_eax);}else g_eax=MEM32(g_esp+88);
        int handler=0;if(g_ebx){g_ecx=MEM32(g_ebx+32);g_edx=MEM32(g_ebx+28);g_ecx=g_edx+g_ecx*8-8;handler=g_eax!=g_ecx;}
        if(handler)parser_stream_handler();else{g_edx=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_esi);g_ecx=g_edi;target=MEM32(g_edx+28);PUSH32(g_esp,0x24f445);RECOMP_ICALL_SAFE(target,saved);}
    }else{
        g_eax=MEM32(g_edi);saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,4);g_ecx=g_esp+32;PUSH32(g_esp,g_ecx);g_ecx=g_edi;target=MEM32(g_eax+32);PUSH32(g_esp,0x24f469);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_ebx+40);
        if(g_ecx){g_eax=MEM32(g_ecx+12);g_edx=MEM32(g_ecx+16);g_esi=g_eax+g_edx*8-8;if(g_eax!=g_esi){g_esi=MEM32(g_esp+24);g_edx=g_eax+g_edx*8-8;while(g_esi!=MEM32(g_eax)){g_eax+=8;if(g_eax==g_edx)break;}}}else g_eax=MEM32(g_esp+88);
        int handler=0;if(g_ecx){g_edx=MEM32(g_ecx+16);g_ecx=MEM32(g_ecx+12);g_ecx+=g_edx*8-8;handler=g_eax!=g_ecx;}
        if(handler)parser_stream_handler();else{
            g_eax=MEM32(g_esp+20);g_esi=4;if(g_eax>g_esi){PUSH32(g_esp,frame);frame=0x48436c;do{
                g_eax=MEM32(g_esp+16);g_ebx=MEM32(g_eax+40);g_ecx=g_esp+36;if(!g_ebx)g_ebx=g_eax;MEM32(g_esp+32)=frame;PUSH32(g_esp,0x24f4dd);sub_0024F1B0();g_ecx=g_esp+52;MEM32(g_esp+84)=0;PUSH32(g_esp,0x24f4ee);sub_0024F1B0();MEM32(g_esp+72)=g_ebx;g_ecx=g_esp+20;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edi);g_ecx=g_esp+40;MEM32(g_esp+92)=1;PUSH32(g_esp,0x24f509);parser_chunk();g_edx=MEM32(g_esp+68);g_esi+=g_edx+8;MEM32(g_esp+32)=frame;g_ecx=g_esp+52;MEM32(g_esp+84)=2;PUSH32(g_esp,0x24f526);sub_0024F220();g_ecx=g_esp+36;MEM32(g_esp+84)=0xffffffff;PUSH32(g_esp,0x24f537);sub_0024F220();
            }while(g_esi<MEM32(g_esp+24));POP32(g_esp,frame);}
            g_ecx=MEM32(g_esp+72);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;g_esp+=72;g_recomp_zf=g_esp==0;g_esp+=8;return;
        }
    }
    POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+60);MEM32(0)=g_ecx;g_esp+=72;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void parser_chunk(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39eae8);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=48;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+72);g_eax=MEM32(g_ebx+4);g_edx=g_ecx;MEM32(g_edx+36)=g_eax;g_eax=MEM32(g_ebx);PUSH32(g_esp,g_esi);MEM32(g_esp+8)=g_edx;
    if(g_eax!=0x46464952&&g_eax!=0x5453494c){
        g_esi=MEM32(g_edx+40);if(g_esi){PUSH32(g_esp,g_ebx);g_ecx=g_esp+80;PUSH32(g_esp,g_ecx);g_ecx=g_esi+20;PUSH32(g_esp,0x24f5af);parser_lookup();g_eax=MEM32(g_eax);}else g_eax=MEM32(g_esp+72);
        int handler=0;if(g_esi){g_edx=MEM32(g_esi+32);g_ecx=MEM32(g_esi+28);g_edx=g_ecx+g_edx*8-8;handler=g_eax!=g_edx;}
        if(handler){g_ecx=MEM32(g_eax+4);g_edx=MEM32(g_esp+72);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x24f5d7);RECOMP_ICALL_SAFE(target,saved);}
        else{g_ecx=MEM32(g_esp+72);g_edx=MEM32(g_ebx+4);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,1);PUSH32(g_esp,g_edx);uint32_t target=MEM32(g_eax+28);PUSH32(g_esp,0x24f5f9);RECOMP_ICALL_SAFE(target,saved);}
        POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+48);MEM32(0)=g_ecx;g_esp+=60;g_recomp_zf=g_esp==0;g_esp+=12;return;
    }
    g_ecx=MEM32(g_edx+40);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
    if(g_ecx){g_eax=MEM32(g_ecx+12);g_esi=MEM32(g_ecx+16);g_edi=g_eax+g_esi*8-8;if(g_eax!=g_edi){g_edi=MEM32(g_ebx+8);g_esi=g_eax+g_esi*8-8;while(g_edi!=MEM32(g_eax)){g_eax+=8;if(g_eax==g_esi)break;}}}else g_eax=MEM32(g_esp+80);
    int handler=0;if(g_ecx){g_esi=MEM32(g_ecx+16);g_ecx=MEM32(g_ecx+12);g_ecx+=g_esi*8-8;handler=g_eax!=g_ecx;}
    if(handler){g_ecx=MEM32(g_eax+4);g_eax=MEM32(g_esp+80);g_edx=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);uint32_t target=MEM32(g_edx+4);PUSH32(g_esp,0x24f661);RECOMP_ICALL_SAFE(target,saved);POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_ecx=MEM32(g_esp+48);}
    else{
        g_eax=MEM32(g_ebx+4);g_esi=4;if(g_eax>g_esi){frame=MEM32(g_esp+80);int first=1;do{if(!first)g_edx=MEM32(g_esp+16);first=0;g_edi=MEM32(g_edx+40);g_ecx=g_esp+24;if(!g_edi)g_edi=g_edx;MEM32(g_esp+20)=0x48436c;PUSH32(g_esp,0x24f6a9);sub_0024F1B0();g_ecx=g_esp+40;MEM32(g_esp+72)=0;PUSH32(g_esp,0x24f6ba);sub_0024F1B0();MEM32(g_esp+60)=g_edi;PUSH32(g_esp,frame);g_ecx=g_esp+24;MEM32(g_esp+76)=1;PUSH32(g_esp,0x24f6d0);parser_stream();g_ecx=MEM32(g_esp+56);g_esi+=g_ecx+8;MEM32(g_esp+20)=0x48436c;g_ecx=g_esp+40;MEM32(g_esp+72)=2;PUSH32(g_esp,0x24f6f1);sub_0024F220();g_ecx=g_esp+24;MEM32(g_esp+72)=0xffffffff;PUSH32(g_esp,0x24f702);sub_0024F220();}while(g_esi<MEM32(g_ebx+4));}
        g_ecx=MEM32(g_esp+64);POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);
    }
    MEM32(0)=g_ecx;g_esp+=60;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_parser(uint32_t a){if(!parser_admit(a))return 0;if(a==0x24f380)parser_stream();else if(a==0x24f560)parser_chunk();else parser_lookup();return 1;}
