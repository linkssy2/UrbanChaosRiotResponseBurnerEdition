#include "environment_tone_draw.h"
#include <math.h>
#include <xmmintrin.h>
int uc_environment_tone_draw(const UCEnvironmentToneDrawInput *in,UCEnvironmentToneDrawPlan *out){
    if(!in||!out||!in->width||!in->height||in->width>4096||in->height>4096||
       in->previous_available>1||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<4;i++)if(!isfinite(in->view[i])||fabsf(in->view[i])>8192)return 1;
    if(!isfinite(in->history)||fabsf(in->history)>8192)return 1;
    const float x[2]={(float)(int32_t)in->view[0],(float)(int32_t)in->view[2]};
    const float y[2]={(float)(int32_t)in->view[1],(float)(int32_t)in->view[3]};
    UCEnvironmentToneDrawPlan p={0};
    for(unsigned i=0;i<4;i++){
        const float px=x[i==1||i==2],py=y[i>=2];
        p.vertices[i]=(UCEnvironmentToneVertex){{px,py,0},{px,py}};
        p.history_constant[i]=in->history;
    }
    p.request_previous=in->history>0;p.bind_previous=p.request_previous&&in->previous_available;
    *out=p;return 0;
}
