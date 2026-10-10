#ifndef UC_FLASHLIGHT_MATERIAL_CACHE_H
#define UC_FLASHLIGHT_MATERIAL_CACHE_H
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

/* B0880's call-free matrix product only. Original resource callbacks,
 * transpose, upload and all material/shadow draws remain in their source order. */
typedef struct UcSpotMaterialCache {
    uint32_t input[32],before,after,stack;
    RecompXmm xmm[8],scratch[6];
    unsigned valid,pending;
} UcSpotMaterialCache;
static RECOMP_TLS UcSpotMaterialCache uc_spot_material_cache;
static RECOMP_TLS int uc_spot_material_option=-1;
static int uc_spot_material_begin(uint32_t light,uint32_t stack){
    UcSpotMaterialCache*c=&uc_spot_material_cache;c->pending=0;
    if(uc_spot_material_option<0){const char*s=getenv("UCRR_HYBRID_FLASHLIGHT_MATERIAL_CACHE");uc_spot_material_option=s&&!strcmp(s,"1");}
    if(!uc_spot_material_option)return 0;
    /* These are ordinary mapped guest bytes. Keep aliased/write-combined and
     * overlapping objects on the unchanged original path. All exceptions must
     * be masked because replay cannot reproduce an intermediate FP trap. */
    const uint32_t csr=_mm_getcsr();
    if((csr&0x1F80)!=0x1F80||light<0x10000u||stack<0x10004u||
       (uint64_t)light+0x1F0>g_xbox_total_ram||(uint64_t)stack+0x90>g_xbox_total_ram||
       ((uint64_t)light+0x170<stack+0x90ull&&(uint64_t)stack-4<light+0x1F0ull))return 0;
    const void*input=(const void*)XBOX_PTR(light+0x170);
    if(c->valid&&c->before==csr&&!memcmp(c->input,input,sizeof(c->input))){
        g_xmm0=c->xmm[0];g_xmm1=c->xmm[1];g_xmm2=c->xmm[2];g_xmm3=c->xmm[3];
        g_xmm4=c->xmm[4];g_xmm5=c->xmm[5];g_xmm6=c->xmm[6];g_xmm7=c->xmm[7];
        XMM_STORE(stack+0x70,c->scratch[0]);XMM_STORE(stack+0x80,c->scratch[1]);
        g_eax=stack+0xC0;XMM_STORE(stack+0x30,c->scratch[2]);
        PUSH32(g_esp,g_eax);g_ecx=g_esp+0x34;
        XMM_STORE(g_esp+0x44,c->scratch[3]);XMM_STORE(g_esp+0x54,c->scratch[4]);XMM_STORE(g_esp+0x64,c->scratch[5]);
        _mm_setcsr(c->after);return 1;
    }
    c->valid=0;c->pending=1;c->before=csr;c->stack=stack;memcpy(c->input,input,sizeof(c->input));return 0;
}
static void uc_spot_material_end(void){
    UcSpotMaterialCache*c=&uc_spot_material_cache;if(!c->pending)return;
    c->after=_mm_getcsr();
    c->xmm[0]=g_xmm0;c->xmm[1]=g_xmm1;c->xmm[2]=g_xmm2;c->xmm[3]=g_xmm3;
    c->xmm[4]=g_xmm4;c->xmm[5]=g_xmm5;c->xmm[6]=g_xmm6;c->xmm[7]=g_xmm7;
    c->scratch[0]=XMM_MEM(c->stack+0x70);c->scratch[1]=XMM_MEM(c->stack+0x80);
    for(unsigned i=0;i<4;i++)c->scratch[i+2]=XMM_MEM(c->stack+0x30+i*16);
    c->valid=1;c->pending=0;
}
#endif
