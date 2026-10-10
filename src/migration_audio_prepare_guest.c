#include "migration_audio_prepare_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00245900(void);
extern void sub_00248A60(void);
extern void sub_00248B20(void);
extern void sub_00248B80(void);
extern void sub_00248BF0(void);
extern void sub_00248C50(void);
extern void sub_002495C0(void);
extern void sub_0024D000(void);
extern void sub_00251810(void);
extern void sub_00251860(void);
extern void sub_00251960(void);
extern void sub_002519A0(void);
extern void sub_002523A0(void);
extern void sub_00252B90(void);
extern void sub_00252CF0(void);
extern void uc_sound_bind_failure(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
static int prepare_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_PREPARE");f=v&&!strcmp(v,"1");}return f;}
static int prepare_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int prepare_admit(uint32_t a){
    if(a!=0x249cc0||!prepare_enabled()||!prepare_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!prepare_span(g_esp-4096,4108)||!prepare_span(g_ecx,0x70))return 0;
    if(!((uint64_t)g_ecx+0x70<=g_esp-4096||(uint64_t)g_esp+12<=g_ecx))return 0;
    return prepare_span(MEM32(g_esp+4),0x64)&&prepare_span(MEM32(g_esp+8),12);
}
/* Device identity is captured once for format/data/region binding. The loop
   owner deliberately reloads the wrapper's current device later. */
static uint32_t prepare_resident(uint32_t *frame){
    PUSH32(g_esp,0);PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x249d4f);sub_00248A60();MEM32(g_esp+44)=g_eax;if(!g_eax)return 0x249d4f;
    *frame=MEM32(g_eax+60);uint32_t device=*frame;MEM32(g_esp+20)=device;if(!device)return 0x249d5b;
    g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed88);g_ecx=g_esi;uint32_t target=MEM32(g_eax);PUSH32(g_esp,0x249d77);RECOMP_ICALL_SAFE(target,saved);
    g_esi=g_eax;g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+52);PUSH32(g_esp,0x249d80);RECOMP_ICALL_SAFE(target,saved);if(!g_eax)return 0x249d80;
    g_eax=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_eax+76);PUSH32(g_esp,0x249d8f);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esp+36)=g_eax;if(!g_eax)return 0x249d8f;
    PUSH32(g_esp,g_eax);g_ecx=device;PUSH32(g_esp,0x249da3);sub_00251810();g_eax=MEM32(device);g_edx=MEM32(g_esi);g_ecx=g_esi;MEM32(g_esp+24)=g_eax;saved=g_esp;target=MEM32(g_edx+56);PUSH32(g_esp,0x249db1);RECOMP_ICALL_SAFE(target,saved);
    saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi);g_ecx=g_esi;target=MEM32(g_eax+52);PUSH32(g_esp,0x249db9);RECOMP_ICALL_SAFE(target,saved);
    saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+32);g_ecx=device;target=MEM32(g_eax+16);PUSH32(g_esp,0x249dc3);RECOMP_ICALL_SAFE(target,saved);
    g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+64);PUSH32(g_esp,0x249dca);RECOMP_ICALL_SAFE(target,saved);
    saved=g_esp;PUSH32(g_esp,g_eax);g_eax=MEM32(g_esi);g_ecx=g_esi;target=MEM32(g_eax+60);PUSH32(g_esp,0x249dd2);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);g_ecx=device;PUSH32(g_esp,0x249dda);sub_00251960();
    g_eax=MEM32(g_edi+68);if(g_eax&&(MEM8(g_eax+68)&1)){g_ecx=0;SET_LO16(g_ecx,MEM16(0x4839b0));PUSH32(g_esp,g_ecx);g_ecx=device;PUSH32(g_esp,0x249df8);sub_002523A0();}
    g_ecx=MEM32(g_edi+60);g_edx=MEM32(g_ecx);saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x249e00);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);
    if(g_ecx!=MEM32(g_eax)){g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_edx+72);PUSH32(g_esp,0x249e13);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esp+48);g_eax=g_eax?0:1;MEM16(g_ecx+8)=LO16(g_eax);}
    g_edx=MEM32(g_esp+48);
    if(MEM16(g_edx+8)!=1){
        g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_eax+72);PUSH32(g_esp,0x249e34);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_esi;
        if(!g_eax){g_edx=MEM32(g_esi);saved=g_esp;target=MEM32(g_edx+64);PUSH32(g_esp,0x249e3f);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);}
        else{g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);target=MEM32(g_eax+72);PUSH32(g_esp,0x249e4b);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_edx+68);PUSH32(g_esp,0x249e55);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);}
        g_ecx=device;PUSH32(g_esp,0x249e5d);sub_002519A0();
    }
    g_eax=MEM32(g_edi+80);if(g_eax&&(MEM8(g_eax+20)&2)){PUSH32(g_esp,0);g_ecx=device;PUSH32(g_esp,0x249e73);sub_00251860();}
    g_eax=MEM32(g_esi);g_ecx=g_esp+28;saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_ecx=g_esi;target=MEM32(g_eax+80);PUSH32(g_esp,0x249e84);RECOMP_ICALL_SAFE(target,saved);
    g_ecx=MEM32(g_esp+28);g_edx=MEM32(g_esp+32);g_eax=MEM32(device);saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=device;target=MEM32(g_eax+44);PUSH32(g_esp,0x249e96);RECOMP_ICALL_SAFE(target,saved);
    *frame=MEM32(g_esp+36);g_eax=MEM32(*frame+4);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x249ea5);sub_00248BF0();g_esi=MEM32(g_esp+48);g_ecx=0;MEM16(g_ebx+82)=LO16(g_eax);SET_LO16(g_ecx,MEM16(g_esi+8));PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,0x249ebd);sub_00245900();if(MEM16(g_esi+8)!=1)MEM32(g_edi+100)|=0x20;
    g_edx=MEM32(g_esp+44);PUSH32(g_esp,*frame);PUSH32(g_esp,g_edx);PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x249ed6);sub_00248B20();PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x249ede);sub_002495C0();PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x249ee6);sub_00248C50();g_eax=0;SET_LO8(g_eax,MEM8(g_ebx+72));SET_LO8(g_eax,LO8(g_eax)>>2);g_ecx=g_edi;g_eax&=0xffffff01u;PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ebx);PUSH32(g_esp,0x249efc);sub_00248B80();return 0;
}
static uint32_t prepare_stream(uint32_t *frame){
    PUSH32(g_esp,0x40000000);PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x249f25);sub_00248A60();MEM32(g_esp+24)=g_eax;if(!g_eax)return 0x249f25;
    *frame=MEM32(g_eax+60);uint32_t device=*frame;if(!device)return 0x249f31;
    g_edx=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed8c);g_ecx=g_esi;uint32_t target=MEM32(g_edx);PUSH32(g_esp,0x249f49);RECOMP_ICALL_SAFE(target,saved);g_esi=g_eax;g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);PUSH32(g_esp,0x4fed58);g_ecx=g_esi;target=MEM32(g_eax);PUSH32(g_esp,0x249f58);RECOMP_ICALL_SAFE(target,saved);if(!g_eax)return 0x249f58;
    PUSH32(g_esp,g_eax);g_ecx=device;PUSH32(g_esp,0x249f68);sub_00252B90();g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+84);PUSH32(g_esp,0x249f6f);RECOMP_ICALL_SAFE(target,saved);MEM32(g_esp+28)=g_eax;if(!g_eax)return 0x249f6f;
    PUSH32(g_esp,g_eax);g_ecx=device;PUSH32(g_esp,0x249f83);sub_00251810();g_eax=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_eax+64);PUSH32(g_esp,0x249f8a);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edx+60);PUSH32(g_esp,0x249f92);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);g_ecx=device;PUSH32(g_esp,0x249f9a);sub_00251960();
    g_ecx=MEM32(g_edi+60);g_eax=MEM32(g_ecx);saved=g_esp;target=MEM32(g_eax+32);PUSH32(g_esp,0x249fa2);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);
    if(g_ecx!=MEM32(g_eax)){g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_edx+80);PUSH32(g_esp,0x249fb5);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(g_esp+48);g_eax=g_eax?0:1;MEM16(g_ecx+8)=LO16(g_eax);}
    g_edx=MEM32(g_esp+48);
    if(MEM16(g_edx+8)!=1){
        g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_eax+80);PUSH32(g_esp,0x249fd6);RECOMP_ICALL_SAFE(target,saved);g_ecx=g_esi;
        if(!g_eax){g_edx=MEM32(g_esi);saved=g_esp;target=MEM32(g_edx+64);PUSH32(g_esp,0x249fe1);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);PUSH32(g_esp,0);}
        else{g_eax=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,0);target=MEM32(g_eax+80);PUSH32(g_esp,0x249fed);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);PUSH32(g_esp,0);g_ecx=g_esi;target=MEM32(g_edx+76);PUSH32(g_esp,0x249ff7);RECOMP_ICALL_SAFE(target,saved);PUSH32(g_esp,g_eax);}
        g_ecx=device;PUSH32(g_esp,0x249fff);sub_002519A0();
    }
    g_eax=MEM32(g_esi);g_ebx=MEM32(device);g_ecx=g_esi;saved=g_esp;target=MEM32(g_eax+56);PUSH32(g_esp,0x24a009);RECOMP_ICALL_SAFE(target,saved);g_edx=MEM32(g_esi);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=g_esi;target=MEM32(g_edx+52);PUSH32(g_esp,0x24a011);RECOMP_ICALL_SAFE(target,saved);saved=g_esp;PUSH32(g_esp,g_eax);g_ecx=device;target=MEM32(g_ebx+16);PUSH32(g_esp,0x24a017);RECOMP_ICALL_SAFE(target,saved);
    g_eax=MEM32(g_edi+80);if(g_eax&&(MEM8(g_eax+20)&2)){PUSH32(g_esp,0);g_ecx=device;PUSH32(g_esp,0x24a02d);sub_00251860();}
    g_eax=MEM32(g_esi);g_ecx=g_esp+36;saved=g_esp;PUSH32(g_esp,g_ecx);g_edx=g_esp+36;PUSH32(g_esp,g_edx);g_ecx=g_esi;target=MEM32(g_eax+88);PUSH32(g_esp,0x24a03e);RECOMP_ICALL_SAFE(target,saved);
    g_ecx=MEM32(g_esp+36);g_edx=MEM32(g_esp+32);g_eax=MEM32(device);saved=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=device;target=MEM32(g_eax+44);PUSH32(g_esp,0x24a050);RECOMP_ICALL_SAFE(target,saved);
    g_esi=MEM32(g_esp+28);g_eax=MEM32(g_esi+4);PUSH32(g_esp,g_eax);g_ecx=g_edi;PUSH32(g_esp,0x24a05f);sub_00248BF0();g_ebx=MEM32(g_esp+44);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+28);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_ebx);g_ecx=g_edi;MEM16(g_ebx+82)=LO16(g_eax);PUSH32(g_esp,0x24a075);sub_00248B20();
    g_ecx=MEM32(g_esp+48);g_edx=0;SET_LO16(g_edx,MEM16(g_ecx+8));g_ecx=g_esi;PUSH32(g_esp,g_edx);PUSH32(g_esp,0x24a087);sub_00245900();g_eax=MEM32(g_esp+48);if(MEM16(g_eax+8)!=1)MEM32(g_edi+100)|=0x20;else{g_ecx=device;PUSH32(g_esp,0x24a09f);sub_00252CF0();}
    PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x24a0a7);sub_002495C0();PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x24a0af);sub_00248C50();g_ecx=0;SET_LO8(g_ecx,MEM8(g_ebx+72));SET_LO8(g_ecx,LO8(g_ecx)>>2);g_ecx&=0xffffff01u;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);g_ecx=g_edi;PUSH32(g_esp,0x24a0c6);sub_00248B80();return 0;
}
static void prepare_voice(void){
    uint32_t frame=g_seh_ebp,gate=0;g_esp-=24;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+32);int skip=(MEM8(g_ebx+73)&0x12)!=0;PUSH32(g_esp,g_edi);g_edi=g_ecx;
    if(!skip){g_eax=MEM32(g_ebx+52);g_eax=g_eax?MEM16(g_eax+4):0;skip=(LO8(g_eax)&2)!=0;}
    if(!skip){
        uint32_t saved=g_esp;PUSH32(g_esp,frame);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edi+60);frame=0xffff;MEM32(g_esp+16)=frame;g_eax=MEM32(g_esi);g_ecx=g_esi;uint32_t target=MEM32(g_eax+32);PUSH32(g_esp,0x249d03);RECOMP_ICALL_SAFE(target,saved);g_ecx=MEM32(0x4fed94);
        if(g_ecx==MEM32(g_eax)){g_eax=MEM32(g_esp+48);g_edx=g_esp+16;PUSH32(g_esp,g_edx);PUSH32(g_esp,g_eax);g_ecx=g_ebx;PUSH32(g_esp,0x249d1e);sub_0024D000();g_esi=g_eax;}
        else{g_ecx=MEM32(g_edi+60);MEM32(g_ebx+56)=g_ecx;}
        if(!g_esi)gate=0x249d28;
        else{
            g_edx=MEM32(g_esi);g_ecx=g_esi;saved=g_esp;target=MEM32(g_edx+32);PUSH32(g_esp,0x249d37);RECOMP_ICALL_SAFE(target,saved);g_eax=MEM32(g_eax);
            if(MEM32(0x4fed88)==g_eax)gate=prepare_resident(&frame);
            else if(MEM32(0x4fed8c)==g_eax)gate=prepare_stream(&frame);
            else if(MEM32(0x4fed94)==g_eax){if(MEM16(g_esp+16)==LO16(frame))gate=0x24a0d6;else MEM8(g_ebx+73)|=0x20;}
        }
        if(gate)uc_sound_bind_failure(gate,g_eax,g_esi,g_edi,g_ebx,frame);else MEM8(g_ebx+73)|=0x10;
        POP32(g_esp,g_esi);POP32(g_esp,frame);
    }
    POP32(g_esp,g_edi);SET_LO8(g_eax,gate?0:1);POP32(g_esp,g_ebx);g_esp+=24;g_recomp_zf=g_esp==0;g_esp+=12;
}
int uc_migration_audio_prepare(uint32_t a){if(!prepare_admit(a))return 0;prepare_voice();return 1;}
