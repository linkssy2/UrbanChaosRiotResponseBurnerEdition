#include "muzzle_placement.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_matrix(const float *m){for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 0;return 1;}
static void multiply(const float *a,const float *b,float *out){
 for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){
  float p0=a[r*4]*b[c],p1=a[r*4+1]*b[4+c],p2=a[r*4+2]*b[8+c],p3=a[r*4+3]*b[12+c];
  out[r*4+c]=(p0+p1)+(p2+p3);
 }
}
static int bind(const UCModel*m,int32_t*out,const uint8_t name[16]){
 if(!m||!out||!m->bones||!m->bone_count||m->bone_count>4096)return 0;
 int32_t found=-1;
 for(uint32_t i=0;i<m->bone_count;i++){
  const uint8_t*p=m->bones[i].source_metadata+4;unsigned j;
  for(j=0;j<16;j++){unsigned a=p[j];if(a>='A'&&a<='Z')a+=32;if(a!=name[j])break;if(!a)break;}
  if(j==16||(!p[j]&&!name[j])){found=(int32_t)i;break;}
 }
 *out=found;return 1;
}
int uc_muzzle_bind(const UCModel*m,int32_t*out){static const uint8_t name[16]="dummyflare";return bind(m,out,name);}
int uc_flashlight_bind(const UCModel*m,int32_t*out){static const uint8_t name[16]="dummylightflare";return bind(m,out,name);}
int uc_muzzle_world(int32_t index,uint32_t count,const float (*pose)[16],const uint8_t*valid,const float*weapon,float*out){
 static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 if((_mm_getcsr()&0xE040)||!weapon||!out||!finite_matrix(weapon)||index< -1||
    (index>=0&&(!pose||!valid||(uint32_t)index>=count||valid[index]!=1||!finite_matrix(pose[index]))))return 0;
 static const float axes[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};
 float local[16],world[16];
 if(index<0)memcpy(local,identity,sizeof local);else multiply(axes,pose[index],local);
 multiply(local,weapon,world);if(!finite_matrix(world))return 0;memcpy(out,world,sizeof world);return 1;
}
