#include "migration_audio_timing_nodes_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00243F40(void);
extern void sub_00243F60(void);
extern void sub_002D57FB(void);
extern void sub_002D5CCD(void);
static int timing_nodes_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_TIMING_NODES");f=v&&!strcmp(v,"1");}return f;}
static int timing_nodes_span(uint32_t p,uint64_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int timing_nodes_safe(uint32_t p,uint64_t n){return timing_nodes_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+20<=p);}
static int timing_nodes_admit(uint32_t a){
    if((a!=0x244670&&a!=0x2447d0&&a!=0x244f00&&a!=0x244f40)||!timing_nodes_enabled()||!timing_nodes_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!timing_nodes_span(g_esp-4096,4116)||!timing_nodes_safe(g_ecx,12)||!timing_nodes_safe(MEM32(g_ecx+4),8))return 0;
    uint32_t p=MEM32(g_ecx);unsigned budget=65536;while(p!=g_ecx){if(!budget--||!timing_nodes_safe(p,64)||!timing_nodes_safe(MEM32(p+4),8))return 0;p=MEM32(p);}return 1;
}
static void timing_nodes_lookup(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(g_ecx);if(g_eax==g_ecx){g_eax=0;g_recomp_zf=1;g_esp+=16;return;}PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+20);PUSH32(g_esp,g_edi);g_edx=g_eax;
    if(g_esi==1){g_edi=MEM32(g_esp+16);g_esi=0;while((int32_t)MEM32(g_edx+48)<(int32_t)g_edi){g_esi=g_edx;g_edx=MEM32(g_edx);if(g_edx==g_ecx)break;}if(g_edx==g_ecx)g_recomp_zf=1;else if(g_esi&&(int32_t)g_edi<(int32_t)MEM32(g_edx+48)){g_eax=g_esi;g_recomp_zf=0;}else g_eax=g_edx;}
    else if(!g_esi){g_edi=MEM32(g_esp+20);g_ebx=MEM32(g_esp+16);PUSH32(g_esp,frame);for(;;){frame=MEM32(g_edx+12);int before=(int32_t)frame<(int32_t)g_edi;if(frame==g_edi){frame=MEM32(g_edx+8);before=frame<g_ebx;}if(!before)break;g_esi=g_edx;g_edx=MEM32(g_edx);if(g_edx==g_ecx)break;}if(g_edx==g_ecx)g_recomp_zf=1;else{int before=(int32_t)g_edi<(int32_t)MEM32(g_edx+12)||(g_edi==MEM32(g_edx+12)&&g_ebx<MEM32(g_edx+8));g_eax=g_esi&&before?g_esi:g_edx;}POP32(g_esp,frame);}
    else{g_eax=0;g_recomp_zf=1;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_esp+=16;
}
static void timing_nodes_insert(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=MEM32(g_esp+16);PUSH32(g_esp,frame);frame=MEM32(g_esp+16);PUSH32(g_esp,g_esi);g_esi=g_ecx;g_eax=MEM32(g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+32);
    while(g_eax!=g_esi){if(MEM32(g_eax+8)==frame&&MEM32(g_eax+12)==g_ebx&&MEM32(g_eax+16)==g_edi){g_ecx=MEM32(g_esp+20);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);MEM32(g_eax+20)=g_ecx;POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=20;return;}g_eax=MEM32(g_eax);}
    PUSH32(g_esp,24);PUSH32(g_esp,0x244804);sub_002D57FB();g_esp+=4;if(g_eax){g_edx=MEM32(g_esp+20);MEM32(g_eax+8)=frame;MEM32(g_eax+12)=g_ebx;MEM32(g_eax+16)=g_edi;MEM32(g_eax+20)=g_edx;MEM32(g_eax+4)=g_eax;MEM32(g_eax)=g_eax;}else g_eax=0;
    PUSH32(g_esp,g_eax);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x244839);sub_00243F40();g_eax=MEM32(g_esi+8);g_esp+=8;POP32(g_esp,g_edi);g_eax++;MEM32(g_esi+8)=g_eax;POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_recomp_zf=g_eax==0;g_esp+=20;
}
static void timing_nodes_remove(int wide){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);g_ebx=g_ecx;PUSH32(g_esp,g_esi);g_esi=MEM32(g_ebx);
    if(g_esi!=g_ebx){PUSH32(g_esp,frame);frame=MEM32(g_esp+16);PUSH32(g_esp,g_edi);do{g_edi=g_esi;g_esi=MEM32(g_esi);if(!frame||MEM32(g_edi+(wide?60:20))==frame){PUSH32(g_esp,g_edi);PUSH32(g_esp,wide?0x244f23:0x244f63);sub_00243F60();PUSH32(g_esp,g_edi);PUSH32(g_esp,wide?0x244f29:0x244f69);sub_002D5CCD();g_eax=MEM32(g_ebx+8);g_esp+=8;g_eax--;MEM32(g_ebx+8)=g_eax;}}while(g_esi!=g_ebx);POP32(g_esp,g_edi);POP32(g_esp,frame);}POP32(g_esp,g_esi);POP32(g_esp,g_ebx);g_recomp_zf=1;g_esp+=8;
}
int uc_migration_audio_timing_nodes(uint32_t a){if(!timing_nodes_admit(a))return 0;if(a==0x244670)timing_nodes_lookup();else if(a==0x2447d0)timing_nodes_insert();else timing_nodes_remove(a==0x244f00);return 1;}
