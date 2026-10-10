#include "migration_services_math_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>

static int enabled(void) {
    static int flag=-1;
    if(flag<0){const char*p=getenv("UCRR_NATIVE_SERVICES_MATH");flag=p&&!strcmp(p,"1");}
    return flag;
}
static int span(uint32_t p,unsigned n) {
    return p>=0x10000&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;
}
/* The title's 250-word XOR lag generator owns its state in guest RAM. */
static void random_step(void) {
    unsigned a=MEM32(0x51e0f8)+1,b=MEM32(0x504670)+1;
    if(a==250)a=0;if(b==250)b=0;
    MEM32(0x51e0f8)=a;MEM32(0x504670)=b;
    g_ecx=MEM32(0x51dd10+4*b);
    g_edx=MEM32(0x51dd10+4*a)^g_ecx;
    MEM32(0x51dd10+4*a)=g_edx;g_eax=g_edx;g_recomp_zf=g_edx==0;
}
static void expand_bounds(void) {
    uint32_t box=g_ecx,point=MEM32(g_esp+4),scratch=g_esp-12;
    /* Sequential comparisons preserve input/output aliasing and unordered
     * comparisons: NaNs must never select a new endpoint. */
    for(unsigned axis=0;axis<3;axis++){
        double low=MEMF(box+4*axis),value=MEMF(point+4*axis);
        if(low==low&&value==value&&low>value)MEM32(box+4*axis)=MEM32(point+4*axis);
        else {
            double high=MEMF(box+12+4*axis);
            if(value==value&&high==high&&value>high)MEM32(box+12+4*axis)=MEM32(point+4*axis);
        }
    }
    g_xmm3=XMM_SCALAR(MEMF(0x44c1f8));
    RecompXmm v[3];
    for(unsigned i=0;i<3;i++)v[i]=XMM_SCALAR(MEMF(box+12+4*i));
    for(unsigned i=0;i<3;i++)v[i].f[0]=v[i].f[0]+MEMF(box+4*i);
    for(unsigned i=0;i<3;i++){
        v[i].f[0]=v[i].f[0]*g_xmm3.f[0];MEMF(scratch+4*i)=v[i].f[0];MEM32(box+24+4*i)=MEM32(scratch+4*i);
    }
    for(unsigned i=0;i<3;i++)v[i]=XMM_SCALAR(MEMF(box+12+4*i));
    for(unsigned i=0;i<3;i++)v[i].f[0]=v[i].f[0]-MEMF(box+4*i);
    for(unsigned i=0;i<3;i++){
        v[i].f[0]=v[i].f[0]*g_xmm3.f[0];MEMF(scratch+4*i)=v[i].f[0];MEM32(box+36+4*i)=MEM32(scratch+4*i);
    }
    g_xmm0=v[0];g_xmm1=v[1];g_xmm2=v[2];g_eax=MEM32(scratch+8);g_edx=MEM32(scratch+4);
    g_ecx=box+36;g_recomp_zf=g_esp==0;g_esp+=8;
}
int uc_migration_services_math(unsigned entry) {
    if(!enabled())return 0;
    uint32_t s=g_esp;
    if(!span(s-16,32)||!span(0x51e0f8,4)||MEM8(0x51c178))return 0;
    if(entry==0x2d3a10){
        uint32_t p=MEM32(s+4),b=g_ecx;
        if(!span(p,12)||!span(b,48))return 0;
        if((uint64_t)p<s+16u&&(uint64_t)p+12>s-16u)return 0;
        if((uint64_t)b<s+16u&&(uint64_t)b+48>s-16u)return 0;
        expand_bounds();return 1;
    }
    if(entry!=0x2d3670&&entry!=0x2d37b0)return 0;
    if(MEM32(0x51e0f8)>=250||MEM32(0x504670)>=250)return 0;
    uint32_t low=MEM32(s+4),width=MEM32(s+8)-low;
    if(entry==0x2d37b0){
        if(!width)return 0;
        unsigned a=(MEM32(0x51e0f8)+1)%250,b=(MEM32(0x504670)+1)%250;
        if(width==0xffffffffu&&(MEM32(0x51dd10+4*a)^MEM32(0x51dd10+4*b))==0x80000000u)return 0;
        MEM32(s-4)=g_esi;MEM32(s-8)=0x2d37b6;
    }
    random_step();
    if(entry==0x2d37b0){g_ecx=low;g_edx=(uint32_t)((int32_t)g_eax%(int32_t)width);g_eax=g_edx+low;g_recomp_zf=g_eax==0;}
    g_esp+=4;return 1;
}
