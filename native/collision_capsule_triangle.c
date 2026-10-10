#include "collision_capsule_triangle.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
#include <fenv.h>
typedef __m128 V;
static V load(const float*p){return _mm_loadu_ps(p);}
static V add(V a,V b){return _mm_add_ps(a,b);}
static V sub(V a,V b){return _mm_sub_ps(a,b);}
static V mul(V a,V b){return _mm_mul_ps(a,b);}
static V scale(V a,float b){return mul(a,_mm_set1_ps(b));}
static V neg(V a){return _mm_xor_ps(a,_mm_set1_ps(-0.0f));}
static float dot(V a,V b){V p=mul(a,b);return _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p,p,0xaa),_mm_add_ss(_mm_shuffle_ps(p,p,0x55),p)));}
static V cross(V a,V b){return sub(mul(_mm_shuffle_ps(a,a,0xc9),_mm_shuffle_ps(b,b,0xd2)),mul(_mm_shuffle_ps(a,a,0xd2),_mm_shuffle_ps(b,b,0xc9)));}
static float inverse_length(float s){float r=1.0f/sqrtf(s);return (.5f*r)*(3.0f-(s*r)*r);}
static V normalize(V a){return scale(a,inverse_length(dot(a,a)));}
static int finite_values(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int bounded(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536.f)return 0;return 1;}
static int vectors(const float p[][4],unsigned n){for(unsigned i=0;i<n;i++)if(!bounded(p[i],4))return 0;return 1;}
static int cache_domain(const UCCapsuleTriangleCache*c){for(unsigned i=0;i<3;i++)if(!(c->inverse_edge[i]>0&&c->inverse_edge[i]<=65536.f))return 0;return c->inverse_area>0&&c->inverse_area<=65536.f&&c->area>0&&isfinite(c->area);}
static int mode(void){return !(_mm_getcsr()&0xe040u)&&fegetround()==FE_TONEAREST;}
static int written_finite(const void*a,uint32_t mask){uint32_t x[24];memcpy(x,a,sizeof x);for(unsigned i=0;i<24;i++)if((mask&(1u<<i))&&(x[i]&0x7f800000u)==0x7f800000u)return 0;return 1;}
int uc_capsule_triangle_cache(const float t[3][4],UCCapsuleTriangleCache*out){
 if(!t||!out||!mode()||!vectors(t,3))return 0;
 V e[3]={sub(load(t[2]),load(t[1])),sub(load(t[0]),load(t[2])),sub(load(t[1]),load(t[0]))};UCCapsuleTriangleCache r;
 for(unsigned i=0;i<3;i++)r.inverse_edge[i]=inverse_length(dot(e[i],e[i]));
 V n=cross(e[0],e[1]);r.area=sqrtf(dot(n,n));r.inverse_area=(float)(1.0/(double)r.area);
 if(!cache_domain(&r))return 0;*out=r;return 1;
}
/* 32F010's stored-float cuts are intentional: numerator's first bound uses
 * its unrounded double, then its later division reloads the stored float. */
