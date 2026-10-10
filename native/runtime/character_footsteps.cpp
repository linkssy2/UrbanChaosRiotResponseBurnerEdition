#include "character_footsteps.h"
#include <cfenv>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
void require(bool okay,const char *why){if(!okay)throw std::runtime_error(why);}
bool finite(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!std::isfinite(p[i]))return false;return true;}
void transform(const float*m,const float*v,float*out){
    _mm_storeu_ps(out,_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_loadu_ps(m),_mm_set1_ps(v[0])),
        _mm_mul_ps(_mm_loadu_ps(m+4),_mm_set1_ps(v[1]))),
        _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(m+8),_mm_set1_ps(v[2])),
        _mm_mul_ps(_mm_loadu_ps(m+12),_mm_set1_ps(v[3])))));
}
}
void native_character_footstep(NativeCharacterFootstep&s,const NativeCharacterFootstepServices&h){
    require(s.world&&h.bone&&h.query&&h.footprint&&finite(s.world,16)&&
        (_mm_getcsr()&0xE040u)==0&&std::fegetround()==FE_TONEAREST,"NPC footstep inputs/services");
    float bone[16],position[4],forward[4],from[3],to[3];
    h.bone(h.owner,s.left?"Bip01 L Foot":"Bip01 R Foot",bone);
    require(finite(bone,16),"NPC footstep pose");
    transform(s.world,bone+12,position);transform(s.world,bone+8,forward);
    std::memcpy(from,position,12);std::memcpy(to,position,12);from[1]+=30;to[1]-=1000;
    require(finite(from,3)&&finite(to,3)&&finite(forward,4),"NPC footstep world pose");
    const auto*hit=h.query(h.owner,from,to,0x31,s.group,0x2000,0);
    s.material=hit?hit->material:0;
    if(!hit||hit->phantom||(hit->material!=19&&hit->material!=21))return;
    require(finite(hit->point,3)&&finite(hit->normal,3),"NPC footstep result geometry");
    float tangent[3];for(unsigned i=0;i<3;i++){unsigned a=(i+1)%3,b=(i+2)%3;tangent[i]=hit->normal[a]*forward[b]-hit->normal[b]*forward[a];}
    double x=tangent[0],y=tangent[1],z=tangent[2],length=std::sqrt((z*z+y*y)+x*x);
    require(length>0&&std::isfinite(length),"NPC footprint tangent");
    UCPlayerFootprint p{};double reciprocal=1.0/length;
    for(unsigned i=0;i<3;i++)p.tangent[i]=(float)(tangent[i]*reciprocal);
    require(finite(p.tangent,3),"NPC footprint normalization");
    p.shader=s.right?"FX\\FX_Footprints\\FX_Footprint_right.shader":"FX\\FX_Footprints\\FX_Footprint_left.shader";
    std::memcpy(p.point,hit->point,12);std::memcpy(p.normal,hit->normal,12);
    p.size[0]=50;p.size[1]=20;p.mask=0x80;h.footprint(h.owner,p);
}
