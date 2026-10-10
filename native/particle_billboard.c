#include "particle_billboard.h"
#include <math.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_particle_color_pack(const float rgba[4],uint32_t *argb){
    if(!rgba||!argb||!arithmetic())return 0;
    uint32_t c[4];
    for(unsigned k=0;k<4;k++){
        if(!isfinite(rgba[k])||fabsf(rgba[k])>65536)return 0;
        int n=_mm_cvtss_si32(_mm_set_ss(rgba[k]*255.0f));c[k]=(uint32_t)(n<0?0:n>255?255:n);
    }
    *argb=(c[3]<<24)|(c[0]<<16)|(c[1]<<8)|c[2];return 1;
}
int uc_particle_billboard_clip(const UCParticleBillboard *b,unsigned corner,
    const UCParticleBillboardView *v,UCParticleBillboardVertex *out){
    if(!b||!v||!out||corner>3||!arithmetic()||!finite_values(b->center,6)||
       !finite_values(&v->clip_rows[0][0],30))return 0;
    static const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    static const float side[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    float x=side[corner][0]*b->radius,y=side[corner][1]*b->radius,z=0.0f*b->radius;
    float rx=(x*b->cosine+y*b->sine)+z*0.0f;
    float ry=(x*-b->sine+y*b->cosine)+z*0.0f;
    float position[4];UCParticleBillboardVertex r={0};
    for(unsigned k=0;k<4;k++){
        float horizontal=v->right[k]*rx;
        position[k]=(v->up[k]*ry+horizontal)+(k==3?1.f:b->center[k]);
    }
    if(!finite_values(position,4))return 0;
    for(unsigned k=0;k<4;k++)r.clip[k]=((position[0]*v->clip_rows[k][0]+position[1]*v->clip_rows[k][1])+position[2]*v->clip_rows[k][2])+position[3]*v->clip_rows[k][3];
    if(!finite_values(r.clip,4))return 0;
    r.color[0]=(float)((b->color>>16)&255)/255.f;r.color[1]=(float)((b->color>>8)&255)/255.f;
    r.color[2]=(float)(b->color&255)/255.f;r.color[3]=(float)(b->color>>24)/255.f;
    r.uv[0]=uv[corner][0];r.uv[1]=uv[corner][1];r.uv[2]=0;r.uv[3]=1;r.fog=r.clip[3];
    *out=r;return 1;
}
int uc_particle_billboard_vertex(const UCParticleBillboard *b,unsigned corner,
    const UCParticleBillboardView *v,UCParticleBillboardVertex *out){
    UCParticleBillboardVertex r;
    if(!out||!uc_particle_billboard_clip(b,corner,v,&r)||r.clip[3]==0)return 0;
    float reciprocal=1.f/r.clip[3],magnitude=fabsf(reciprocal);
    if(magnitude<0x1p-64f)magnitude=0x1p-64f;if(magnitude>0x1p64f)magnitude=0x1p64f;
    reciprocal=copysignf(magnitude,reciprocal);
    for(unsigned k=0;k<3;k++)r.screen[k]=(r.clip[k]*v->viewport_scale[k])*reciprocal+v->viewport_offset[k];
    if(!finite_values(r.screen,3))return 0;*out=r;return 1;
}
