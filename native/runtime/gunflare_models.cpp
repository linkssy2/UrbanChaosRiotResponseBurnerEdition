#include "gunflare_models.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
static const char *const names[]={
    "Assets_PlayerWeaponsAndEquipment\\EQP_Police_Assault_Riffle\\EQP_AssaultRiffle_muzzleflash_FP_01_Skin.amf",
    "Assets_PlayerWeaponsAndEquipment\\EQP_HandGun\\EQP_HandGun_muzzleflash_FP_01_Skin.AMF"};
static bool finite(const float *p,unsigned n){if(!p)return false;for(unsigned i=0;i<n;++i)if(!std::isfinite(p[i]))return false;return true;}
static XMMATRIX matrix(const float *p){XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
NativeGunFlareModels::Instance *NativeGunFlareModels::find(void *p){
    if(ready)for(auto &i:instances)if(p==&i)return &i;
    return nullptr;
}
void NativeGunFlareModels::load(NativeRenderer &r,const UCArchive *a,const uint8_t hash[32],
    const std::filesystem::path &catalog,const float world[16]){
    if(ready||instances[0].model.source.vertices||instances[1].model.source.vertices||!finite(world,16))
        throw std::invalid_argument("Invalid fresh GunFlare model owner");
    const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for(unsigned order=0;order<2;++order){
        unsigned index=1-order;auto &i=instances[index];
        i.model.load(r,a,hash,catalog,names[index],nullptr);
        for(const auto &m:i.model.materials)if(!m.supported()||!m.state.blend_mode||!m.state.vertex_exposure)
            throw std::runtime_error("Unsupported GunFlare material");
        // Existing loader admits source header+8==1; B54F0 returns that count.
        // B5510 group0 is a no-op in this exact single-group domain.
        i.model.bind_pose(r,identity);memcpy(i.world,world,64);
        handles[index]=&i;groups[index]={names[index],handles+index,1};
    }
    ready=true;
}
int NativeGunFlareModels::tint(void *p,const float rgba[4]){auto *i=find(p);if(!i||!finite(rgba,4))return 1;memcpy(i->tint,rgba,16);return 0;}
int NativeGunFlareModels::visible(void *p,unsigned enabled){auto *i=find(p);if(!i||enabled>1)return 1;i->visible=enabled!=0;return 0;}
int NativeGunFlareModels::transform(void *p,const float world[16]){auto *i=find(p);if(!i||!finite(world,16))return 1;memcpy(i->world,world,64);return 0;}
void NativeGunFlareModels::draw(NativeRenderer &r,const float view[16],const float projection[16],float exposure){
    float sliced[16];
    if(!ready||!finite(view,16)||!std::isfinite(exposure)||exposure<=0||
       uc_firstperson_depth_slice(projection,.1f,.15f,sliced))throw std::invalid_argument("Invalid GunFlare draw parent");
    for(auto &i:instances)if(i.visible){
        for(auto &m:i.model.materials)m.state.inverse_exposure=exposure;
        XMFLOAT4X4 clip;XMStoreFloat4x4(&clip,matrix(i.world)*matrix(view)*matrix(sliced));
        i.model.draw(r,&clip._11,i.tint);
    }
}
