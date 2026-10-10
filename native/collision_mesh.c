#include "collision_mesh.h"
#include <fenv.h>
#include <math.h>
#include <float.h>
#include <xmmintrin.h>
static UCCollisionStatus locate(const UCCollisionAsset *a,uint32_t key,
    const UCCollisionTriangle **triangle,const UCCollisionSubpart **part) {
    uint32_t subpart,local;
    if(!a||!a->subparts||!a->triangles)return UC_COLLISION_ARGUMENT;
    if(a->subpart_index_bits==0||a->subpart_index_bits>=32)return UC_COLLISION_UNSUPPORTED;
    subpart=key>>(32-a->subpart_index_bits);local=key&(UINT32_MAX>>a->subpart_index_bits);
    if(subpart>=a->subpart_count)return UC_COLLISION_FORMAT;
    *part=&a->subparts[subpart];
    if((*part)->first_triangle>a->triangle_count
       ||(*part)->triangle_count>a->triangle_count-(*part)->first_triangle
       ||local>=(*part)->triangle_count)return UC_COLLISION_FORMAT;
    *triangle=&a->triangles[(*part)->first_triangle+local];
    if((*triangle)->subpart!=subpart||(*triangle)->local_triangle!=local)return UC_COLLISION_FORMAT;
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_mesh_child(const UCCollisionAsset *a,uint32_t key,
    UCCollisionMeshChild *out) {
    const UCCollisionTriangle *t;const UCCollisionSubpart *p;
    UCCollisionStatus status;
    if(!out)return UC_COLLISION_ARGUMENT;
    status=locate(a,key,&t,&p);if(status)return status;
    if(!a->vertices)return UC_COLLISION_ARGUMENT;
    if(p->first_vertex>a->vertex_count||p->vertex_count>a->vertex_count-p->first_vertex)
        return UC_COLLISION_FORMAT;
    float vertices[3][3];
    for(unsigned i=0;i<3;++i){
        if(t->vertex[i]<p->first_vertex||t->vertex[i]-p->first_vertex>=p->vertex_count)
            return UC_COLLISION_FORMAT;
        for(unsigned j=0;j<3;++j)vertices[i][j]=a->vertices[t->vertex[i]][j];
    }
    return uc_collision_mesh_scale(vertices,a->scaling,a->radius,out);
}
UCCollisionStatus uc_collision_mesh_scale(const float vertices[3][3],const float scaling[3],
    float radius,UCCollisionMeshChild *out){
    if(!vertices||!scaling||!out)return UC_COLLISION_ARGUMENT;
    if((_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return UC_COLLISION_UNSUPPORTED;
    if(!isfinite(radius)||radius<0)return UC_COLLISION_UNSUPPORTED;
    UCCollisionMeshChild next;next.radius=radius;
    __m128 scale=_mm_set_ps(0,scaling[2],scaling[1],scaling[0]);
    __m128 sign=_mm_set1_ps(-0.0f),maximum=_mm_set1_ps(FLT_MAX);
    if(_mm_movemask_ps(_mm_cmple_ps(_mm_andnot_ps(sign,scale),maximum))!=15)return UC_COLLISION_UNSUPPORTED;
    for(unsigned i=0;i<3;++i){
        __m128 v=_mm_set_ps(0,vertices[i][2],vertices[i][1],vertices[i][0]);
        if(_mm_movemask_ps(_mm_cmple_ps(_mm_andnot_ps(sign,v),maximum))!=15)return UC_COLLISION_UNSUPPORTED;
        v=_mm_mul_ps(v,scale);
        if(_mm_movemask_ps(_mm_cmple_ps(_mm_andnot_ps(sign,v),maximum))!=15)return UC_COLLISION_UNSUPPORTED;
        float lanes[4];_mm_storeu_ps(lanes,v);
        for(unsigned j=0;j<3;j++)next.vertices[i][j]=lanes[j];
    }
    *out=next;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_mesh_material(const UCCollisionAsset *a,uint32_t key,
    uint32_t *out) {
    const UCCollisionTriangle *t;const UCCollisionSubpart *p;UCCollisionStatus status;
    if(!out)return UC_COLLISION_ARGUMENT;
    status=locate(a,key,&t,&p);if(status)return status;
    *out=t->material;return UC_COLLISION_OK;
}
