#include "player_footstep.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_player_footstep(UCPlayerFootstep *state,const UCPlayerFootstepServices *h){
    if(!state||!h||!h->preflight||!h->query||!h->footprint||state->side>1||
       !finite_values(state->body,16)||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    float start[3],end[3],forward[4];const float offset=state->side?-40.f:40.f;
    for(unsigned i=0;i<3;i++){
        float x=(state->body[i]*offset+state->body[4+i]*0.f)+(state->body[8+i]*0.f+0.f);
        start[i]=end[i]=x+state->body[12+i];
    }
    start[1]=start[1]+30.f;end[1]=end[1]-1000.f;
    /* Actual2D3DC0(0, bitsBFC90FDB,0, {0,0,0,1}); immutable quarter-turn. */
    static const float rotation[16]={-4.371138828673793e-08f,0.f,-1.f,0.f,
        0.f,1.f,0.f,0.f,1.f,0.f,-4.371138828673793e-08f,0.f,0.f,0.f,0.f,1.f};
    const float *v=state->body+8;
    _mm_storeu_ps(forward,_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_loadu_ps(rotation),_mm_set1_ps(v[0])),
        _mm_mul_ps(_mm_loadu_ps(rotation+4),_mm_set1_ps(v[1]))),
        _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(rotation+8),_mm_set1_ps(v[2])),
        _mm_mul_ps(_mm_loadu_ps(rotation+12),_mm_set1_ps(v[3])))));
    if(!finite_values(start,3)||!finite_values(end,3)||!finite_values(forward,4)||!h->preflight(h->context,state))return 1;
    const UCCollisionResult *hit=h->query(h->context,start,end,0x31,state->actor_id,0x2000,0.f);
    state->material=hit?hit->material:0;
    if(!hit||hit->phantom||(hit->material!=19&&hit->material!=21))return 0;
    if(state->side>1||!finite_values(hit->point,3)||!finite_values(hit->normal,3))return 1;
    UCPlayerFootprint out;float tangent[3];
    for(unsigned i=0;i<3;i++){unsigned a=(i+1)%3,b=(i+2)%3;tangent[i]=hit->normal[a]*forward[b]-hit->normal[b]*forward[a];}
    double x=tangent[0],y=tangent[1],z=tangent[2],length=sqrt((z*z+y*y)+x*x);
    if(!(length>0)||!isfinite(length))return 1;
    double reciprocal=1.0/length;for(unsigned i=0;i<3;i++)out.tangent[i]=(float)(tangent[i]*reciprocal);
    if(!finite_values(out.tangent,3))return 1;
    out.shader=state->side?"FX\\FX_Footprints\\FX_Footprint_left.shader":"FX\\FX_Footprints\\FX_Footprint_right.shader";
    memcpy(out.point,hit->point,sizeof out.point);memcpy(out.normal,hit->normal,sizeof out.normal);
    out.size[0]=50.f;out.size[1]=20.f;out.mask=0x2000;out.flags=0;
    h->footprint(h->context,&out);return 0;
}
