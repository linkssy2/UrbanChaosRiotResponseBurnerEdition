#include "player_footsteps.h"
#include "player_audio.h"
#include <cfenv>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
}
NativePlayerFootsteps::NativePlayerFootsteps(const UCCollisionAsset &world,NativePlayerAudio &audio)
    :world_(world),audio_(audio){
    require(world.triangles&&world.vertices&&world.triangle_count,"Footsteps need a live static world");
    for(uint32_t i=0;i<world.triangle_count;i++){
        const uint32_t material=(uint16_t)(world.triangles[i].material>>12);
        require(material<36,"Footstep world material outside source name table");
    }
}
bool NativePlayerFootsteps::step(const float frame[16],uint32_t side){
    require(frame&&side<2&&(_mm_getcsr()&0xE040u)==0&&std::fegetround()==FE_TONEAREST,"Invalid footstep frame/side/FP mode");
    for(unsigned i=0;i<16;i++)require(std::isfinite(frame[i]),"Nonfinite footstep frame");
    float start[3],end[3],from[3],to[3];const float offset=side?-40.f:40.f;
    // C3310 ->3BD10 exact scalar grouping. Category31/mask2000/zero impulse
    // belong to the original query; this preview uses the actual static mesh.
    for(unsigned i=0;i<3;i++){
        const float x=(frame[i]*offset+frame[4+i]*0.f)+(frame[8+i]*0.f+0.f);
        start[i]=end[i]=x+frame[12+i];
    }
    start[1]+=30.f;end[1]-=1000.f;
    uc_collision_to_physics(start,from);uc_collision_to_physics(end,to);
    UCCollisionRayHit hit{};const int result=uc_collision_raycast(&world_,from,to,&hit);
    require(result>=0,"Footstep static query failed");
    require(!result||hit.triangle<world_.triangle_count,"Footstep triangle outside world");
    // Actual2A7470 projection; low12 bits are collision filters, not material.
    const uint32_t material=result?(uint16_t)(world_.triangles[hit.triangle].material>>12):0;
    ++counts_.cycles;counts_.hits+=result!=0;counts_.misses+=result==0;
    counts_.material=material;counts_.side=side;
    if(material==30){++counts_.unsupported;return false;}
    const auto before=audio_.started();audio_.footstep(material);
    require(audio_.started()==before+1,"Footstep sound did not start");
    ++counts_.started;return true;
}
