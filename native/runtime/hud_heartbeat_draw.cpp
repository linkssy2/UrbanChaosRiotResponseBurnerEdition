#include "hud_heartbeat_draw.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
NativeHeartbeatQuads native_heartbeat_quads(const UCHudHeartbeat &h,const float color[4]){
    if(!color||!std::isfinite(h.elapsed)||h.elapsed<0)throw std::invalid_argument("Invalid heartbeat draw state");
    for(unsigned j=0;j<4;j++)if(!std::isfinite(color[j])||color[j]<0||color[j]>1)throw std::invalid_argument("Invalid heartbeat color");
    float t=h.elapsed/.44999998807907104f;t=std::clamp(t,0.f,1.f);t=1-t;t=1-t;
    const float ease=t>=1?0:t<=0?1:(float)(1.0-sin((double)t*1.5707963705062866));
    const float x=49-(640.f*ease),y=34+14.f;
    NativeHeartbeatQuads result;
    for(unsigned i=0;i<53;i++){
        if(!std::isfinite(h.samples[i]))throw std::invalid_argument("Invalid heartbeat sample");
        for(unsigned j=0;j<4;j++)if(!std::isfinite(h.colors[i][j])||h.colors[i][j]<0||h.colors[i][j]>1)throw std::invalid_argument("Invalid heartbeat sample color");
    }
    for(unsigned i=1;i<53;i++){
        float rgba[4];for(unsigned j=0;j<4;j++)rgba[j]=h.colors[i][j]*color[j];
        if(rgba[3]<=0)continue;
        uint32_t c[4];for(unsigned j=0;j<4;j++)c[j]=(uint32_t)(rgba[j]*255.f);
        float a=y+h.samples[i],b=y+h.samples[i-1];if(a>b)std::swap(a,b);
        const float left=std::floor((float)((double)x+i)),top=std::floor(a-1),right=std::floor((float)((double)x+i)+2),bottom=std::floor(b+2);
        result.quads[result.count++]={left-.5f,top-.5f,right-left,bottom-top,0,0,1,1,6,(c[3]<<24)|(c[0]<<16)|(c[1]<<8)|c[2]};
    }
    return result;
}
NativeHudAmmo native_lifeline_pulse(const UCHudHeartbeat &h,const UCHudHealth &health){
    native_hud_health(health); // Shared snapshot validation, including colors.
    if(!std::isfinite(h.pulse)||h.pulse<0||!std::isfinite(h.previous_health)||h.previous_health<0)
        throw std::invalid_argument("Invalid lifeline pulse");
    NativeHudAmmo result;
    if(h.previous_health==0||health.fraction==0||h.pulse>=.3f)return result;
    const float width=std::floor(103+health.extent*std::min(health.fraction,1.f))-103;
    const float tail=std::max(32.f,width/6),head=(width+tail)*(h.pulse/.3f);
    const auto bright=[&](unsigned i){return (uint32_t)(std::min(1.f,health.color[i]*1.2f)*255);};
    const uint32_t rgb=(bright(0)<<16)|(bright(1)<<8)|bright(2);
    for(unsigned i=0;i<4;i++){
        const float left=std::clamp(head-tail+tail*i/4,0.f,width),right=std::clamp(head-tail+tail*(i+1)/4,0.f,width);
        if(right>left)result.quads[result.count++]={102.5f+left,46.5f,right-left,3,0,0,1,1,6,
            ((uint32_t)(health.color[3]*255*(i+1)/4)<<24)|rgb};
    }
    return result;
}
void NativeHeartbeat::load(NativeRenderer &r){
    NativeVertex vertices[224]{};uint32_t indices[336];
    for(unsigned q=0;q<56;q++){const uint32_t ix[]={0,1,2,0,2,3};for(unsigned i=0;i<6;i++)indices[q*6+i]=q*4+ix[i];}
    auto next=r.upload(vertices,224,indices,336,true);mesh=std::move(next);uc_hud_heartbeat_initialize(&history);
}
void NativeHeartbeat::update(const UCHudHeartbeatActor &a,float dt,UCGameRandom &rng,void (*sound)(void *),void *ctx){
    if(!uc_hud_heartbeat_step(&history,&a,dt,&rng,sound,ctx))throw std::invalid_argument("Invalid heartbeat update");
}
void NativeHeartbeat::draw(NativeRenderer &r,ID3D11ShaderResourceView *texture,const UCHudHealth &health){
    auto result=native_heartbeat_quads(history,health.color);
    const auto pulse=native_lifeline_pulse(history,health);
    for(size_t i=0;i<pulse.count;i++)result.quads[result.count++]=pulse.quads[i];
    if(!result.count)return;
    if(!mesh.vertices||!texture)throw std::logic_error("Heartbeat resources unavailable");
    NativeVertex vertices[224]{};const float scale=std::min(r.width/640.f,r.height/480.f),ox=0,oy=(r.height-480*scale)*.5f;
    for(size_t i=0;i<result.count;i++){
        const auto &q=result.quads[i];const float x=ox+(q.x+.5f)*scale,y=oy+(q.y+.5f)*scale;
        const float xy[4][2]={{x,y},{x+q.width*scale,y},{x+q.width*scale,y+q.height*scale},{x,y+q.height*scale}};
        const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
        for(unsigned j=0;j<4;j++){auto &v=vertices[i*4+j];v.position[0]=xy[j][0];v.position[1]=xy[j][1];v.position[2]=0;v.color=q.color;memcpy(v.uv,uv[j],8);}
    }
    r.update(mesh,vertices,224);const float matrix[16]={2.f/r.width,0,0,0,0,-2.f/r.height,0,0,0,0,1,0,-1,1,0,1},white[4]={1,1,1,1};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.vertex_exposure=true;state.write_alpha=false;
    r.draw(mesh,matrix,white,texture,0,(uint32_t)result.count*6,state);
}