static UCCapsuleSegmentClosest closest(V a,V da,V b,V db){
 V rel=sub(b,a);float ab=dot(da,db),ap=dot(da,rel),bp=dot(db,rel),aa=dot(da,da),bb=dot(db,db);
 float product=(float)((double)bb*aa),square=(float)((double)ab*ab),den=(float)fabs((double)product-square);
 double numer=(double)bb*ap-(double)bp*ab;float saved=(float)numer;double s;uint32_t flags;
 if((double)den*den<=numer*den){s=1;flags=1;}
 else if(saved<=0){s=0;flags=2;}
 else if(((double)square+product)*0x1p-20f>=den){s=1;flags=1;}
 else{s=(double)saved/den;flags=0;}
 double projected=(double)ab*s-bp;float t=(float)projected;
 if(projected>=bb){t=1;flags=4;}
 else if(t<=0){t=0;flags=8;}
 else {t=(float)((double)t/bb);goto done;}
 numer=(double)ab*t+ap;saved=(float)numer;
 if(numer<=0){s=0;flags|=2;}
 else if(saved>=aa){s=1;flags|=1;}
 else s=(double)saved/aa;
 done:;
 UCCapsuleSegmentClosest r;V p=add(a,scale(da,(float)s)),d=sub(p,add(b,scale(db,t)));
 _mm_storeu_ps(r.point,p);_mm_storeu_ps(r.delta,d);r.distance_squared=dot(d,d);r.features=flags;return r;
}
int uc_capsule_segment_closest(const float*a,const float*da,const float*b,const float*db,UCCapsuleSegmentClosest*out){
 if(!a||!da||!b||!db||!out||!mode()||!bounded(a,4)||!bounded(da,4)||!bounded(b,4)||!bounded(db,4))return 0;
 UCCapsuleSegmentClosest r=closest(load(a),load(da),load(b),load(db));if(!finite_values(r.point,4)||!finite_values(r.delta,4)||!isfinite(r.distance_squared))return 0;*out=r;return 1;
}
static void contact(UCCapsuleTriangleContact*out,V point,V normal,double distance,float rb,double total){
 _mm_storeu_ps(out->normal_distance,normal);
 _mm_storeu_ps(out->position,add(point,scale(normal,(float)((double)rb-distance))));
 out->normal_distance[3]=(float)(distance-total);
}
/* 32FA30 retains three independently selected contact slots. Its first slot
 * is also refreshed on the distance-rejected branch; do not turn this into a
 * generic closest-contact reducer. */
