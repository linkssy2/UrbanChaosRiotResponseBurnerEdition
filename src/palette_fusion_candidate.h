#ifndef UCRR_PALETTE_FUSION_CANDIDATE_H
#define UCRR_PALETTE_FUSION_CANDIDATE_H
/* Startup-only experiment. No persistent palette or guest-state cache. */
#include <stdlib.h>
static int uc_palette_fusion_option=-1;
static __forceinline int uc_palette_fusion_enabled(void){
    if(uc_palette_fusion_option<0){
        const char*s=getenv("UCRR_PALETTE_FUSION");
        uc_palette_fusion_option=s&&s[0]=='1'&&!s[1];
    }
    return uc_palette_fusion_option;
}
#endif
