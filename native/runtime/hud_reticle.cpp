#include "hud_reticle.h"
#include "../texture_asset/texture_asset.h"
#include <algorithm>
#include <bcrypt.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace {
void validate(const NativeReticleHistory &h){
    for(const auto &f:h.items)if(!std::isfinite(f.alpha)||f.alpha<0||f.alpha>1||
        !std::isfinite(f.target)||f.target<0||f.target>1||f.mode>1)
        throw std::invalid_argument("Invalid reticle history");
}
void target(NativeReticleFade &f,float value){if(f.target!=value){f.target=value;f.mode=1;}}
void resource(const UCArchive *a,const char *name,const char *expected,BytesOwner &b){
    const size_t at=find_resource(a,name);
    if(at==SIZE_MAX||uc_archive_read(a,at,UC_AR_LOCAL_PAYLOAD,&b.value)!=UC_AR_OK)throw std::runtime_error("Missing reticle resource");
    BCRYPT_ALG_HANDLE alg=nullptr;unsigned char hash[32];char hex[65];
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
    auto status=BCryptHash(alg,nullptr,0,b.value.data,(ULONG)b.value.size,hash,32);BCryptCloseAlgorithmProvider(alg,0);
    if(status<0)throw std::runtime_error("Reticle identity check failed");
    for(unsigned i=0;i<32;i++)sprintf_s(hex+i*2,3,"%02x",hash[i]);
    if(strcmp(hex,expected))throw std::runtime_error("Unsupported reticle resource revision");
}
}
void native_reticle_step(NativeReticleHistory &h,const UCMainplayerState &inventory,
    const UCMainplayerActionState &action,float dt,uint8_t opacity){
    validate(h);
    if(!std::isfinite(dt)||dt<0||dt>1||opacity>8||
       (inventory.active_item!=-1&&inventory.active_item!=0&&inventory.active_item!=1&&inventory.active_item!=2&&inventory.active_item!=11&&inventory.active_item!=17)||
       (inventory.active_item>=0&&!inventory.owned[inventory.active_item]))
        throw std::invalid_argument("Unsupported reticle snapshot");
    const bool visible=(inventory.active_item==2||inventory.active_item==17)&&action.state!=3&&action.state!=11&&
        (action.state!=7||action.ready_latch);
    /* Source clears EVERY target before setting the selected one. This also
     * resets its mode even when the final target equals the previous target. */
    for(auto &f:h.items)target(f,0);
    if(visible)target(h.items[inventory.active_item==2?0:1],opacity*.125f);
    for(auto &f:h.items){
        if(!f.mode)f.alpha=f.target;
        else if(f.target>f.alpha)f.alpha=std::min(f.target,f.alpha+(dt*2.f)/.25f);
        else if(f.alpha>f.target)f.alpha=std::max(f.target,f.alpha-(dt*2.f)/.25f);
        else f.mode=0;
    }
}
NativeHudAmmo native_reticle_quads(const NativeReticleHistory &h){
    validate(h);NativeHudAmmo out;
    for(unsigned i=0;i<2;i++)if(h.items[i].alpha>0)
        out.quads[out.count++]={287.5f,207.5f,64,64,0,0,1,1,i,
            ((uint32_t)(h.items[i].alpha*255.f)<<24)|0xffffffu};
    return out;
}
void NativeReticle::load(const UCArchive *a,NativeRenderer &r){
    static const char *names[]={"hud\\hud_crosshair_pistol","hud\\hud_crosshair_shotgun"};
    static const char *sh[]={"3cca3ae09aab8bd8a22715ebaf8a63101e14f938b96d4d3c88e5cc15fbf2c7b6","248ae97977626c54d89598fe32af858942b727570e0aef7a976492274e263487"};
    static const char *th[]={"55297c49fb3d2894fa9a4cffeddb15032335c462662105e4aa9d17808c339903","2e51f4d97222adcafb6a4f982ccce975e472cea1d150345a657bd02749ab68f2"};
    NativeReticle next;
    for(unsigned i=0;i<2;i++){
        BytesOwner shader,bytes;resource(a,(std::string(names[i])+".shader").c_str(),sh[i],shader);
        resource(a,(std::string(names[i])+".tga=nomipmap").c_str(),th[i],bytes);
        struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} t;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&t.value)!=UC_TEXTURE_OK)throw std::runtime_error("Invalid reticle texture");
        next.textures[i]=r.upload(t.value);
    }
    NativeVertex vertices[8]{};const uint32_t indices[]={0,1,2,0,2,3,4,5,6,4,6,7};
    next.mesh=r.upload(vertices,8,indices,12,true);*this=std::move(next);
}
void NativeReticle::draw(NativeRenderer &r){
    const auto quads=native_reticle_quads(history);if(!quads.count)return;
    if(!mesh.vertices||!textures[0]||!textures[1])throw std::logic_error("Reticle resources unavailable");
    NativeVertex vertices[8]{};const float scale=std::min(r.width/640.f,r.height/480.f),ox=(r.width-640*scale)*.5f,oy=(r.height-480*scale)*.5f;
    for(size_t i=0;i<quads.count;i++){
        const auto &q=quads.quads[i];const float x=ox+(q.x+.5f)*scale,y=oy+(q.y+.5f)*scale;
        const float xy[4][2]={{x,y},{x+64*scale,y},{x+64*scale,y+64*scale},{x,y+64*scale}},uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
        for(unsigned j=0;j<4;j++){auto &v=vertices[i*4+j];v.position[0]=xy[j][0];v.position[1]=xy[j][1];v.color=q.color;memcpy(v.uv,uv[j],8);}
    }
    r.update(mesh,vertices,8);const float m[16]={2.f/r.width,0,0,0,0,-2.f/r.height,0,0,0,0,1,0,-1,1,0,1},white[4]={1,1,1,1};
    NativeDrawState state;state.depth_disabled=state.point_filter=state.clamp_u=state.clamp_v=true;
    state.blend_mode=1;state.vertex_exposure=true;state.write_alpha=false;
    for(size_t i=0;i<quads.count;i++)r.draw(mesh,m,white,textures[quads.quads[i].texture].Get(),(uint32_t)i*6,6,state);
}