static uint32_t capsules(const float aa[2][4],float ra,const float bb[2][4],float rb,UCCapsuleTriangleContact out[3]){
 uint32_t written=0xf0;
 V a=load(aa[0]),other=load(aa[1]),b=load(bb[0]);V da=sub(other,a),db=sub(load(bb[1]),b);
 float ab=dot(da,db);if(ab<0){V swap=a;a=other;other=swap;da=neg(da);ab=-ab;}
 V rel=sub(b,a);float ap=dot(da,rel),bp=dot(db,rel),asq=dot(da,da),bsq=dot(db,db);
 float product=(float)((double)bsq*asq),square=(float)((double)ab*ab),den=(float)fabs((double)product-square);
 double numer=(double)bsq*ap-(double)bp*ab;float saved=(float)numer;double s;unsigned flags;
 if((double)den*den<=numer*den){s=1;flags=1;}
 else if(saved<=0){s=0;flags=2;}
 else if((fabs((double)product)+square)*0x1p-20f>=den){s=1;flags=1;}
 else{s=(double)saved/den;flags=0;}
 double projected=(double)ab*s-bp;float t=(float)projected;
 if(projected>=bsq){t=1;flags=4;}
 else if(t<=0){t=0;flags=8;}
 else{t=(float)((double)t/bsq);goto pair;}
 numer=(double)t*ab+ap;saved=(float)numer;
 if(numer<=0){s=0;flags|=2;}
 else if(saved>=asq){s=1;flags|=1;}
 else s=(double)saved/asq;
 pair:;
 V p=add(a,scale(da,(float)s)),q=add(b,scale(db,t)),delta=sub(p,q);float dsq=dot(delta,delta);
 double total=(double)ra+rb;float total_stored=(float)total;
 double threshold=total+out[0].normal_distance[3];float inv=(float)(1.0/sqrt((double)dsq));
 if(dsq>=threshold*threshold){_mm_storeu_ps(out[0].normal_distance,scale(delta,inv));out[0].normal_distance[3]=(float)((double)dsq*inv-total);return written;}
 V n;double distance;
 if(dsq<0x1p-23f){
  float v[4],o[4];_mm_storeu_ps(v,da);unsigned largest=0,first=1,second=2;
  if(fabsf(v[1])<fabsf(v[0])){largest=1;first=0;}
  if(fabsf(v[2])<fabsf(v[largest])){second=largest;largest=2;}
  o[largest]=0;o[first]=v[second];o[second]=-v[first];o[3]=0;n=normalize(load(o));distance=0;
 }else{n=scale(delta,inv);distance=(double)dsq*inv;}
 contact(&out[0],p,n,distance,rb,total);written|=0xff;
 if((double)product*.2f<den)return written;
 double inv_b=(1.0/(double)product)*asq;float inv_a=(float)((double)bsq*(1.0/(double)product));
 double x=ap,y=-(double)bp;V pa=a,pb=b;
 for(unsigned k=0;k<2;k++){
  if(!(flags&(k?5:10))){
   V ca=pa,cb=pb;int admit=1,move_a=0,move_b=0;
   if(y>0){
    if(y>bsq)admit=0;
    else if(x>0){if(x>asq)admit=0;else if(x*x*inv_a>=y*y*inv_b)move_b=1;else move_a=1;}
    else move_b=1;
   }else if(x>0){if(x>asq)admit=0;else move_a=1;}
   if(admit){
    if(move_a)ca=add(ca,scale(da,(float)((double)inv_a*x)));
    if(move_b)cb=add(cb,scale(db,(float)(y*inv_b)));
    V diff=sub(ca,cb);float squared=dot(diff,diff);double bound=(double)total_stored+out[k+1].normal_distance[3];
    if(squared<bound*bound){V normal=squared>0x1p-23f?normalize(diff):load(out[0].normal_distance);contact(&out[k+1],ca,normal,sqrt((double)squared),rb,total_stored);written|=0xffu<<(8*(k+1));}
   }
  }
  pa=other;pb=load(bb[1]);da=neg(da);db=neg(db);y=(double)bsq-y-ab;x=(double)asq-x-ab; }
 return written;
}
int uc_capsule_triangle_contacts(const float ends[2][4],float ra,const float triangle[3][4],float rb,const UCCapsuleTriangleCache*cache,float limit,int manifold,UCCapsuleTriangleContact output[3]){
 if(!ends||!triangle||!cache||!output||!mode()||!vectors(ends,2)||!vectors(triangle,3)||!cache_domain(cache)||!isfinite(ra)||!isfinite(rb)||!isfinite(limit)||ra<0||rb<0||ra>65536.f||rb>65536.f||limit<0)return 0;
 uint32_t written=(1u<<7)|(1u<<15)|(1u<<23);
 UCCapsuleTriangleContact result[3];memcpy(result,output,sizeof result);
 V origin=load(triangle[0]),e[3]={sub(load(triangle[2]),load(triangle[1])),sub(origin,load(triangle[2])),sub(load(triangle[1]),origin)};
 V normal=scale(cross(e[0],e[1]),cache->inverse_area),planes[3];
 for(unsigned i=0;i<3;i++)planes[i]=cross(scale(e[i],cache->inverse_edge[i]),normal);
 float matrix[4][4];for(unsigned i=0;i<3;i++)_mm_storeu_ps(matrix[i],planes[i]);_mm_storeu_ps(matrix[3],normal);
 V columns[3];for(unsigned axis=0;axis<3;axis++)columns[axis]=_mm_set_ps(matrix[3][axis],matrix[2][axis],matrix[1][axis],matrix[0][axis]);
 float offset=(float)(-(double)cache->area*cache->inverse_edge[0]);V offsets=_mm_set_ps(0,0,0,offset),d[2];float distances[2][4];unsigned signs[2];
 for(unsigned i=0;i<2;i++){
  V rel=sub(load(ends[i]),origin);float r[4];_mm_storeu_ps(r,rel);
  d[i]=add(add(scale(columns[0],r[0]),scale(columns[1],r[1])),add(scale(columns[2],r[2]),offsets));
  _mm_storeu_ps(distances[i],d[i]);signs[i]=(unsigned)_mm_movemask_ps(d[i]);result[i].normal_distance[3]=FLT_MAX;
 }
 result[2].normal_distance[3]=FLT_MAX;
 double total=(double)ra+rb;float stored_total=(float)total;double expanded=total+limit;float bound=(float)expanded;
 if(_mm_movemask_ps(_mm_cmplt_ps(_mm_set1_ps(bound),d[0]))&_mm_movemask_ps(_mm_cmplt_ps(_mm_set1_ps(bound),d[1])))goto finish;
 if(distances[0][3]<-expanded&&distances[1][3]<-expanded)goto finish;
 if((signs[0]^signs[1])&8){
  float t=(float)((double)distances[0][3]/((double)distances[0][3]-distances[1][3]));V intersection=add(scale(d[0],1.f-t),scale(d[1],t));
  if((_mm_movemask_ps(intersection)&7)==7){
   result[1].normal_distance[3]=-FLT_MAX;
   for(unsigned i=0;i<2;i++)if((signs[i]&7)==7){
    double penetration=-fabs((double)distances[i][3])-stored_total;
    if(penetration>result[1].normal_distance[3]){
     float shift=(float)((signs[i]&8?(double)rb:-(double)rb)-distances[i][3]);
     _mm_storeu_ps(result[1].normal_distance,signs[i]&8?normal:neg(normal));
     _mm_storeu_ps(result[1].position,add(load(ends[i]),scale(normal,shift)));result[1].normal_distance[3]=(float)penetration;written|=0xff00;
    }
   }
   float delta[4];_mm_storeu_ps(delta,sub(d[1],d[0]));double dw2=(double)delta[3]*delta[3];
   for(unsigned i=0;i<3;i++){
    double reciprocal=1.0/((double)delta[i]*delta[i]+dw2);
    double numerator=(double)distances[0][3]*delta[i]-(double)delta[3]*distances[0][i];
    float squared=(float)(numerator*numerator*reciprocal);double threshold=(double)stored_total+result[1].normal_distance[3];
    if(squared>=threshold*threshold)continue;
    float at=(float)(-(((double)delta[i]*distances[0][i]+(double)delta[3]*distances[0][3])*reciprocal));
    if(at<=-.0001f||at>=1.000100016593933f)continue;
    float dx=delta[i],dw=delta[3];if(dw<0){dw=-dw;dx=-dx;}
    V n=normalize(add(scale(planes[i],dw),scale(normal,-dx)));double penetration=-sqrt((double)squared);
    V point=add(scale(load(ends[0]),1.f-at),scale(load(ends[1]),at));contact(&result[1],point,n,penetration,rb,stored_total);written|=0xff00;
   }
   goto finish;
  }
 }
 unsigned united=signs[0]|signs[1];int shared=(united&7)!=7;
 static const unsigned first[3]={1,2,0},second[3]={2,0,1};
 if(shared&&manifold){
  for(unsigned i=0;i<3;i++)result[i].normal_distance[3]=limit;
  for(unsigned i=0;i<3;i++)if(!(united&(1u<<i))){float edge[2][4];memcpy(edge[0],triangle[first[i]],16);memcpy(edge[1],triangle[second[i]],16);written|=capsules(ends,ra,edge,rb,result);}
  goto finish;
 }
 V direction=sub(load(ends[1]),load(ends[0]));
 for(unsigned end=0;end<(shared?1u:2u);end++){
  if(!shared&&(signs[end]&7)==7){
   float shift=(float)((signs[end]&8?-(double)rb:(double)rb)-distances[end][3]);
   _mm_storeu_ps(result[end].position,add(load(ends[end]),scale(normal,shift)));
   _mm_storeu_ps(result[end].normal_distance,signs[end]&8?neg(normal):normal);
   result[end].normal_distance[3]=(float)((signs[end]&8?-(double)distances[end][3]:(double)distances[end][3])-stored_total);written|=0xffu<<(8*end);continue;
  }
  unsigned selection=end;
  for(unsigned i=0;i<3;i++){
   if((shared?(signs[0]&signs[1]):signs[end])&(1u<<i))continue;
   V edge_start=load(triangle[first[i]]),edge_dir=sub(load(triangle[second[i]]),edge_start);
   UCCapsuleSegmentClosest c=closest(load(ends[0]),direction,edge_start,edge_dir);
   double threshold=(double)stored_total+result[end].normal_distance[3];if(c.distance_squared>=threshold*threshold)continue;
   if(c.features&(1u<<selection)){if(!shared)continue;selection=1;}
   V n,delta_reference=load(c.delta);
   if(!c.features)n=cross(direction,edge_dir);
   else if(c.distance_squared<=0x1p-46f){
    /* 330CC2 overwrites the closest-delta scratch with this edge plane
     * before both normalization and the subsequent signed-distance dot. */
    delta_reference=planes[i];n=cross(direction,edge_dir);if(dot(n,n)<=0x1p-46f)n=planes[i];
   }else n=delta_reference;
   n=normalize(n);float distance=dot(n,delta_reference);double positive=distance;if(distance<0){positive=-positive;n=neg(n);}
   contact(&result[end],load(c.point),n,positive,rb,stored_total);written|=0xffu<<(8*end);
  }
 }
 finish:
 if(!written_finite(result,written))return 0;memcpy(output,result,sizeof result);return 1;
}
