#include "molotov_receiver.h"
#include <math.h>
#include <string.h>
static void cross(const float*a,const float*b,float*out){
 out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
const char*uc_molotov_particle_asset(unsigned i){
 static const char*names[]={"MolotovBurstFlame1.simpleparticlesystem","MolotovBurstFlame2.simpleparticlesystem","MolotovBurstFlame3.simpleparticlesystem","MolotovBurstFlame4.simpleparticlesystem","MolotovBurstFlame5.simpleparticlesystem","generic_fx\\MolotovExp1.simpleparticlesystem","generic_fx\\MolotovExp2.simpleparticlesystem","generic_fx\\MolotovExp3.simpleparticlesystem"};
 return i<8?names[i]:NULL;
}
int uc_molotov_receive(float world[16],int32_t id,int32_t sender,
 const UCNpcMolotovExplosion*p,const UCMolotovReceiverServices*h){
 float matrix[16],frame[16]={0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1};unsigned found=0;
 if(!world||!p||!h||!h->sender)return 1;
 if(h->sender(h->owner,sender,matrix,&found))return 2;
 if(!found)return 0;
 for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))return 1;
 for(unsigned i=0;i<3;i++)if(!isfinite(p->normal[i])||!isfinite(p->direction[i])||!isfinite(p->point[i]))return 1;
 memcpy(world,matrix,64);
 float n=p->normal[0]*p->normal[0]+p->normal[1]*p->normal[1];n+=p->normal[2]*p->normal[2];
 int unit=fabs(sqrt((double)n)-1.)<(double).01f;
 if(unit){memcpy(frame,p->normal,12);cross(p->direction,p->normal,frame+4);cross(p->normal,frame+4,frame+8);}
 memcpy(frame+12,world+12,16);
 if(!h->particles)return 2;
 if(p->hit){for(unsigned i=5;i<8;i++)if(h->particles(h->owner,uc_molotov_particle_asset(i),frame))return 2;}
 else{
  if(h->particles(h->owner,uc_molotov_particle_asset(0),frame))return 2;
  if(unit&&!p->airborne){
   for(unsigned i=1;i<5;i++)if(h->particles(h->owner,uc_molotov_particle_asset(i),frame))return 2;
   if(!h->sound||h->sound(h->owner,id,world,"Banks\\Objects\\MolotovFire.amp#FireStart","Banks\\Objects\\MolotovFire.amp#FireStop",2.f))return 2;
   if(!h->damage_sphere||h->damage_sphere(h->owner,id,world,120.f,p->owner,2.f,p->group1,p->group2))return 2;
   if(p->surface&&(!h->scorch||h->scorch(h->owner,"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader",p->point,p->normal,128.f,128.f,0x4000)))return 2;
  }
 }
 UCMolotovBlast b={0};memcpy(b.point,world+12,12);b.radius=200.f;b.owner=p->owner;b.flag1=1;b.kind=4;b.team=p->team;b.projectile=sender;
 return !h->broadcast||h->broadcast(h->owner,9,&b,id)?2:0;
}
