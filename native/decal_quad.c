#include "decal_quad.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int bounded(const float *p,unsigned n,float limit){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>limit)return 0;return 1;}
int uc_decal_quad(const float point[3],const float normal[3],const float size[2],UCVectorRandom *random,float corners[4][3]){
    if(!random||!corners||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||
        !bounded(point,3,1e6f)||!bounded(normal,3,16)||!bounded(size,2,1e4f))return 1;
    unsigned axis=0;if(fabsf(normal[0])>fabsf(normal[1]))axis=1;if(fabsf(normal[axis])>fabsf(normal[2]))axis=2;
    float unit[3]={0},a[3],b[3];unit[axis]=1;
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3,k=(i+2)%3;a[i]=unit[k]*normal[j]-unit[j]*normal[k];}
    double x=a[0],y=a[1],z=a[2],length=sqrt((x*x+z*z)+y*y);
    if(!(length>0)||!isfinite(length))return 1;
    float reciprocal=(float)(1.0/length);if(!isfinite(reciprocal))return 1;
    for(unsigned i=0;i<3;i++)a[i]=a[i]*reciprocal;
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3,k=(i+2)%3;b[i]=a[k]*normal[j]-a[j]*normal[k];}
    float angles[4];if(uc_vector_random_signed(random,3.1415927410125732421875f,angles))return 1;
    float sn=(float)sin((double)angles[0]),cs=(float)cos((double)angles[0]),one=1.f-cs;
    float nx=normal[0],ny=normal[1],nz=normal[2],xy=(ny*nx)*one,xz=(nz*nx)*one,yz=(nz*ny)*one;
    float sx=nx*sn,sy=ny*sn,sz=nz*sn;
    const float m[9]={(nx*nx)*one+cs,sz+xy,xz-sy,xy-sz,(ny*ny)*one+cs,sx+yz,sy+xz,yz-sx,(nz*nz)*one+cs};
    float height[3],width[3];
    for(unsigned i=0;i<3;i++){
        width[i]=(m[i]*a[0]+m[3+i]*a[1])+(m[6+i]*a[2]+0.f);
        height[i]=(m[i]*b[0]+m[3+i]*b[1])+(m[6+i]*b[2]+0.f);
    }
    for(unsigned i=0;i<3;i++){
        float w=(width[i]*size[0])*.5f,h=(height[i]*size[1])*.5f,center=point[i]+normal[i];
        corners[0][i]=center+(h+w);corners[1][i]=center+(w-h);
        corners[2][i]=center+((0.f-w)-h);corners[3][i]=center+((0.f-w)+h);
    }
    return 0;
}
