#include "migration_audio_loader_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_0024F2A0(void);
extern void sub_0024ACA0(void);
extern void sub_0024F300(void);
extern void sub_0024CCE0(void);
extern void sub_0024DCD0(void);
extern void sub_0024F560(void);
extern void sub_002D589F(void);
extern void sub_002D68FB(void);
extern void sub_002D57FB(void);
extern void sub_00241880(void);
extern void sub_0024D220(void);
extern void sub_00247E70(void);
extern void sub_0024F220(void);
static int loader_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_LOADER");f=v&&!strcmp(v,"1");}return f;}
static int loader_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int loader_safe(uint32_t p,uint32_t n){return loader_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int loader_admit(uint32_t a){
    if(a!=0x24fe60||!loader_enabled()||!loader_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!loader_span(g_esp-4096,4108)||!loader_safe(g_ecx,192))return 0;
    uint32_t input=MEM32(g_esp+4);return loader_safe(input,4)&&loader_span(MEM32(input),44);
}
static void loader_record(uint32_t at,uint32_t vtable,uint32_t output){MEM32(at)=vtable;MEM32(at+4)=0;MEM32(at+8)=0;MEM32(at+12)=output;}
static void loader_handlers(void){
    uint32_t base=g_esp;
    static const uint16_t records[]={0x104,0xe4,0xc4,0xa4,0x94,0x144,0xf4,0x134,0xd4,0x84,0xb4,0x174,0x114,0x154,0x164};
    static const uint16_t fields[]={0x74,0x78,0x7c,0x8c,0x98,0x80,0x84,0x88,0x9c,0x90,0xb4,0xb4,0xb8,0,0xbc};
    g_eax=g_edi+112;MEM32(base+0x124)=0x483ffc;MEM32(base+0x128)=g_eax;MEM32(base+0x12c)=0;MEM32(base+0x130)=0;
    g_eax=0x483ffc;g_ebx=g_edi+116;g_esi=0x484024;
    for(unsigned n=0;n<15;n++)loader_record(base+records[n],records[n]==0x94||records[n]==0xd4||records[n]==0x154?g_esi:g_eax,fields[n]?g_edi+fields[n]:base+28);
    g_ecx=g_edi?g_edi+4:0;MEM32(base+32)=0x484370;MEM32(base+36)=g_ecx;
    static const uint16_t actions[]={0x6c,0x60,0x78};static const uint32_t callbacks[]={0x24ad10,0x24dfa0,0x24ad30};
    for(unsigned n=0;n<3;n++){MEM32(base+actions[n])=0x484004;MEM32(base+actions[n]+4)=g_ecx;MEM32(base+actions[n]+8)=callbacks[n];}
    g_ecx=base+16;g_eax=base+0x124;g_edx=base+28;MEM8(base+0x2bc)=20;
    static const uint32_t tags[]={0x68706d61,0x76706d61,0x73706d61,0x33706d61,0x7a706d61,0x6c6b7274,0x77706d61,0x70706d61,0x6d706d61,0x71706d61,0x45564157,0x53564157,0x66726d61,0x64697567,0x65746164,0x656d616e,0x65646678,0x74636d61,0x6b706d61,0x43545061};
    static const uint16_t handlers[]={0x124,0x104,0xe4,0xc4,0xa4,0x94,0xd4,0x144,0xf4,0x134,0x20,0x20,0x84,0x6c,0x78,0x60,0xb4,0x114,0x154,0x164};
    static const uint32_t returns[]={0x2501d4,0x2501f5,0x250216,0x250237,0x250258,0x250279,0x25029a,0x2502bb,0x2502dc,0x2502fd,0x25031b,0x250339,0x25035a,0x250378,0x250396,0x2503b4,0x2503d5,0x2503f6,0x250417,0x250438};
    for(unsigned n=0;n<20;n++){
        uint32_t record=g_esp+handlers[n];
        if(n%3==0){g_ecx=g_esp+16;g_eax=record;PUSH32(g_esp,g_ecx);}
        else if(n%3==1){g_eax=g_esp+16;g_edx=record;PUSH32(g_esp,g_eax);}
        else{g_ecx=record;g_edx=g_esp+16;PUSH32(g_esp,g_edx);}
        g_ecx=g_esp+((n==5||n==6||n==10||n==11||n==18)?0x44:0x54);MEM32(g_esp+20)=tags[n];MEM32(g_esp+24)=record;PUSH32(g_esp,returns[n]);sub_0024DCD0();
    }
}
static void loader_destroy_handlers(void){
    g_eax=0x483ea0;g_ecx=0x483ea8;
    static const uint16_t plain[]={0x164,0x114,0xb4,0x78,0x60,0x6c,0x84,0x134,0xf4,0x144,0xa4,0xc4,0xe4,0x104,0x124};
    static const uint16_t lists[]={0x154,0x20,0xd4,0x94};
    for(unsigned n=0;n<15;n++)MEM32(g_esp+plain[n])=g_eax;for(unsigned n=0;n<4;n++)MEM32(g_esp+lists[n])=g_ecx;
}
static void loader_voice_index(void){
    g_esi=MEM32(g_eax+8);g_eax=g_esi*8+4;PUSH32(g_esp,g_eax);PUSH32(g_esp,0x250515);sub_002D589F();g_esp+=4;MEM32(g_esp+24)=g_eax;MEM8(g_esp+0x2bc)=21;
    if(g_eax){PUSH32(g_esp,0x24ce00);PUSH32(g_esp,0x24b5f0);PUSH32(g_esp,g_esi);MEM32(g_eax)=g_esi;g_eax+=4;PUSH32(g_esp,8);PUSH32(g_esp,g_eax);MEM32(g_esp+36)=g_eax;PUSH32(g_esp,0x250544);sub_002D68FB();g_eax=MEM32(g_esp+16);}else g_eax=0;
    g_edx=MEM32(g_ebx);MEM32(g_edi+148)=g_eax;g_eax=MEM32(g_edx+8);g_ecx=0;MEM8(g_esp+0x2bc)=20;MEM32(g_esp+24)=g_ecx;
    if(g_eax){MEM32(g_esp+16)=0;do{
        g_eax=MEM32(g_ebx);g_edx=MEM32(g_esp+16);g_eax+=12;g_eax=MEM16(g_edx+g_eax+14);g_edx=MEM32(g_edi+148);g_esi=g_edx+g_eax*8;
        if(MEM32(g_esi+4)==0xffffffffu)MEM32(g_esi+4)=g_ecx;
        else{while(MEM32(g_esi))g_esi=MEM32(g_esi);PUSH32(g_esp,8);PUSH32(g_esp,0x2505a3);sub_002D57FB();g_esp+=4;if(g_eax){MEM32(g_eax)=0;MEM32(g_eax+4)=0xffffffffu;}else g_eax=0;g_ecx=MEM32(g_esp+24);MEM32(g_esi)=g_eax;MEM32(g_eax+4)=g_ecx;}
        MEM32(g_esp+16)+=16;g_edx=MEM32(g_ebx);g_eax=MEM32(g_edx+8);g_ecx++;MEM32(g_esp+24)=g_ecx;
    }while(g_ecx<g_eax);}
}
static void loader_resources(void){
    g_eax=MEM32(g_edi+144);
    if(g_eax&&MEM32(g_eax+8)){
        PUSH32(g_esp,0);PUSH32(g_esp,0x2505f0);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fedd0);g_ecx=g_eax;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x2505ff);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_edi+144);g_esi=MEM32(g_ecx+8);g_ebx=0;MEM32(g_esp+16)=g_eax;g_edx=g_ecx+12;
        if(g_esi){g_esi=g_edx;for(;;){
            g_ecx=MEM32(g_edi+84);MEM32(g_esp+0x2a8)=g_ecx;g_edx=g_esi;g_ecx=MEM32(g_edx);MEM32(g_esp+0x28c)=g_ecx;g_ecx=MEM32(g_edx+4);MEM32(g_esp+0x290)=g_ecx;g_ecx=MEM32(g_edx+8);g_edx=MEM32(g_edx+12);MEM32(g_esp+0x294)=g_ecx;g_ecx=g_esp+0x184;MEM32(g_esp+0x298)=g_edx;g_edx=MEM32(g_eax);saved=g_esp;PUSH32(g_esp,g_ecx);g_ecx=g_eax;MEM32(g_esp+0x188)=0x104;target=MEM32(g_edx+4);PUSH32(g_esp,0x25066d);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esi+16)=g_eax;g_edx=MEM32(g_edi+144);g_eax=MEM32(g_edx+8);g_ebx++;g_esi+=20;if(g_ebx>=g_eax)break;g_eax=MEM32(g_esp+16);
        }}
    }
}
static void loader_groups(void){
    g_edx=MEM32(g_esp+28);
    if(g_edx){g_eax=MEM32(g_edi+184);if(g_eax&&MEM32(g_eax+8)){
        g_esi=MEM32(g_edx+4);g_ecx=4;g_eax=g_edx+12;
        if(g_esi>g_ecx)for(;;){
            g_edx=MEM32(g_eax+4);g_esi=MEM32(g_edi+184);g_ebx=g_eax+8;g_eax=MEM16(g_ebx);g_ecx+=g_edx+8;g_edx=g_eax+g_eax*4;g_esi+=12;g_eax=MEM16(g_esi+g_edx*4);g_esi+=g_edx*4;MEM32(g_esp+24)=g_ecx;g_ecx=MEM32(g_edi+140);g_edx=g_eax+g_ecx+8;
            MEM16(g_esp+44)=0;MEM16(g_esp+46)=0;MEM16(g_esp+48)=0;MEM32(g_esp+40)=g_edx;SET_LO16(g_eax,MEM16(g_esi+2));MEM16(g_esp+44)=LO16(g_eax);SET_LO16(g_ecx,MEM16(g_esi+4));MEM16(g_esp+46)=LO16(g_ecx);SET_LO16(g_edx,MEM16(g_esi+6));PUSH32(g_esp,0);MEM16(g_esp+52)=LO16(g_edx);PUSH32(g_esp,0x250722);sub_00241880();g_edx=MEM32(g_eax);g_esp+=4;g_ecx=g_esp+40;uint32_t saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,0x4fedd4);g_ecx=g_eax;uint32_t target=MEM32(g_edx+12);PUSH32(g_esp,0x250736);RECOMP_ICALL_SAFE(target,saved);
            g_ecx=g_eax;g_eax=g_ebx+4;MEM32(g_esi+16)=g_ecx;MEM32(g_esp+16)=g_ecx;
            if(MEM32(g_eax)==0x5453494c&&MEM32(g_eax+8)==0x6b706d61){
                g_esi=MEM32(g_esp+24);g_edx=MEM32(g_eax+4);g_edx+=g_esi+8;g_eax+=12;g_esi=0;MEM32(g_esp+24)=g_edx;
                if(MEM16(g_ebx+2)){for(;;){PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_eax);g_ecx=g_edi;g_esi++;PUSH32(g_esp,0x25077d);sub_0024D220();g_ecx=MEM16(g_ebx+2);if(g_esi>=g_ecx)break;g_ecx=MEM32(g_esp+16);}}
            }
            g_edx=MEM32(g_esp+28);g_ecx=MEM32(g_esp+24);if(g_ecx>=MEM32(g_edx+4))break;g_ecx=MEM32(g_esp+24);
        }
    }}
}
static void loader_link_groups(void){
    g_eax=MEM32(g_edi+188);if(g_eax){g_ecx=MEM32(g_eax+8);if(g_ecx){g_esi=0;g_ebx=g_eax+12;do{SET_LO16(g_eax,MEM16(g_ebx));if(LO16(g_eax)!=0xffff){g_edx=LO16(g_eax);g_eax=MEM32(g_edi+140);g_eax+=g_edx+8;PUSH32(g_esp,0);PUSH32(g_esp,g_eax);PUSH32(g_esp,0x2507cd);sub_00247E70();g_esp+=8;if(g_eax){MEM32(g_eax+116)=g_edi;MEM32(g_eax+120)=g_ebx;}}g_ecx=MEM32(g_edi+188);g_eax=MEM32(g_ecx+8);g_esi++;g_ebx+=8;}while(g_esi<g_eax);}}
}
static void loader_load(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,0x39ecf0);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;g_esp-=0x2a4;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;frame=0;PUSH32(g_esp,frame);g_ecx=g_esp+52;PUSH32(g_esp,0x24fe8c);sub_0024F2A0();g_esi=MEM32(g_esp+0x2c0);g_eax=MEM32(g_esi);g_ecx=g_esi;MEM32(g_esp+0x2b8)=frame;MEM32(g_esp+24)=frame;uint32_t saved=g_esp,target=MEM32(g_eax+40);PUSH32(g_esp,0x24fea5);RECOMP_ICALL_SAFE(target,saved);int opened=1;
    if(g_eax==2){g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+8);PUSH32(g_esp,0x24feb1);RECOMP_ICALL_SAFE(target,saved);}
    else{
        g_eax=MEM32(g_esp+0x2c4);PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);g_ecx=g_edi+4;PUSH32(g_esp,0x24fec4);sub_0024ACA0();
        if(!LO8(g_eax)){g_ecx=g_esp+48;MEM32(g_esp+0x2b8)=0xffffffffu;PUSH32(g_esp,0x24fedc);sub_0024F300();g_eax=0;opened=0;}
        else{g_ecx=0x51ce90;PUSH32(g_esp,0x24feed);sub_0024CCE0();if(g_eax){g_ecx=MEM32(g_edi+80);g_edx=MEM32(g_edi+76);MEM32(g_eax+12)=g_ecx;g_ecx=g_eax+20;MEM32(g_eax)=0x484048;MEM32(g_eax+4)=1;MEM32(g_eax+8)=g_edx;MEM32(g_eax+16)=frame;MEM32(g_eax+24)=g_ecx;MEM32(g_ecx)=g_ecx;}else g_eax=0;MEM32(g_esp+0x2c0)=g_eax;}
    }
    if(opened){
        g_eax=g_edi+112;PUSH32(g_esp,g_ebx);loader_handlers();g_ecx=MEM32(g_esp+0x2c8);g_edx=MEM32(g_esp+0x2c4);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esp+60;PUSH32(g_esp,0x250451);sub_0024F560();int version=1;
        if(MEM32(g_ebx)){g_eax=MEM32(g_edi+112);if(MEM16(g_eax)!=0x120){loader_destroy_handlers();g_ecx=g_esp+52;MEM32(g_esp+0x2bc)=0xffffffffu;PUSH32(g_esp,0x2504fe);sub_0024F300();g_eax=0;version=0;}else loader_voice_index();}
        if(version){loader_resources();loader_groups();loader_link_groups();g_ecx=MEM32(g_esp+0x2c4);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+12);PUSH32(g_esp,0x2507f7);RECOMP_ICALL_SAFE(target,saved);loader_destroy_handlers();MEM32(g_esp+52)=0x48436c;g_ecx=g_esp+72;MEM32(g_esp+0x2bc)=22;PUSH32(g_esp,0x250896);sub_0024F220();g_ecx=g_esp+56;MEM32(g_esp+0x2bc)=0xffffffffu;PUSH32(g_esp,0x2508aa);sub_0024F220();g_eax=g_edi;}
        POP32(g_esp,g_ebx);
    }
    g_ecx=MEM32(g_esp+0x2b0);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(0)=g_ecx;g_esp+=0x2b0;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_loader(uint32_t a){if(!loader_admit(a))return 0;loader_load();return 1;}
