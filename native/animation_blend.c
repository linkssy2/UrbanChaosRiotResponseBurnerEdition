#include "animation_blend.h"
#include <math.h>
#include <string.h>

static int finite_key(const UCAnimationKey *k) {
 if(!k)return 0;
 for(unsigned i=0;i<4;i++)if(!isfinite(k->rotation[i]))return 0;
 for(unsigned i=0;i<3;i++)if(!isfinite(k->translation[i]))return 0;
 return 1;
}
static int valid_blend(const UCAnimationBlend *b) {
 return b&&b->count<=b->capacity&&(!b->capacity||b->contributions);
}
UCAnimationStatus uc_animation_blend_math_load(const void *data,size_t size,UCAnimationBlendMath *out) {
 if(!data||!out||size!=sizeof(*out))return UC_ANIMATION_ARGUMENT;
 const unsigned char *p=(const unsigned char *)data;
 for(size_t i=0;i<4096;i++) {
  uint32_t bits=(uint32_t)p[i*4]|((uint32_t)p[i*4+1]<<8)|((uint32_t)p[i*4+2]<<16)|((uint32_t)p[i*4+3]<<24);
  float v;memcpy(&v,&bits,4);if(!isfinite(v)||v<0||v>2)return UC_ANIMATION_FORMAT;
 }
 for(size_t i=0;i<4096;i++) {
  uint32_t bits=(uint32_t)p[i*4]|((uint32_t)p[i*4+1]<<8)|((uint32_t)p[i*4+2]<<16)|((uint32_t)p[i*4+3]<<24);
  memcpy(out->coefficients+i,&bits,4);
 }
 return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_blend_init(UCAnimationBlend *b,const UCAnimationKey *prior,UCAnimationContribution *storage,size_t capacity) {
 if(!b||!finite_key(prior)||(capacity&&!storage))return UC_ANIMATION_ARGUMENT;
 UCAnimationKey copy=*prior;memset(b,0,sizeof(*b));b->pose=copy;b->contributions=storage;b->capacity=capacity;
 return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_blend_stage(UCAnimationBlend *b,float weight) {
 if(!valid_blend(b)||!isfinite(weight)||weight<0)return UC_ANIMATION_ARGUMENT;
 float limit=b->accumulated+weight;if(limit>=1)limit=1;
 b->limit=limit;b->scale=weight>0?(limit-b->accumulated)/weight:0;
 return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_blend_add(UCAnimationBlend *b,const UCAnimationKey *key,float weight,float factor) {
 if(!valid_blend(b)||!finite_key(key)||!isfinite(weight)||weight<0||!isfinite(factor)||factor<0)return UC_ANIMATION_ARGUMENT;
 float accepted=b->scale*weight,remaining=b->limit-b->accumulated;
 if(accepted>remaining)accepted=remaining;
 if(accepted==0)return UC_ANIMATION_OK;
 if(b->count==b->capacity)return UC_ANIMATION_LIMIT;
 b->contributions[b->count++]=(UCAnimationContribution){*key,accepted,factor};
 b->accumulated+=accepted;return UC_ANIMATION_OK;
}
static void scale_pose(UCAnimationKey *p,float factor) {
 if(factor==1)return;
 if(factor==0){memset(p,0,sizeof(*p));p->rotation[3]=1;return;}
 p->rotation[3]*=factor;
 for(unsigned i=0;i<3;i++)p->translation[i]*=factor;
}
static int interpolate(const UCAnimationBlendMath *m,const float from[4],const float to[4],float t,float out[4],unsigned *path) {
 float dot=from[0]*to[0]+from[3]*to[3];dot+=from[2]*to[2];dot+=from[1]*to[1];
 if(!isfinite(dot)||!isfinite(t)||t<0||t>1)return 0;
 if(path&&dot<0)*path|=UC_BLEND_NEGATIVE;
 float end[4];for(unsigned i=0;i<4;i++)end[i]=dot<0?0.0f-to[i]:to[i];
 if(dot<0)dot=0.0f-dot;
 float a=1.0f-t,b=t;
 if(1.0f-dot>0.01f) {
  if(path)*path|=UC_BLEND_TABLE;
  float x=dot*63.0f,y=t*63.0f;if(x>63)x=63;
  unsigned row=(unsigned)x,column=(unsigned)y;
  float dx=x-(float)row,dy=y-(float)column,nx=1.0f-dx,ny=1.0f-dy;
  /* The endpoint reads the next row's first element at column64, just as
   * the source lookup does. The near-angle branch excludes the last row. */
  if(row>=63||column>63)return 0;
  const float *table=m->coefficients;size_t at=row*64+column;
  float high=dy==0?table[at+64]:table[at+64]*ny+table[at+65]*dy;
  float low=table[at+1]*dy+table[at]*ny;
  b=high*dx+low*nx;
  at=row*64+63-column;
  high=table[at+64]*ny+table[at+63]*dy;
  low=dy==0?table[at]:table[at]*ny+table[at-1]*dy;
  a=high*dx+low*nx;
 }
 float q[4];for(unsigned i=0;i<4;i++)q[i]=from[i]*a+end[i]*b;
 double norm=(double)q[0]*q[0]+(double)q[1]*q[1];norm+=(double)q[2]*q[2];norm+=(double)q[3]*q[3];
 if(!(norm>0)||!isfinite(norm))return 0;
 float inverse=(float)(1.0/sqrt(norm));
 for(unsigned i=0;i<4;i++){out[i]=inverse*q[i];if(!isfinite(out[i]))return 0;}
 return 1;
}
UCAnimationStatus uc_animation_blend_finish_path(UCAnimationBlend *b,const UCAnimationBlendMath *m,UCAnimationBlendPath *summary) {
 if(!valid_blend(b)||!m)return UC_ANIMATION_ARGUMENT;
 UCAnimationBlendPath path={0};
 UCAnimationKey pose=b->pose;float cumulative=1.0f-b->accumulated;
 for(size_t i=b->count;i>0;i--) {
  const UCAnimationContribution *n=b->contributions+i-1;
  cumulative+=n->weight;UCAnimationKey key=n->key;scale_pose(&key,n->factor);
  path.last_interpolation=n->weight!=cumulative;path.flags|=path.last_interpolation?UC_BLEND_INTERPOLATE:UC_BLEND_COPY;
  if(n->weight==cumulative)pose=key;
  else {
   float t=n->weight/cumulative;
   if(!interpolate(m,pose.rotation,key.rotation,t,pose.rotation,&path.flags))return UC_ANIMATION_UNSUPPORTED;
   float inv=1.0f-t;for(unsigned j=0;j<3;j++)pose.translation[j]=inv*pose.translation[j]+key.translation[j]*t;
  }
  if(!finite_key(&pose))return UC_ANIMATION_UNSUPPORTED;
 }
 b->pose=pose;b->count=0;b->accumulated=0;if(summary)*summary=path;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_blend_finish(UCAnimationBlend *b,const UCAnimationBlendMath *m) {
 return uc_animation_blend_finish_path(b,m,NULL);
}
UCAnimationStatus uc_animation_blend_rotation(const UCAnimationBlendMath*m,const float*a,const float*b,float t,float*out){
 if(!m||!a||!b||!out)return UC_ANIMATION_ARGUMENT;
 for(unsigned i=0;i<4;i++)if(!isfinite(a[i])||!isfinite(b[i]))return UC_ANIMATION_ARGUMENT;
 float q[4];if(!interpolate(m,a,b,t,q,NULL))return UC_ANIMATION_UNSUPPORTED;
 memcpy(out,q,sizeof q);return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_blend_matrix(const UCAnimationKey *key,float out[16]) {
 if(!finite_key(key)||!out)return UC_ANIMATION_ARGUMENT;
 const float *q=key->rotation;float x=q[0]*2,y=q[1]*2,z=q[2]*2;
 float xx=q[0]*x,yy=q[1]*y,zz=q[2]*z,xy=q[1]*x,xz=q[2]*x,yz=q[2]*y;
 float wx=q[3]*x,wy=q[3]*y,wz=q[3]*z;
 float m[16]={1-(zz+yy),wz+xy,xz-wy,0,xy-wz,1-(zz+xx),yz+wx,0,
              wy+xz,yz-wx,1-(yy+xx),0,key->translation[0],key->translation[1],key->translation[2],1};
 for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return UC_ANIMATION_UNSUPPORTED;
 memcpy(out,m,sizeof(m));return UC_ANIMATION_OK;
}
