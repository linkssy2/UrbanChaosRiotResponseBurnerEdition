#include "objective_icon.h"
#include "../texture_asset/texture_asset.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

int native_objective_icon_begin(NativeObjectiveIconState&s,uint32_t kind,uint32_t color){
    if(kind>2)return 1;
    s={};s.kind=kind;s.color=kind?0xffffffffu:color;s.grow=kind==1?2.f:1.f;
    UCUINotificationPositions p;uc_ui_notification_positions(&p);
    return uc_ui_motion_begin(&s.motion,p.icon,kind?.8f:.5f,kind?4:6,1);
}
static void resize(float*p,float scale){
    const float x=(p[0]+p[2])*.5f,y=(p[1]+p[3])*.5f;
    const float dx=(p[2]-x)*scale,dy=(p[3]-y)*scale;
    p[0]=x-dx;p[1]=y-dy;p[2]=dx+x;p[3]=dy+y;
}
int native_objective_icon_step(NativeObjectiveIconState&s,float delta,NativeObjectiveIconQuad&q){
    if(s.kind>2||!std::isfinite(s.elapsed)||uc_ui_motion_step(&s.motion,delta))return 1;
    q={};q.rectangle[0]=s.motion.x;q.rectangle[1]=s.motion.y;q.rectangle[2]=32.f+s.motion.x;q.rectangle[3]=s.motion.y+32.f;
    if(!s.kind){
        resize(q.rectangle,s.motion.scale);
        q.color=(s.color&0xffffffu)|(uint32_t(int(float(s.color>>24)*s.motion.alpha))<<24);q.draw=true;return 0;
    }
    if(s.motion.progress<=0)return 0;
    const float previous=s.elapsed;s.elapsed+=delta;s.started=previous==0;s.settled=0;q.color=0xffffffffu;q.draw=true;
    if(s.elapsed<.3f){
        const float fraction=s.elapsed*3.3333332538604736328125f;
        resize(q.rectangle,s.grow);
        q.color=(uint32_t(int(std::min(fraction*3.f,1.f)*255.f))<<24)|0xffffffu;
    }else if(s.elapsed<.4f){
        float scale=(s.elapsed-.3f)*10.00000095367431640625f;
        scale=scale*(1.f-s.grow)+s.grow;resize(q.rectangle,scale);
    }else if(s.elapsed<.8f&&previous<.4f)s.settled=1;
    if(s.elapsed>1.5f)s.finished=1;
    return 0;
}
void NativeObjectiveIcons::load(const UCArchive*a,NativeRenderer&r){
    static const char*names[]={"frontend\\frt_content\\tick_box","frontend\\frt_content\\tick","frontend\\frt_content\\cross","frontend\\frt_content\\lc_medal02"};
    static const char*shaders[]={"fd1af4124e13ddd3827246bcc36bcaa27edd1c3b9823fa592fa7d451e6af8148","9d990b4791565a53dae0c728247f32f29b7a2075d866b030b602780dd0fed7fe","db43003434ecbd5dbe0cea0e6cf9869c04f87c1feb7551135229106d08ce3783"};
    static const char*textures[]={"feadabe450de3e2862aa8f8356bba0cbebbf19aefc9978e880716b3fadf7d108","d37e9b128d813f9bd5ec39bec0e6ad43b52b93ce52628f2d10673b018280f5b0","d1fb139e5dc00308ebcb452a17173ba1ae5958352b4cf31bdc41326cc85e21c0"};
    ComPtr<ID3D11ShaderResourceView> loaded[4];
    for(unsigned i=0;i<4;i++){
        BytesOwner shader,bytes;checked_resource(a,(std::string(names[i])+".shader").c_str(),i<3?shaders[i]:"0fd17fde9a1b48229d0ab2b569d67dcb23ab066a34ce5e8bde3d02b8e37c481d",shader);
        checked_resource(a,(std::string(names[i])+".tga").c_str(),i<3?textures[i]:"6605fa51b8170f84c1c8ea7c5b2af69110c9b3f1f16a2623e54ae8595e084ec1",bytes);
        struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} t;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&t.value))throw std::runtime_error("Objective icon texture");
        loaded[i]=r.upload(t.value);
    }
    const NativeVertex vertices[4]{};const uint32_t indices[]={0,1,2,0,2,3};auto mesh=r.upload(vertices,4,indices,6,true);
    for(unsigned i=0;i<4;i++)textures_[i]=std::move(loaded[i]);mesh_=std::move(mesh);clear();
}
void NativeObjectiveIcons::begin(int32_t type,uint32_t color,int32_t role){
    if(type<0||type>4||!mesh_.vertices||(role!=-1&&role!=1))throw std::invalid_argument("Objective icon begin");count_=0;
    auto add=[&](unsigned kind,unsigned texture,uint32_t tint){if(native_objective_icon_begin(icons_[count_],kind,tint))throw std::logic_error("Objective icon");texture_indices_[count_++]=texture;};
    if(role==1)add(0,3,0xffffffffu); //77720 actual Sharp Shooter medal.
    if(role==-1||type!=2)add(0,0,color);
    if(type>=3)add(type==3?1u:2u,type==3?1u:2u,color);
}
void NativeObjectiveIcons::draw(NativeRenderer&r,float delta){
    const float scale=std::min(r.width/640.f,r.height/480.f),ox=(r.width-640*scale)*.5f,oy=(r.height-480*scale)*.5f;
    const float matrix[16]={2.f*scale/r.width,0,0,0,0,-2.f*scale/r.height,0,0,0,0,1,0,-1+2.f*ox/r.width,1-2.f*oy/r.height,0,1},white[4]={1,1,1,1};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.vertex_exposure=true;state.write_alpha=false;
    //Exact shader family: exposure vertex, Texture0*vertexRGBA, ordinary
    //bilinear wrap sampler, SRC_ALPHA/INV_SRC_ALPHA. RGB UI parent owns depth.
    bool live=false;for(unsigned i=0;i<count_;i++){
        if(icons_[i].motion.finishing&&icons_[i].motion.phase<=0)continue; //274320 before draw
        live=true;
        NativeObjectiveIconQuad q;if(native_objective_icon_step(icons_[i],delta,q))throw std::runtime_error("Objective icon update");
        if(!q.draw)continue;for(float&v:q.rectangle)v=std::floor(v); //65550, then D3D11 center conversion
        const float xy[4][2]={{q.rectangle[0],q.rectangle[1]},{q.rectangle[2],q.rectangle[1]},{q.rectangle[2],q.rectangle[3]},{q.rectangle[0],q.rectangle[3]}};
        const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};NativeVertex v[4]{};
        for(unsigned j=0;j<4;j++){std::memcpy(v[j].position,xy[j],8);std::memcpy(v[j].uv,uv[j],8);v[j].color=q.color;}
        r.update(mesh_,v,4);r.draw(mesh_,matrix,white,textures_[texture_indices_[i]].Get(),0,6,state);
    }
    if(!live)count_=0;
}
