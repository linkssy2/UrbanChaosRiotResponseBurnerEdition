#include "collision_bounds.h"
#include "collision_mesh.h"
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <xmmintrin.h>
UCCollisionStatus uc_collision_bounds(const UCCollisionAsset *asset,const float m[4][4],
    float tolerance,UCCollisionBounds *out) {
    if(!asset||!m||!out||!asset->subparts||!asset->subpart_count||!asset->triangle_count||
       asset->subpart_index_bits<1||asset->subpart_index_bits>31||
       asset->subpart_count>(1u<<asset->subpart_index_bits)||
       !isfinite(tolerance)||tolerance<0||!isfinite(asset->radius)||asset->radius<0||
       (_mm_getcsr()&0xe040u)||fegetround()!=FE_TONEAREST)return UC_COLLISION_ARGUMENT;
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)if(!isfinite(m[i][j]))return UC_COLLISION_ARGUMENT;
    float half=(float)((double)tolerance*.5),margin=(float)((double)half+asset->radius);
    if(!isfinite(margin))return UC_COLLISION_UNSUPPORTED;
    __m128 minimum=_mm_set_ps(0,FLT_MAX,FLT_MAX,FLT_MAX),maximum=_mm_set_ps(0,-FLT_MAX,-FLT_MAX,-FLT_MAX);
    __m128 x=_mm_loadu_ps(m[0]),y=_mm_loadu_ps(m[1]),z=_mm_loadu_ps(m[2]),translation=_mm_loadu_ps(m[3]);
    uint32_t count=0,shift=32-asset->subpart_index_bits;
    for(uint32_t p=0;p<asset->subpart_count;p++){
        const UCCollisionSubpart *part=&asset->subparts[p];
        if(part->first_triangle!=count||part->triangle_count>asset->triangle_count-count||
           part->triangle_count>(1u<<shift))return UC_COLLISION_FORMAT;
        for(uint32_t t=0;t<part->triangle_count;t++){
            UCCollisionMeshChild child;UCCollisionStatus status=uc_collision_mesh_child(asset,(p<<shift)|t,&child);
            if(status)return status;
            for(unsigned v=0;v<3;v++){
                __m128 point=_mm_add_ps(_mm_add_ps(_mm_mul_ps(x,_mm_set1_ps(child.vertices[v][0])),
                    _mm_mul_ps(y,_mm_set1_ps(child.vertices[v][1]))),
                    _mm_add_ps(_mm_mul_ps(z,_mm_set1_ps(child.vertices[v][2])),translation));
                float lanes[4];_mm_storeu_ps(lanes,point);
                for(unsigned i=0;i<4;i++)if(!isfinite(lanes[i]))return UC_COLLISION_UNSUPPORTED;
                minimum=_mm_min_ps(minimum,point);maximum=_mm_max_ps(maximum,point);
            }
        }
        count+=part->triangle_count;
    }
    if(count!=asset->triangle_count)return UC_COLLISION_FORMAT;
    UCCollisionBounds next;_mm_storeu_ps(next.minimum,_mm_sub_ps(minimum,_mm_set1_ps(margin)));
    _mm_storeu_ps(next.maximum,_mm_add_ps(maximum,_mm_set1_ps(margin)));
    for(unsigned i=0;i<4;i++)if(!isfinite(next.minimum[i])||!isfinite(next.maximum[i]))return UC_COLLISION_UNSUPPORTED;
    *out=next;return UC_COLLISION_OK;
}
