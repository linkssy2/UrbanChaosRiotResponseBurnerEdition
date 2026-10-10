#include "rigid_prop.h"
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
int uc_rigid_prop_parameters(const UCLevelEntity*d,const uint8_t**cursor,UCRigidPropParameters*out){
    if(!d||!cursor||!out)return 1;
    static const uint8_t types[]={2,2,2,2,2,4,4,6,3,1,2,2,3,3,4,3,2,3,1,2,9,9,2};
    static const size_t offsets[]={offsetof(UCRigidPropParameters,mass),offsetof(UCRigidPropParameters,linear_damping),offsetof(UCRigidPropParameters,angular_damping),offsetof(UCRigidPropParameters,friction),offsetof(UCRigidPropParameters,restitution),offsetof(UCRigidPropParameters,velocity),offsetof(UCRigidPropParameters,angular_velocity),offsetof(UCRigidPropParameters,sound),offsetof(UCRigidPropParameters,unmovable),offsetof(UCRigidPropParameters,max_bullet_hits),offsetof(UCRigidPropParameters,max_life),offsetof(UCRigidPropParameters,explode_delay),offsetof(UCRigidPropParameters,hurts_on_collision),offsetof(UCRigidPropParameters,explodable),offsetof(UCRigidPropParameters,explosion_offset),offsetof(UCRigidPropParameters,explode_on_contact),offsetof(UCRigidPropParameters,contact_speed),offsetof(UCRigidPropParameters,influenced_by_explosions),offsetof(UCRigidPropParameters,explode_contact),offsetof(UCRigidPropParameters,respawn_time),offsetof(UCRigidPropParameters,point_particle),offsetof(UCRigidPropParameters,shot_particle),offsetof(UCRigidPropParameters,temperature)};
    const uint8_t*p=*cursor;UCRigidPropParameters v={0};
    for(unsigned i=0;i<sizeof types;i++){
        void*field=(uint8_t*)&v+offsets[i];int found;
        if(uc_entity_stream_get(d,&p,types[i],field,&found)||!found)return 1;
        if(types[i]==3)*(uint32_t*)field=(*(uint32_t*)field&255)!=0;
    }
    *out=v;*cursor=p;return 0;
}
static void product(const float*a,const float*b,float*out){
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)
        out[r*4+c]=(b[c]*a[r*4]+b[4+c]*a[r*4+1])+(b[8+c]*a[r*4+2]+b[12+c]*a[r*4+3]);
}
int uc_rigid_prop_box(const UCModelCollisionShape*in,UCModelCollisionShape*out){
    if(!in||!out||in==out||in->family!=2||in->kind!=1||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(in->transform[i])||fabsf(in->transform[i])>1e6f)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(in->dimensions[i])||in->dimensions[i]<=0||in->dimensions[i]>1e4f)return 1;
    if(in->transform[3]!=0||in->transform[7]!=0||in->transform[11]!=0||in->transform[15]!=1||!isfinite(in->radius)||in->radius<0)return 1;
    UCModelCollisionShape v=*in;float a[16],b[16],c[16];
    const float axes[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};
    const float zero_euler[16]={1,-0.f,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    product(in->transform,axes,a);a[12]=in->transform[12]*100.f;a[13]=in->transform[14]*100.f;a[14]=in->transform[13]*100.f;a[15]=in->transform[15];
    product(a,zero_euler,b);product(b,axes,c);c[12]=b[12]*.01f;c[13]=b[14]*.01f;c[14]=b[13]*.01f;c[15]=b[15];
    memcpy(v.transform,c,sizeof c);v.dimensions[1]=in->dimensions[2];v.dimensions[2]=in->dimensions[1];
    float minimum=v.dimensions[0]<v.dimensions[1]?v.dimensions[0]:v.dimensions[1];v.dimensions[3]=minimum>v.dimensions[2]?v.dimensions[2]:minimum;
    /*302D70 tails the same double-backed2E2810 quaternion extraction.*/
    double trace=((double)c[5]+c[0])+c[10];
    if(trace>0){double s=sqrt(trace+1),k=.5/s;v.rotation[0]=(float)(((double)c[6]-c[9])*k);v.rotation[1]=(float)(((double)c[8]-c[2])*k);v.rotation[2]=(float)(((double)c[1]-c[4])*k);v.rotation[3]=(float)(s*.5);}
    else{unsigned i=c[5]>c[0]?1u:0u;if(c[10]>c[i*5])i=2;unsigned j=(i+1)%3,k=(j+1)%3;double s=sqrt(((double)c[i*5]-((double)c[k*5]+c[j*5]))+1),w=.5/s;v.rotation[i]=(float)(s*.5);v.rotation[3]=(float)(((double)c[j*4+k]-c[k*4+j])*w);v.rotation[j]=(float)(((double)c[j*4+i]+c[i*4+j])*w);v.rotation[k]=(float)(((double)c[k*4+i]+c[i*4+k])*w);}
    for(unsigned i=0;i<4;i++)if(!isfinite(v.rotation[i]))return 1;
    *out=v;return 0;
}
