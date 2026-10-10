#include "firstperson_input.h"
#include <math.h>
#include <xmmintrin.h>

UCMotionStatus uc_firstperson_input_movement_step(UCFirstpersonInputMotion *state,
    const UCFirstpersonInputResolved *input, UCFirstpersonInputMovement *out) {
    UCFirstpersonInputMotion next;
    UCFirstpersonInputMovement result;
    UCMotionStatus status;
    if(!state||!input||!out) return UC_MOTION_ARGUMENT;
    if(input->alternate_movement) return UC_MOTION_UNSUPPORTED;
    if(!isfinite(input->movement[0])||!isfinite(input->movement[1])||!isfinite(input->delta)||
       input->delta<0||input->delta>1||!isfinite(state->angular_input)) return UC_MOTION_ARGUMENT;
    if((_mm_getcsr()&0xE040u)!=0) return UC_MOTION_UNSUPPORTED;
    next=*state;
    status=uc_motion_smoother_step(&next.movement[0],input->movement[0],input->delta);
    if(status) return status;
    if(next.angular_input<=.01f) next.angular_input=0;
    else {
        double decay=((double)input->delta*next.angular_input)*6.0f;
        next.angular_input=(float)((double)next.angular_input-decay);
        if(!isfinite(next.angular_input)) return UC_MOTION_UNSUPPORTED;
    }
    status=uc_motion_smoother_step(&next.movement[1],input->movement[1],input->delta);
    if(status) return status;
    result.movement[0]=next.movement[0].current;
    result.movement[1]=next.movement[1].current;
    for(unsigned i=0;i<2;++i) {
        status=uc_motion_smoother_step(&next.recoil[i],result.movement[i],input->delta);
        if(status) return status;
    }
    result.blend=(float)(1.0-((fabs((double)result.movement[0])-fabs((double)result.movement[1]))+1.0)*.5);
    if(input->crouched){result.movement[0]*=.5f;result.movement[1]*=.3f;}
    if(!isfinite(result.blend)) return UC_MOTION_UNSUPPORTED;
    *state=next;*out=result;return UC_MOTION_OK;
}

UCMotionStatus uc_firstperson_input_spring_targets(UCMotionSpring springs[2],
    const float axes[2],uint32_t aim_state) {
    float first,second;
    if(!springs||!axes||!isfinite(axes[0])||!isfinite(axes[1])) return UC_MOTION_ARGUMENT;
    if((_mm_getcsr()&0xE040u)!=0) return UC_MOTION_UNSUPPORTED;
    first=aim_state?0.0f:axes[0]*.075f;
    second=aim_state?0.0f:axes[1]*.075f;
    springs[0].target=first;springs[1].target=second;return UC_MOTION_OK;
}
