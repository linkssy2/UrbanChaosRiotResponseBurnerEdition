#include "environment_early_tone.h"
#include <math.h>
#include <xmmintrin.h>
int uc_environment_early_tone(const UCEnvironmentEarlyToneInput *in,UCEnvironmentEarlyTonePlan *out){
    if(!in||!out||!in->width||!in->height||in->width>4096||in->height>4096||
       !isfinite(in->input_scale)||in->input_scale==0||!isfinite(in->post_scale)||
       fabsf(in->post_scale)>8192||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<4;i++)if(!isfinite(in->view[i])||fabsf(in->view[i])>8192)return 1;
    const float reciprocal=(float)(1.0/(double)in->input_scale);
    if(!isfinite(reciprocal))return 1;
    const float amount=(float)(1.0-(double)in->post_scale),gray=amount*0.3333333432674408f;
    const float x[2]={(float)(int32_t)in->view[0],(float)(int32_t)in->view[2]};
    const float y[2]={(float)(int32_t)in->view[1],(float)(int32_t)in->view[3]};
    UCEnvironmentEarlyTonePlan p={0};
    for(unsigned i=0;i<4;i++){
        const float px=x[i==1||i==2],py=y[i>=2];
        p.vertices[i]=(UCEnvironmentToneVertex){{px,py,0},{px,py}};
        p.constant4[i]=i==3?1:reciprocal;p.constant0[i]=i==3?amount:gray;
    }
    *out=p;return 0;
}
