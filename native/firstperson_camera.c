#include "firstperson_camera.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int finite_values(const float *p,unsigned n) {
    for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;
}
static float inverse_length(float a,float b,float c) {
    /* Original x87 precision-control53 path, then one float store. */
    double sum=(double)a*a+(double)b*b;sum+=(double)c*c;
    return (float)(1.0/sqrt(sum));
}
static void multiply(const float a[16],const float b[16],float out[16]) {
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++) {
        float p0=b[c]*a[r*4],p1=b[4+c]*a[r*4+1];
        float p2=b[8+c]*a[r*4+2],p3=b[12+c]*a[r*4+3];
        out[r*4+c]=(p0+p1)+(p2+p3);
    }
}
UCFirstpersonCameraStatus uc_firstperson_camera_initialize(const float player_world[16],
    float initial_fov,UCFirstpersonCamera *state,UCFirstpersonCameraInput *input) {
    if((_mm_getcsr()&0xE040)!=0)return UC_CAMERA_UNSUPPORTED;
    if(!player_world||!state||!input||!finite_values(player_world,16)||
       !isfinite(initial_fov))return UC_CAMERA_ARGUMENT;
    UCFirstpersonCamera next={0};
    memcpy(next.world,player_world,sizeof(next.world));next.fov=initial_fov;
    for(unsigned j=0;j<3;j++) {
        float z=player_world[8+j]*0.0f+player_world[12+j];
        float x=player_world[j]*0.0f,y=player_world[4+j]*170.0f;
        next.world[12+j]=(x+y)+z;
    }
    next.world[15]=1.0f;
    if(!finite_values(next.world,16))return UC_CAMERA_ARGUMENT;
    *state=next;
    input->to_offset[0]=input->to_offset[1]=0.0f;input->to_offset[2]=10000.0f;
    memset(input->extra,0,sizeof(input->extra));
    for(unsigned j=0;j<4;j++)input->extra[j*5]=1.0f;
    input->from_damper=input->to_damper=input->fov_damper=1.0f;
    input->target_fov=initial_fov;
    return UC_CAMERA_OK;
}
UCFirstpersonCameraStatus uc_firstperson_camera_follow(UCFirstpersonCamera *state,
    const UCFirstpersonCameraInput *in,float render_world[16]) {
    if((_mm_getcsr()&0xE040)!=0)return UC_CAMERA_UNSUPPORTED;
    if(!state||!in||!render_world||!finite_values(state->world,16)||
       !finite_values(state->target,3)||!isfinite(state->fov)||
       !finite_values(in->player_frame,16)||!finite_values(in->to_offset,3)||
       !finite_values(in->extra,16)||!isfinite(in->from_damper)||
       !isfinite(in->to_damper)||!isfinite(in->fov_damper)||
       !isfinite(in->target_fov)||!isfinite(in->effect_amplitude))return UC_CAMERA_ARGUMENT;
    if(in->debug_flags||in->attachment_blocked||(!in->getter_bypass&&in->effect_amplitude>0))return UC_CAMERA_UNSUPPORTED;
    UCFirstpersonCamera next=*state;
    if(next.fov!=in->target_fov)next.fov+=(in->target_fov-next.fov)*in->fov_damper;
    float forward[3],right[3],up[3],matrix[16];
    for(unsigned j=0;j<3;j++) {
        float z=in->player_frame[8+j]*in->to_offset[2]+in->player_frame[12+j];
        float x=in->player_frame[j]*in->to_offset[0],y=in->player_frame[4+j]*in->to_offset[1];
        float desired=(x+y)+z;
        next.target[j]+=(desired-next.target[j])*in->to_damper;
        float backwards=next.world[12+j]-next.target[j];
        forward[j]=0.0f-backwards;
    }
    float scale=inverse_length(forward[2],forward[1],forward[0]);
    for(unsigned j=0;j<3;j++)forward[j]=scale*forward[j];
    float dot=(forward[2]*0.0f+forward[0]*0.0f)+forward[1];
    if(dot< -1.0f)dot=-1.0f;else if(dot>1.0f)dot=1.0f;
    /* Preserve the original near-up branch; it has no alternate-up policy.
     * Its zero cross product is refused below instead of publishing NaNs. */
    float axis[3];memcpy(axis,forward,sizeof(axis));
    if(acos((double)dot)<(double)0.01f){axis[0]=0;axis[1]=1;axis[2]=0;}
    float zero_product=axis[1]*0.0f;
    right[0]=axis[2]-zero_product;
    right[1]=axis[0]*0.0f-axis[2]*0.0f;
    right[2]=zero_product-axis[0];
    scale=inverse_length(right[2],right[1],right[0]);
    for(unsigned j=0;j<3;j++)right[j]*=scale;
    up[0]=right[2]*axis[1]-right[1]*axis[2];
    up[1]=right[0]*axis[2]-right[2]*axis[0];
    up[2]=right[1]*axis[0]-right[0]*axis[1];
    scale=inverse_length(up[1],up[0],up[2]);
    for(unsigned j=0;j<3;j++) {
        next.world[j]=right[j];next.world[4+j]=scale*up[j];next.world[8+j]=forward[j];
        next.world[12+j]+=(in->player_frame[12+j]-next.world[12+j])*in->from_damper;
    }
    next.world[3]=next.world[7]=next.world[11]=0;next.world[15]=1;
    multiply(in->extra,next.world,matrix);
    if(!finite_values(matrix,16)||!finite_values(next.target,3)||!isfinite(next.fov))return UC_CAMERA_SINGULAR;
    memcpy(next.world,matrix,sizeof(matrix));*state=next;memcpy(render_world,matrix,sizeof(matrix));return UC_CAMERA_OK;
}
