/* Original default contact constraint sizing and island accounting. Writes
 * remain ordered: the info output may alias its input or island counters. */
#include "migration_physics_constraint_data.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
static int data_span(uint32_t p,uint32_t n) {
    return p>=0x10000 && p<0x80000000 && (uint64_t)p+n<=g_xbox_total_ram;
}
int uc_migration_physics_constraint_data(unsigned entry) {
    static int flag=-1;
    if(flag<0){const char*v=getenv("UCRR_NATIVE_PHYSICS_CONSTRAINT_DATA");flag=v&&!strcmp(v,"1");}
    if(!flag || (g_esp&3) || g_esp<0x10008 || !data_span(g_esp-8,24))return 0;
    if(entry==0x2ffed0) {
        if(!data_span(MEM32(g_esp+8),8))return 0;
        g_eax=MEM32(g_esp+8);MEM32(g_eax)=0;MEM32(g_eax+4)=0;g_esp+=12;return 1;
    }
    if(entry==0x2ffef0) {
        if(!data_span(g_ecx,64)||!data_span(MEM32(g_esp+4),16))return 0;
        g_eax=MEM32(g_esp+4);g_edx=0;
        MEM32(g_eax)=0;MEM32(g_eax+4)=0;MEM32(g_eax+12)=0;MEM32(g_eax+8)=24;
        g_ecx=MEM32(g_ecx+60);g_edx=48*g_ecx+128;
        PUSH32(g_esp,g_esi);MEM32(g_eax)=g_edx;g_edx=g_ecx+2;g_esi=3*g_edx;
        PUSH32(g_esp,g_edi);g_esi*=16;g_edi=4*g_ecx+44;
        MEM32(g_eax+4)=g_esi;MEM32(g_eax+8)=g_edi;MEM32(g_eax+12)=g_edx;
        if((int32_t)g_ecx>=2){g_esi+=32;g_edi+=4;++g_edx;MEM32(g_eax+4)=g_esi;MEM32(g_eax+8)=g_edi;MEM32(g_eax+12)=g_edx;}
        POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;return 1;
    }
    if((entry!=0x2fc8c0&&entry!=0x2fc910)||!data_span(g_ecx,28)||!data_span(MEM32(g_esp+8),16))return 0;
    if(entry==0x2fc8c0) {
        g_eax=MEM32(g_esp+8);g_edx=MEM32(g_eax+4);PUSH32(g_esp,g_esi);
        MEM32(g_ecx+12)-=g_edx;g_edx=MEM32(g_eax+8);g_esi=MEM32(g_ecx+16)-g_edx;
        g_edx=MEM32(g_ecx+24);MEM32(g_ecx+16)=g_esi;g_eax=MEM32(g_eax+12);g_edx-=g_eax;
        MEM32(g_ecx+24)=g_edx;POP32(g_esp,g_esi);g_recomp_zf=!g_edx;
    } else {
        g_edx=MEM32(g_esp+8);g_eax=MEM32(g_edx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx+8);
        if((int32_t)g_esi>(int32_t)g_eax)g_eax=g_esi;
        MEM32(g_ecx+8)=g_eax;g_esi=MEM32(g_edx+4);
        if((int32_t)g_eax<=(int32_t)g_esi)g_eax=g_esi;
        g_esi=MEM32(g_ecx+12);MEM32(g_ecx+8)=g_eax;g_eax=MEM32(g_edx+4);g_esi+=g_eax;
        MEM32(g_ecx+12)=g_esi;g_eax=MEM32(g_edx+8);g_esi=MEM32(g_ecx+16)+g_eax;
        g_eax=MEM32(g_ecx+24);MEM32(g_ecx+16)=g_esi;g_edx=MEM32(g_edx+12);g_eax+=g_edx;
        MEM32(g_ecx+24)=g_eax;POP32(g_esp,g_esi);g_recomp_zf=!g_eax;
    }
    g_esp+=12;return 1;
}
