#include "glass_bullet.h"
#include "point_light_receiver.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
static int bounded(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;}
static int local_segment(const float*world,const UCBulletSegment*s,float local[2][3]){
    float inverse[16];if(!world||!s||fegetround()!=FE_TONEAREST||!bounded(s->from,3)||!bounded(s->to,3)||!uc_point_light_inverse(world,inverse))return 0;
    for(unsigned n=0;n<2;n++){const float*p=n?s->to:s->from;for(unsigned j=0;j<3;j++)
        local[n][j]=(inverse[j]*p[0]+inverse[4+j]*p[1])+(inverse[8+j]*p[2]+inverse[12+j]);}
    return bounded(local[0],6);
}
int uc_glass_bullet_query(const float*world,const float quad[4][2],const UCBulletSegment*s,UCGlassBulletHit*out,uint8_t*accepted){
    float p[2][3];if(!quad||!out||!accepted||!bounded(quad[0],8)||!local_segment(world,s,p))return 1;
    *accepted=0;float d[3];for(unsigned j=0;j<3;j++)d[j]=p[1][j]-p[0][j];
    float denominator=(d[1]*0.f+d[0]*0.f)+d[2];if(denominator<.01f&&denominator>-.01f)return 0;
    float numerator=((0.f-p[0][1])*0.f+(0.f-p[0][0])*0.f)+(0.f-p[0][2]);
    float t=numerator/denominator;out->fraction=t;if(t<=0.f||t>=1.f)return 0;
    for(unsigned j=0;j<2;j++)out->point[j]=p[0][j]+d[j]*t;
    for(unsigned j=0;j<4;j++){unsigned k=(j+1)&3;
        float cross=(out->point[1]-quad[j][1])*(quad[k][0]-quad[j][0])-(quad[k][1]-quad[j][1])*(out->point[0]-quad[j][0]);
        if(cross>0)return 0;
    }
    float inverse=(float)(1.0/sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]));
    float direction=((d[1]*inverse)*0.f+(d[0]*inverse)*0.f)+d[2]*inverse;
    out->normal[0]=out->normal[1]=0.f;out->normal[2]=direction<0?1.f:-1.f;*accepted=1;return 0;
}
int uc_glass_bullet_receive(const UCGlassBullet*v,const UCBulletSegment*s){
    if(!v||!v->triggered)return 1;if(*v->triggered)return 0;
    UCGlassBulletHit hit;uint8_t accepted;
    if(uc_glass_bullet_query(v->world,v->quad,s,&hit,&accepted))return 1;if(!accepted)return 0;
    float local[2][3],impact[6],quad[4][2];
    if(!local_segment(v->world,s,local)||!isfinite(s->delta)||s->delta==0||!isfinite(s->mass))return 1;
    float inverse=1.f/s->delta;impact[0]=hit.point[0];impact[1]=hit.point[1];
    for(unsigned j=0;j<3;j++){impact[2+j]=(local[1][j]-local[0][j])*inverse;if(!isfinite(impact[2+j]))return 1;}impact[5]=s->mass;
    memcpy(quad,v->quad,sizeof quad);const UCGlassBulletSink*h=v->sink?*v->sink:NULL;
    if(!h||!h->preflight||!h->submit||!h->preflight(h->owner,quad,impact))return 1;
    if(h->submit(h->owner,quad,impact))return 2;*v->triggered=1;return 0;
}
