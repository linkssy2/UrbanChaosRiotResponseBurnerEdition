#include "footprint.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned count){for(unsigned i=0;i<count;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_footprint_quad(const UCPlayerFootprint *p,float corners[4][3]){
    if(!p||!corners||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||
       !finite_values(p->point,3)||!finite_values(p->normal,3)||!finite_values(p->tangent,3)||!finite_values(p->size,2))return 1;
    float a[3],b[3],result[4][3];
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3,k=(i+2)%3;a[i]=p->tangent[j]*p->normal[k]-p->tangent[k]*p->normal[j];}
    double x=a[0],y=a[1],z=a[2],length=sqrt((y*y+x*x)+z*z);
    if(!(length>0)||!isfinite(length))return 1;
    float reciprocal=(float)(1.0/length);for(unsigned i=0;i<3;i++)a[i]=a[i]*reciprocal;
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3,k=(i+2)%3;b[i]=a[j]*p->normal[k]-a[k]*p->normal[j];}
    for(unsigned i=0;i<3;i++){
        const float w=(b[i]*p->size[0])*.5f,h=(a[i]*p->size[1])*.5f,center=p->point[i]+p->normal[i];
        result[0][i]=center+(h+w);result[1][i]=center+(w-h);
        result[2][i]=center+((0.f-w)-h);result[3][i]=center+((0.f-w)+h);
    }
    if(!finite_values(&result[0][0],12))return 1;memcpy(corners,result,sizeof result);return 0;
}
int uc_footprint_submit(const UCPlayerFootprint *p,float fade,const UCFootprintServices *h,uint8_t *submitted){
    if(!p||!h||!submitted||!h->preflight||!h->query||!h->manager||!h->material_cache||!h->publish)return 1;
    if(!p->shader){*submitted=0;return 0;}
    float corners[4][3],starts[4][3],ends[4][3];
    if(!isfinite(fade)||uc_footprint_quad(p,corners))return 1;
    if(!(p->flags&4))for(unsigned i=0;i<4;i++)for(unsigned j=0;j<3;j++){
        const float delta=p->normal[j]*2.f;starts[i][j]=delta+corners[i][j];ends[i][j]=corners[i][j]-delta;
    }
    if(!(p->flags&4)&&(!finite_values(&starts[0][0],12)||!finite_values(&ends[0][0],12)))return 1;
    if(!h->preflight(h->context,p,fade))return 1;
    *submitted=0;
    if(!(p->flags&4))for(unsigned i=0;i<4;i++)if(!h->query(h->context,starts[i],ends[i],1,65535,p->mask,0.f))return 0;
    void *manager=h->manager(h->context);if(!manager)return 0;
    void *cache=h->material_cache(h->context);if(!cache)return 1;
    h->publish(h->context,manager,cache,p->shader,corners,p->flags&3,fade);*submitted=1;return 0;
}
