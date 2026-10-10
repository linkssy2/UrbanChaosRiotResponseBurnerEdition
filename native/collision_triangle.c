#include "collision_triangle.h"
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n) {
    if(!p)return 0;
    for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;
    return 1;
}
static __m128 vector3(const float *v) {return _mm_set_ps(0,v[2],v[1],v[0]);}
static __m128 cross(__m128 a,__m128 b) {
    return _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(a,a,0xC9),_mm_shuffle_ps(b,b,0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(a,a,0xD2),_mm_shuffle_ps(b,b,0xC9)));
}
static float dot(__m128 a,__m128 b) {
    __m128 p=_mm_mul_ps(a,b);
    return _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(p,p,0xAA),
        _mm_add_ss(_mm_shuffle_ps(p,p,0x55),p)));
}
static int triangle_cast(const float v[3][3],const float from[3],
    const float to[3],float limit,UCCollisionTriangleHit *out,int control) {
    __m128 a,b,c,n,s,e,p,r;float plane,ds,de,f,squared,tolerance,edge;
    double end_distance;UCCollisionTriangleHit next;
    if(!v||!out||!finite_values(&v[0][0],9)||!finite_values(from,3)
       ||!finite_values(to,3)||!isfinite(limit)||limit<0||limit>1)return -1;
    if((_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return -1;
    a=vector3(v[0]);b=vector3(v[1]);c=vector3(v[2]);
    s=vector3(from);e=vector3(to);n=cross(_mm_sub_ps(b,a),_mm_sub_ps(c,a));
    plane=dot(a,n);ds=(float)((double)dot(n,s)-plane);
    end_distance=(double)dot(n,e)-plane;de=(float)end_distance;
    if(!isfinite(plane)||!isfinite(ds)||!isfinite(de))return -1;
    /* The end distance remains extended across the original non-popping FST
     * for this product, but the subsequent fraction uses its stored float. */
    if(!(end_distance*ds<0)&&(ds!=0||de==0))return 0;
    f=(float)((double)ds/((double)ds-de));
    if(!isfinite(f))return -1;
    if(!(f<limit))return 0;
    squared=dot(n,n);tolerance=(float)((double)squared*-0.00009999999747378752f);
    if(!isfinite(squared)||!isfinite(tolerance))return -1;
    p=_mm_add_ps(_mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1),_mm_set1_ps(f)),s),
                 _mm_mul_ps(_mm_set1_ps(f),e));
    a=_mm_sub_ps(a,p);b=_mm_sub_ps(b,p);c=_mm_sub_ps(c,p);
    edge=dot(cross(a,b),n);if(!isfinite(edge))return -1;if(edge<tolerance)return 0;
    edge=dot(cross(b,c),n);if(!isfinite(edge))return -1;if(edge<tolerance)return 0;
    edge=dot(cross(c,a),n);if(!isfinite(edge))return -1;if(edge<tolerance)return 0;
    if(squared<FLT_MIN)return -1;
    if(ds<=0)n=_mm_xor_ps(n,_mm_set1_ps(-0.0f));
    r=control?_mm_set_ss(1.0f/sqrtf(squared)):_mm_rsqrt_ss(_mm_set_ss(squared));
    r=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),r),
        _mm_sub_ss(_mm_set_ss(3),_mm_mul_ss(_mm_mul_ss(_mm_set_ss(squared),r),r)));
    _mm_storeu_ps(next.normal,_mm_mul_ps(n,_mm_shuffle_ps(r,r,0)));
    if(!finite_values(next.normal,4))return -1;
    next.shape_key=UINT32_MAX;next.fraction=f;*out=next;return 1;
}
int uc_collision_triangle_cast(const float v[3][3],const float from[3],
 const float to[3],float limit,UCCollisionTriangleHit*out){return triangle_cast(v,from,to,limit,out,0);}
int uc_collision_triangle_cast_control(const float v[3][3],const float from[3],
 const float to[3],float limit,UCCollisionTriangleHit*out){return triangle_cast(v,from,to,limit,out,1);}
int uc_collision_triangle_cast_world(const float v[3][3],const float from[3],
    const float to[3],float limit,const float axes[3][4],uint32_t key,
    UCCollisionTriangleHit *out) {
    UCCollisionTriangleHit next;int hit;
    if(!out||!axes||!finite_values(&axes[0][0],12))return -1;
    hit=uc_collision_triangle_cast(v,from,to,limit,&next);if(hit!=1)return hit;
    return uc_collision_triangle_world_hit(&next,axes,key,out);
}
int uc_collision_triangle_world_hit(const UCCollisionTriangleHit *local,
    const float axes[3][4],uint32_t key,UCCollisionTriangleHit *out) {
    if(!local||!out||!axes||!finite_values(&axes[0][0],12)||!finite_values(local->normal,4))return -1;
    if((_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return -1;
    UCCollisionTriangleHit next=*local;__m128 n;
    n=_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_loadu_ps(axes[0]),_mm_set1_ps(next.normal[0])),
                           _mm_mul_ps(_mm_loadu_ps(axes[1]),_mm_set1_ps(next.normal[1]))),
                           _mm_mul_ps(_mm_loadu_ps(axes[2]),_mm_set1_ps(next.normal[2])));
    _mm_storeu_ps(next.normal,n);if(!finite_values(next.normal,4))return -1;
    next.shape_key=key;*out=next;return 1;
}
