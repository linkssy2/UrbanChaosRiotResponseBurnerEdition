#include "bullet_impact.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
typedef UCBulletImpactRecipe Material;
/* Retail4FA510:31 rows, preserving duplicate particle requests and gaps. */
static const Material materials[]={
 {0u,0.0f,"FX\\FX_Decals\\FX_Decal_Default.shader",{NULL},0,0},
 {1u,100.0f,"FX\\FX_Decals\\FX_Decal_Concrete.shader",{"bullet_hit\\grey-dust.simpleparticlesystem","bullet_hit\\grey-dust2.simpleparticlesystem",NULL},0,0},
 {2u,0.0f,"FX\\FX_Decals\\FX_Decal_Metal.shader",{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {3u,0.0f,"FX\\FX_Decals\\FX_Decal_Wood.shader",{"bullet_hit\\debris-wood.simpleparticlesystem",NULL},0,0},
 {4u,100.0f,"FX\\FX_Decals\\FX_Decal_ConcreteSoft.shader",{"bullet_hit\\grey-dust.simpleparticlesystem","bullet_hit\\grey-dust2.simpleparticlesystem",NULL},0,0},
 {5u,100.0f,"FX\\FX_Decals\\FX_Decal_Brick.shader",{"bullet_hit\\brick-dust.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {6u,100.0f,"FX\\FX_Decals\\FX_Decal_Tile.shader",{"bullet_hit\\grey-dust.simpleparticlesystem","bullet_hit\\grey-dust2.simpleparticlesystem",NULL},0,0},
 {7u,300.0f,"FX\\FX_Decals\\FX_Decal_Metal.shader",{"bullet_hit\\smokepipe.simpleparticlesystem",NULL},0,1},
 {8u,0.0f,"FX\\FX_Decals\\FX_Decal_Glass.shader",{NULL},0,0},
 {9u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {11u,0.0f,"FX\\FX_Decals\\FX_Decal_Carpet.shader",{NULL},0,0},
 {12u,0.0f,"FX\\FX_Decals\\FX_Decal_Cloth.shader",{NULL},0,0},
 {13u,0.0f,NULL,{NULL},0,0},
 {14u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {15u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {16u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {17u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {18u,0.0f,NULL,{"bullet_hit\\blood.simpleparticlesystem","bullet_hit\\blood.simpleparticlesystem","bullet_hit\\bloodsplat.simpleparticlesystem",NULL},1,0},
 {19u,0.0f,"FX\\FX_Decals\\FX_Decal_Snow.shader",{"bullet_hit\\snow.simpleparticlesystem",NULL},0,0},
 {21u,0.0f,"FX\\FX_Decals\\FX_Decal_Snow.shader",{"bullet_hit\\snow.simpleparticlesystem",NULL},0,0},
 {22u,0.0f,"FX\\FX_Decals\\FX_Decal_ThinMetal.shader",{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {23u,0.0f,"FX\\FX_Decals\\FX_Decal_ThinMetal.shader",{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {25u,0.0f,"FX\\FX_Decals\\FX_Decal_ThinMetal.shader",{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {24u,0.0f,"FX\\FX_Decals\\FX_Decal_Leather.shader",{NULL},0,0},
 {26u,0.0f,"FX\\FX_Decals\\FX_Decal_Paper.shader",{NULL},0,0},
 {30u,0.0f,"FX\\FX_Decals\\FX_Decal_Wood.shader",{"bullet_hit\\debris-wood.simpleparticlesystem",NULL},0,0},
 {31u,0.0f,NULL,{NULL},0,0},
 {32u,0.0f,NULL,{NULL},0,0},
 {33u,0.0f,NULL,{NULL},0,0},
 {34u,0.0f,"FX\\FX_Decals\\FX_Decal_ThinMetal.shader",{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
 {35u,0.0f,NULL,{"bullet_hit\\sparks.simpleparticlesystem","bullet_hit\\sml-flame.simpleparticlesystem",NULL},0,0},
};
static const char *const alternate_blood[]={"bullet_hit\\multiplayer-blood.simpleparticlesystem","bullet_hit\\multiplayer-blood.simpleparticlesystem","bullet_hit\\multiplayer-bloodsplat.simpleparticlesystem",NULL};
const UCBulletImpactRecipe *uc_bullet_impact_recipe(uint32_t id){for(unsigned i=0;i<31;i++)if(materials[i].id==id)return materials+i;return NULL;}
static int rounding(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int vector(const float*p){if(!p)return 0;for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;}
static void cross(const float*a,const float*b,float*out){
 out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
}
int uc_bullet_impact_frame(const float*n,const float*p,float*out){
 float axis[3]={0},t[3],value[16]={0};unsigned k=0;
 if(!out||!vector(n)||!vector(p)||!rounding())return 1;
 if(fabsf(n[0])>fabsf(n[1]))k=1;if(fabsf(n[k])>fabsf(n[2]))k=2;axis[k]=1;
 cross(n,axis,t);double square=((double)t[2]*t[2]+(double)t[1]*t[1])+(double)t[0]*t[0];
 if(!(square>0))return 1;double inverse=1./sqrt(square);
 memcpy(value,n,12);for(unsigned i=0;i<3;i++)value[4+i]=(float)((double)t[i]*inverse);
 cross(value,value+4,value+8);memcpy(value+12,p,12);value[15]=1;
 for(unsigned i=0;i<16;i++)if(!isfinite(value[i]))return 1;
 memcpy(out,value,sizeof value);return 0;
}
static double sample(UCGameRandom*r,double low,double high){int32_t word;uc_game_random_next(r,&word);return (double)word*(high-low)*0x1p-31+low;}
int uc_bullet_static_impact(const UCBulletImpactServices*h,const UCEntityFreshView*v,
 UCBulletState*b,const UCBulletImpact*impact,UCGameRandom*r){
 float frame[16],size;const Material*m=NULL;uint8_t blocked,present;
 if(!h||!v||!b||!impact||!r||r->first>=250||r->second>=250||!rounding()
  ||b->definition_kind<1||b->definition_kind>4||!vector(b->motion.velocity))return 1;
 if(b->dead1F5)return 0;
 if(!h->blood_enabled||!h->session_mode||!h->preflight||!h->quota_test||!h->camera
  ||!h->quota_add||!h->particle||!h->decal||!h->sound
  ||uc_bullet_impact_frame(impact->normal,impact->point,frame)
  ||!h->preflight(h->context,v,b,impact))return 1;
 b->impact1F0=0;size=b->definition_kind==2?(float)sample(r,5.,6.):10.f;
 for(unsigned i=0;i<31;i++)if(materials[i].id==impact->material){m=materials+i;break;}
 if(m){
  const char *const*particles=m->particles;int emit=1;
  if(m->blood){if(!*h->blood_enabled)emit=0;else if(*h->session_mode)particles=alternate_blood;}
  if(emit&&m->radius!=0){
   if(h->quota_test(h->context,frame+12,&blocked))return 2;
   if(blocked)emit=0;
   else{
    float camera[16];if(h->camera(h->context,camera,&present))return 2;
    /* The source computes a temporary camera distance which is never read.
     * Preserve the completing getter, not an unobservable discarded RSQRT. */
    if(h->quota_add(h->context,frame+12,m->radius,m->id==7?4.f:.6f))return 2;
   }
  }
  if(emit){
   for(unsigned i=0;particles[i];i++)if(h->particle(h->context,particles[i],frame))return 2;
   if(m->steam&&h->sound(h->context,"Banks\\Impacts.amp","SteamPipe",m->id,frame+12))return 2;
  }
  if(size>0&&m->decal){
   /* Every admitted retail row has zero or ONE decal.2D37B0 still consumes
    * one shared random word for [0,1), before the current decal service. */
   int32_t ignored;if(uc_game_random_next(r,&ignored))return 2;
   if(h->decal(h->context,m->decal,frame+12,frame,size))return 2;
  }
  if(h->sound(h->context,"Banks\\Impacts.amp",NULL,m->id,frame+12))return 2;
 }
 if(!b->impact1F0)b->dead1F5=1;
 if(!vector(b->motion.velocity))return 2;
 const float*x=b->motion.velocity;
 double speed=sqrt(((double)x[2]*x[2]+(double)x[0]*x[0])+(double)x[1]*x[1]);
 if(speed<5.&&impact->normal[1]>0)b->dead1F5=1;
 return 0;
}
