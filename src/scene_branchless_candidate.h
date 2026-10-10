#ifndef UCRR_SCENE_BRANCHLESS_CANDIDATE_H
#define UCRR_SCENE_BRANCHLESS_CANDIDATE_H
#include <stdlib.h>
/* Startup option, checked once per existing callback-bounded scan batch. */
static int uc_scene_branchless_option=-1;
static __forceinline int uc_scene_branchless_enabled(void){
    if(uc_scene_branchless_option<0){
        const char*s=getenv("UCRR_SCENE_BRANCHLESS");
        uc_scene_branchless_option=s&&s[0]=='1'&&!s[1];
    }
    return uc_scene_branchless_option;
}
#endif
