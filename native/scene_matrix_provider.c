#include "scene_matrix_provider.h"
#include "point_light_receiver.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_matrix(const float*p){if(!p)return 0;for(unsigned i=0;i<16;i++)if(!isfinite(p[i]))return 0;return 1;}
static void product(const float*a,const float*b,float*out){
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)out[i*4+j]=(b[j]*a[i*4]+b[4+j]*a[i*4+1])+(b[8+j]*a[i*4+2]+b[12+j]*a[i*4+3]);
}
static void transpose(const float*in,float*out){for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)out[i*4+j]=in[j*4+i];}
int uc_scene_matrix_initialize(UCSceneMatrixProvider*p){if(!p)return 1;memset(p->dirty,0,4);return 0;}
int uc_scene_matrix_set(UCSceneMatrixProvider*p,uint32_t slot,const float in[16]){
    if(!p||slot>4||!finite_matrix(in))return 1;
    float *dst=slot==0?p->camera:slot==1?p->view:slot==2?p->material:slot==3?p->projection:p->alternate;
    memcpy(dst,in,64);if(slot<4)p->dirty[slot]=1;return 0;
}
int uc_scene_matrix_flush(UCSceneMatrixProvider*p,const UCSceneMatrixServices*s){
    if(!p||!s||!s->context||!s->upload||(_mm_getcsr()&0xE040u))return 1;
    int dirty=p->dirty[0]||p->dirty[1]||p->dirty[2]||p->dirty[3];
    float composed[16],intermediate[16],inverse[16],local_camera[4],rows[16];
    if(dirty){
        if(!finite_matrix(p->material)||!finite_matrix(p->view)||!finite_matrix(p->projection))return 1;
        product(p->material,p->view,intermediate);product(intermediate,p->projection,composed);if(!finite_matrix(composed))return 1;
        if(p->dirty[0]||p->dirty[2]){
            if(!finite_matrix(p->camera)||!uc_point_light_inverse(p->material,inverse))return 1;
            for(unsigned i=0;i<4;i++){local_camera[i]=(inverse[i]*p->camera[12]+inverse[4+i]*p->camera[13])+(inverse[8+i]*p->camera[14]+inverse[12+i]*p->camera[15]);if(!isfinite(local_camera[i]))return 1;}
        }
    }
    s->context(s->owner);
    if(dirty){
        memcpy(p->composite,composed,64);
        if(p->dirty[2]){transpose(p->material,rows);s->upload(s->owner,96,rows,16);}
        if(p->dirty[0])s->upload(s->owner,118,p->camera+12,4);
        if(p->dirty[1]){transpose(p->view,rows);s->upload(s->owner,100,rows,16);}
        if(p->dirty[0]||p->dirty[2])s->upload(s->owner,117,local_camera,4);
        transpose(p->composite,rows);s->upload(s->owner,104,rows,16);
    }
    memset(p->dirty,0,4);return 0;
}
