#include "environment_clear.h"
int uc_environment_clear(const UCEnvironmentClearInput *in,UCEnvironmentClearPlan *out){
    if(!in||!out||in->owner_present>1||in->reserved)return 1;
    const unsigned color=in->owner_present||in->collector12||in->collector14;
    UCEnvironmentClearPlan p={color?0x10101u:0,(color?0xF0u:0)|(in->collector10?3u:0),in->color&0xFFFFFFu,1,0};
    *out=p;return 0;
}
