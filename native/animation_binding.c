#include "animation_binding.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

static float constructor_scale(const float v[4]) {
 __m128 q=_mm_loadu_ps(v);q=_mm_mul_ps(q,q);
 q=_mm_add_ps(q,_mm_movehl_ps(q,q));
 q=_mm_add_ss(q,_mm_shuffle_ps(q,q,1));
 __m128 y=_mm_rsqrt_ss(q),t=_mm_mul_ss(_mm_mul_ss(y,q),y);
 t=_mm_sub_ss(_mm_set_ss(3),t);
 return _mm_cvtss_f32(_mm_mul_ss(q,_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),y),t)));
}
static int initial_key(const float m[16],float scale,UCAnimationKey *key,int enabled) {
 for(unsigned k=0;k<16;k++)if(!isfinite(m[k]))return 0;
 float trace=m[5]+m[0];trace+=m[10];
 if(trace>=0) {
  float s=(float)-sqrt((double)trace+1.0),f=.5f/s;
  key->rotation[0]=(m[6]-m[9])*f;key->rotation[1]=(m[8]-m[2])*f;
  key->rotation[2]=(m[1]-m[4])*f;key->rotation[3]=(float)((double)s*.5);
 } else {
  unsigned i=m[5]>m[0]?1:0;if(m[10]>m[i*5])i=2;
  unsigned j=(i+1)%3,k=(j+1)%3;float q[4];
  double s=(double)m[k*5]+m[j*5];s=(double)m[i*5]-s;s=sqrt(s+1.0);
  float rounded=(float)s,f=rounded!=0?.5f/rounded:rounded;q[i]=(float)(s*.5);
  q[3]=(m[j*4+k]-m[k*4+j])*f;
  q[j]=(m[j*4+i]+m[i*4+j])*f;q[k]=(m[k*4+i]+m[i*4+k])*f;
  for(unsigned n=0;n<4;n++)key->rotation[n]=0.0f-q[n];
 }
 float reciprocal=1.0f/scale;
 for(unsigned i=0;i<3;i++)key->translation[i]=m[12+i]*reciprocal;
 for(unsigned i=0;i<4;i++)if(!isfinite(key->rotation[i]))return 0;
 if(enabled||isfinite(scale)){
  for(unsigned i=0;i<3;i++)if(!isfinite(key->translation[i]))return 0;
 }
 return 1;
}
UCAnimationStatus uc_animation_binding_movement(const UCAnimationBinding *b,float delta,float out[16]) {
 if(!b||!out||!b->local||!b->skeleton.bone_count||b->skeleton.bone_count>4096||
    !isfinite(delta)||delta<0||delta>1||b->skeleton.root_bone< -1||
    b->skeleton.root_bone>=(int32_t)b->skeleton.bone_count)return UC_ANIMATION_ARGUMENT;
 float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 if(b->skeleton.root_bone!=-1){
  UCAnimationKey key;
  if(!initial_key(b->local[b->skeleton.root_bone],1,&key,1))return UC_ANIMATION_UNSUPPORTED;
  key.rotation[3]*=delta;
  for(unsigned i=0;i<3;i++)key.translation[i]*=delta;
  if(uc_animation_blend_matrix(&key,m)!=UC_ANIMATION_OK)return UC_ANIMATION_UNSUPPORTED;
  __m128 rows[3];
  for(unsigned i=0;i<2;i++){
   __m128 row=_mm_loadu_ps(m+i*4),q=_mm_mul_ps(row,row);
   q=_mm_add_ps(q,_mm_movehl_ps(q,q));q=_mm_add_ss(q,_mm_shuffle_ps(q,q,1));
   __m128 y=_mm_rsqrt_ss(q),t=_mm_mul_ss(_mm_mul_ss(y,q),y);
   t=_mm_sub_ss(_mm_set_ss(3),t);y=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),y),t);
   rows[i]=_mm_mul_ps(row,_mm_shuffle_ps(y,y,0));
  }
  for(unsigned i=2;i>0;i--){
   __m128 a=rows[i==2?0:2],c=rows[i==2?1:0];
   rows[i]=_mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a,a,0xc9),_mm_shuffle_ps(c,c,0xd2)),
                       _mm_mul_ps(_mm_shuffle_ps(a,a,0xd2),_mm_shuffle_ps(c,c,0xc9)));
  }
  for(unsigned i=0;i<3;i++){_mm_storeu_ps(m+i*4,rows[i]);m[i*4+3]=0;}
  for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return UC_ANIMATION_UNSUPPORTED;
 }
 memcpy(out,m,sizeof m);return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_binding_key_from_matrix(const float*m,UCAnimationKey*out){
 if(!m||!out)return UC_ANIMATION_ARGUMENT;
 UCAnimationKey key;if(!initial_key(m,1,&key,1))return UC_ANIMATION_UNSUPPORTED;
 *out=key;return UC_ANIMATION_OK;
}
void uc_animation_binding_free(UCAnimationBinding *b) {
 if(!b)return;
 free((void*)b->skeleton.enabled);free((void*)b->skeleton.translation_scale);free((void*)b->skeleton.translation_offset);
 free(b->poses);free(b->local);free(b->work);free(b->contributions);free(b->pose_scratch);free(b->matrix_scratch);
 memset(b,0,sizeof(*b));
}
UCAnimationStatus uc_animation_binding_init(const UCModel *m,const UCAnimationTemplate *t,UCAnimationBinding *out) {
 if(!m||!t||!out||!m->bones||!m->bone_count||m->bone_count>4096||m->bone_count!=t->bone_count||
  (t->source_default!=-1&&(t->source_default<0||(uint32_t)t->source_default>=t->bone_count)))return UC_ANIMATION_ARGUMENT;
 uint32_t n=m->bone_count;
 UCAnimationBinding b={0};UCAnimationStatus status=UC_ANIMATION_MEMORY;
 uint8_t *enabled=(uint8_t*)calloc(n,1);float *scale=(float*)calloc(n,4);float (*offset)[3]=(float(*)[3])calloc(n,12);
 b.skeleton=(UCAnimationSkeleton){n,t->source_default,m->source_header[72],enabled,scale,offset};
 b.poses=(UCAnimationKey*)calloc(n,sizeof(*b.poses));b.local=(float(*)[16])calloc(n,64);
 b.work=(UCAnimationBlend*)calloc(n,sizeof(*b.work));b.contributions=(UCAnimationContribution*)calloc(n,2*sizeof(*b.contributions));
 b.contributions_per_bone=2;
 b.pose_scratch=(UCAnimationKey*)calloc(n,sizeof(*b.pose_scratch));b.matrix_scratch=(float(*)[16])calloc(n,64);
 if(!enabled||!scale||!offset||!b.poses||!b.local||!b.work||!b.contributions||!b.pose_scratch||!b.matrix_scratch)goto fail;
 status=UC_ANIMATION_UNSUPPORTED;
 for(uint32_t i=0;i<n;i++) {
  const UCModelBone *bone=m->bones+i;enabled[i]=bone->source_metadata[4]!=0;memcpy(b.local[i],bone->initial_local,64);scale[i]=1;
  if(bone->parent!=i) {
   float v[4];memcpy(v,bone->source_metadata+80,16);
   for(unsigned k=0;k<4;k++)if(!isfinite(v[k]))goto fail;
   memcpy(offset[i],v,12);scale[i]=constructor_scale(v);
   if(!isfinite(scale[i])&&(enabled[i]||v[0]!=0||v[1]!=0||v[2]!=0||v[3]!=0))goto fail;
  }
  if(scale[i]==0)goto fail;
  float matrix[16];
  if(i==0){memset(matrix,0,64);matrix[0]=matrix[6]=b.skeleton.mirrored?-1.0f:1.0f;matrix[9]=matrix[15]=1;}
  else {memcpy(matrix,bone->initial_local,64);for(unsigned k=0;k<3;k++)matrix[12+k]-=offset[i][k];}
  if(!initial_key(matrix,scale[i],b.poses+i,enabled[i]))goto fail;
 }
 if(t->source_default!=-1&&!enabled[t->source_default]){
  float *matrix=b.local[t->source_default];memset(matrix,0,64);matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;
 }
 *out=b;return UC_ANIMATION_OK;
fail:uc_animation_binding_free(&b);return status;
}
UCAnimationStatus uc_animation_binding_sample(UCAnimationBinding *b,UCAnimationSample *sample,
 const UCAnimationBlendMath *math,float phase,float weight,float factor) {
 UCAnimationBindingSample entry={sample,phase,weight,factor};
 return uc_animation_binding_sample_many(b,&entry,1,math);
}
static UCAnimationStatus sample_many(UCAnimationBinding *b,
 const UCAnimationBindingSample *entries,size_t count,const UCAnimationBlendMath *math,int graph) {
 if(!b||!math||!b->poses||!b->work||!b->contributions||(count&&!entries))return UC_ANIMATION_ARGUMENT;
 if(count>32)return UC_ANIMATION_LIMIT;
 uint32_t n=b->skeleton.bone_count;
 if(!n||n>4096)return UC_ANIMATION_ARGUMENT;
 for(size_t j=0;j<count;j++) {
  const UCAnimationBindingSample *e=entries+j;UCAnimationSample *s=e->sample;uint8_t used[4096]={0};
  if(!s||!s->clip.source_tracks||!s->clip.track_count||s->clip.track_count>4096||
   !isfinite(e->phase)||e->phase<0||e->phase>=1||!isfinite(e->weight)||e->weight<0||e->weight>1||
   !isfinite(e->root_factor)||e->root_factor<0)return UC_ANIMATION_ARGUMENT;
  for(size_t k=0;k<j;k++)if(entries[k].sample==s)return UC_ANIMATION_ARGUMENT;
  for(uint32_t i=0;i<s->clip.track_count;i++) {
   int32_t id=s->clip.source_tracks[i];uint32_t bone=(uint32_t)(id<0?~id:id);
   if(bone>=n)return UC_ANIMATION_FORMAT;
   if(used[bone])return UC_ANIMATION_UNSUPPORTED;used[bone]=1;
  }
 }
 if(count*2>b->contributions_per_bone) {
  UCAnimationContribution *p=(UCAnimationContribution*)realloc(b->contributions,n*count*2*sizeof(*p));
  if(!p)return UC_ANIMATION_MEMORY;
  b->contributions=p;b->contributions_per_bone=count*2;
 }
 for(uint32_t i=0;i<n;i++) {
  /* Disabled priors can contain original constructor NaNs. An empty
   * accumulator retains their bytes; no disabled contribution is admitted. */
  UCAnimationStatus st=UC_ANIMATION_OK;
  UCAnimationContribution *storage=b->contributions+i*b->contributions_per_bone;
  if(b->skeleton.enabled[i])st=uc_animation_blend_init(b->work+i,b->poses+i,storage,b->contributions_per_bone);
  else b->work[i]=(UCAnimationBlend){b->poses[i],0,0,0,storage,0,b->contributions_per_bone};
  if(st)return st;
  if(graph){st=uc_animation_blend_stage(b->work+i,1);if(st)return st;}
 }
 for(size_t j=0;j<count;j++) {
  UCAnimationSample *sample=entries[j].sample;float weight=entries[j].weight,factor=entries[j].root_factor;
  if(weight==0)continue;
  UCAnimationStatus st=uc_animation_sample_phase(sample,entries[j].phase);if(st)return st;
  if(!graph)for(uint32_t i=0;i<n;i++){st=uc_animation_blend_stage(b->work+i,weight);if(st)return st;}
  float lower=(1.0f-sample->fraction)*weight,upper=sample->fraction*weight;
  for(uint32_t i=0;i<sample->clip.track_count;i++) {
   int32_t id=sample->clip.source_tracks[i];uint32_t bone=(uint32_t)(id<0?~id:id);
   if(!b->skeleton.enabled[bone])continue;
   float f=(int32_t)i==sample->clip.root_track?factor:1.0f;
   st=uc_animation_blend_add(b->work+bone,sample->lower+i,lower,f);if(st)return st;
   if(sample->fraction!=0){st=uc_animation_blend_add(b->work+bone,sample->upper+i,upper,f);if(st)return st;}
  }
 }
 UCAnimationStatus st=uc_animation_skeleton_finish(&b->skeleton,math,b->work,b->pose_scratch);if(st)return st;
 st=uc_animation_skeleton_locals(&b->skeleton,b->work,b->local,b->matrix_scratch);if(st)return st;
 for(uint32_t i=0;i<n;i++)b->poses[i]=b->work[i].pose;
 return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_binding_sample_many(UCAnimationBinding *b,
 const UCAnimationBindingSample *entries,size_t count,const UCAnimationBlendMath *math){
 return sample_many(b,entries,count,math,0);
}
UCAnimationStatus uc_animation_binding_sample_graph(UCAnimationBinding *b,
 const UCAnimationBindingSample *entries,size_t count,const UCAnimationBlendMath *math){
 return sample_many(b,entries,count,math,1);
}
