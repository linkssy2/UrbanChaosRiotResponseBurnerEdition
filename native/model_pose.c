#include "model_pose.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int finite_matrix(const float m[16]) {
    for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 0;return 1;
}
static int valid_model(const UCModel *m) { return m&&m->bones&&m->bone_count&&m->bone_count<=4096; }
static void multiply(const float a[16],const float b[16],float out[16]) {
    float result[16];
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++) {
        float p0=b[c]*a[r*4],p1=b[4+c]*a[r*4+1];
        float p2=b[8+c]*a[r*4+2],p3=b[12+c]*a[r*4+3];
        result[r*4+c]=(p0+p1)+(p2+p3);
    }
    memcpy(out,result,sizeof(result));
}
typedef UCModelPoseVisit Visit;
static UCModelPoseStatus linked_world(const UCModel*m,const float(*local)[16],const float base[16],float(*world)[16],
    int partial,uint8_t*valid,Visit*work,uint8_t*seen,int visited_only,uint32_t*visited_count){
    Visit *stack=work+m->bone_count;uint32_t pending=1,count=0;
    stack[0]=(Visit){0,UINT32_MAX};
    while(pending) {
        Visit v=stack[--pending];
        if(v.bone>=m->bone_count||seen[v.bone])return UC_POSE_HIERARCHY;
        if(visited_only&&!finite_matrix(local[v.bone]))return UC_POSE_ARGUMENT;
        seen[v.bone]=1;work[count++]=v;const UCModelBone *b=&m->bones[v.bone];
        if(b->first_child){if(pending==m->bone_count)return UC_POSE_HIERARCHY;stack[pending++]=(Visit){b->first_child,v.bone};}
        if(b->next_sibling){if(pending==m->bone_count)return UC_POSE_HIERARCHY;stack[pending++]=(Visit){b->next_sibling,v.parent};}
    }
    if(count!=m->bone_count&&!partial)return UC_POSE_INCOMPLETE;
    for(uint32_t i=0;i<count;i++) {
        Visit v=work[i];multiply(local?local[v.bone]:m->bones[v.bone].initial_local,
            v.parent==UINT32_MAX?base:world[v.parent],world[v.bone]);
    }
    if(valid)memcpy(valid,seen,m->bone_count);
    if(visited_count)*visited_count=count;
    return UC_POSE_OK;
}
static UCModelPoseStatus pose_world(const UCModel *m,const float (*local)[16],const float root[16],float (*world)[16],int retained,uint8_t *valid) {
    if(!valid_model(m)||!root||!world||!finite_matrix(root))return UC_POSE_ARGUMENT;
    for(uint32_t i=0;i<m->bone_count;i++)if(!finite_matrix(local?local[i]:m->bones[i].initial_local)||
        (retained&&!finite_matrix(world[i])))return UC_POSE_ARGUMENT;
    float base[16];memcpy(base,root,sizeof(base));
    if(m->source_header[73]) {
        /* B8B60 always processes bone0 first and reads only parent thereafter. */
        for(uint32_t i=1;i<m->bone_count;i++)if(m->bones[i].parent>=i)return UC_POSE_HIERARCHY;
        for(uint32_t i=0;i<m->bone_count;i++)multiply(local?local[i]:m->bones[i].initial_local,
            i?world[m->bones[i].parent]:base,world[i]);
        if(valid)memset(valid,1,m->bone_count);
        return UC_POSE_OK;
    }
    /* Validate linked traversal before publishing any output. B8E40 processes
     * next sibling with the same parent before descending to its own child. */
    /* ponytail: precompute this validated order once if integrated in a hot
     * animation loop; the isolated correctness boundary does not need a cache. */
    Visit *work=(Visit*)malloc(m->bone_count*2*sizeof(*work));
    unsigned char *seen=(unsigned char*)calloc(m->bone_count,1);
    if(!work||!seen){free(work);free(seen);return UC_POSE_MEMORY;}
    UCModelPoseStatus status=linked_world(m,local,base,world,retained||valid,valid,work,seen,0,NULL);
    free(work);free(seen);return status;
}
UCModelPoseStatus uc_model_pose_world_linked_borrowed(const UCModel*m,const float(*local)[16],const float root[16],
    float(*world)[16],UCModelPoseVisit*work,uint8_t*seen,uint32_t capacity,uint32_t*count){
    if(!valid_model(m)||m->source_header[73]||!local||!root||!world||!work||!seen||!count||
        capacity<m->bone_count||!finite_matrix(root))return UC_POSE_ARGUMENT;
    float base[16];memcpy(base,root,sizeof base);memset(seen,0,m->bone_count);
    return linked_world(m,local,base,world,1,NULL,work,seen,1,count);
}
UCModelPoseStatus uc_model_pose_world(const UCModel *m,const float (*local)[16],const float root[16],float (*world)[16]) {
    return pose_world(m,local,root,world,0,NULL);
}
UCModelPoseStatus uc_model_pose_world_retained(const UCModel *m,const float (*local)[16],const float root[16],float (*world)[16]) {
    return pose_world(m,local,root,world,1,NULL);
}
UCModelPoseStatus uc_model_pose_world_partial(const UCModel *m,const float (*local)[16],const float root[16],float (*world)[16],uint8_t *valid) {
    if(!valid)return UC_POSE_ARGUMENT;
    return pose_world(m,local,root,world,0,valid);
}
static UCModelPoseStatus palette(const UCModel *m,const float (*world)[16],const uint8_t *selected,float (*rows)[12]) {
    if(!valid_model(m)||!world||!rows||!isfinite(m->position_scale))return UC_POSE_ARGUMENT;
    for(uint32_t i=0;i<m->bone_count;i++){
        if(selected&&selected[i]>1)return UC_POSE_ARGUMENT;
        if((!selected||selected[i])&&(!finite_matrix(world[i])||!finite_matrix(m->bones[i].bind_correction)))return UC_POSE_ARGUMENT;
    }
    float s=m->position_scale,scale[16]={s,0,0,0,0,s,0,0,0,0,s,0,0,0,0,1};
    for(uint32_t i=0;i<m->bone_count;i++) {
        if(selected&&!selected[i])continue;
        float corrected[16],posed[16];multiply(scale,m->bones[i].bind_correction,corrected);
        multiply(corrected,world[i],posed);
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)rows[i][r*4+c]=posed[c*4+r];
    }
    return UC_POSE_OK;
}
UCModelPoseStatus uc_model_pose_palette(const UCModel *m,const float (*world)[16],float (*rows)[12]) {
    return palette(m,world,NULL,rows);
}
UCModelPoseStatus uc_model_pose_palette_selected(const UCModel *m,const float (*world)[16],const uint8_t *selected,float (*rows)[12]) {
    if(!selected)return UC_POSE_ARGUMENT;
    return palette(m,world,selected,rows);
}

int uc_model_pose_equipment(const UCModelEquipment*s,float(*local)[16],uint32_t count){
    if(!s||!local||!count||count>4096)return 1;
    for(unsigned i=0;i<9;i++)if(s->bones[i]<-1||(s->bones[i]>=0&&(uint32_t)s->bones[i]>=count))return 1;
    for(unsigned i=0;i<9;i++)if(s->bones[i]!=-1&&!s->enabled[i])
        for(unsigned j=0;j<12;j++)local[s->bones[i]][j]=0.00001f;
    return 0;
}
