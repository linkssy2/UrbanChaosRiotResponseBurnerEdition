#include "hud.h"
#include "input_bindings.h"
#include "../texture_asset/texture_asset.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

NativeHudAmmo native_hud_ammo(const UCMainplayerState &s,const UCMainplayerItemSpec &spec) {
    if(s.active_item==11){
        if(!s.owned[11]||!spec.available||spec.category!=1||s.clip[11]>INT_MAX)
            throw std::invalid_argument("Invalid throwable HUD snapshot");
        NativeHudAmmo result;
        for(unsigned i=0;i<2;i++){
            const unsigned digit=i?s.clip[11]%10:s.clip[11]/10%10;
            result.quads[result.count++]={497.5f+19*i,412.5f,19,27,digit*19.f/256,0,(digit+1)*19.f/256,27.f/32,0};
        }
        result.quads[result.count++]={554.5f,411.5f,32,32,0,0,1,1,14};return result;
    }
    if(s.active_item!=2&&s.active_item!=17)return {};
    const unsigned item=(unsigned)s.active_item,category=item==2?1:5;
    if(!s.owned[item]||!spec.available||spec.category||spec.ammo_category!=category||
       s.clip[item]>INT_MAX||s.ammo[category]>INT_MAX)
        throw std::invalid_argument("Invalid native ammo HUD snapshot");
    const unsigned clip=s.clip[item],reserve=s.ammo[category];
    const unsigned digits[]={clip%100/10,clip%10,reserve%1000/100,reserve%100/10,reserve%10};
    NativeHudAmmo result;
    /* Original root right/bottom alignment,48/36 margins;159x32 ammo group.
     * The constructor's digit arrays are tens,ones and hundreds,tens,ones. */
    const float x=640-48-159,y=480-36-32;
    for(unsigned i=0;i<5;++i) {
        if(i>=2&&!spec.reserve_factor)continue; /* Original43850. */
        const float scale=i<2?1.f:.5f;
        const float dx=i<2?65.f+19.f*i:103.f+9.5f*(i-2);
        const float dy=i<2?1.f:14.f;
        result.quads[result.count++]={std::floor(x+dx)-.5f,std::floor(y+dy)-.5f,19*scale,27*scale,
            digits[i]*19.f/256,0,(digits[i]+1)*19.f/256,27.f/32,0};
    }
    result.quads[result.count++]={x+122-.5f,y-.5f,32,32,0,0,1,1,item==2?1u:2u};
    return result;
}

NativeHudAmmo native_hud_health(const UCHudHealth &h) {
    if(!std::isfinite(h.extent)||h.extent<20||h.extent>2580||!std::isfinite(h.fraction)||h.fraction<0||h.fraction>4||
       !std::isfinite(h.delayed_fraction)||h.delayed_fraction<0||h.delayed_fraction>4||!std::isfinite(h.delay)||h.delay<0||h.delay>1)
        throw std::invalid_argument("Invalid native health HUD snapshot");
    for(float value:h.color)if(!std::isfinite(value)||value<0||value>1)throw std::invalid_argument("Invalid native health HUD color");
    NativeHudAmmo result;
    auto fill=[&](float fraction,const float rgba[4]) {
        if(fraction<=0||rgba[3]<=0)return;
        fraction=std::min(fraction,1.f);uint32_t c[4];
        for(unsigned i=0;i<4;++i)c[i]=(uint32_t)(rgba[i]*255.f); // original871A0 truncation.
        result.quads[result.count++]={102.5f,46.5f,std::floor(103+h.extent*fraction)-103,3,0,0,fraction,1,6,
            (c[3]<<24)|(c[0]<<16)|(c[1]<<8)|c[2]};
    };
    result.quads[result.count++]={47.5f,35.5f,64,21,0,0,1,21.f/32,3};
    const float middle=h.extent-14;
    result.quads[result.count++]={111.5f,35.5f,std::floor(112+middle)-112,21,0,0,1,21.f/32,4};
    result.quads[result.count++]={std::floor(112+middle)-.5f,35.5f,8,21,0,0,1,21.f/32,5};
    // 76A50 nests79770; its7AC30 draw visits second child before first:
    // frame, delayed red, current health, label, heartbeat.
    const float red[4]={1,0,0,h.delay};fill(h.delayed_fraction,red);fill(h.fraction,h.color);
    return result;
}

NativeHudAmmo native_hud_inventory(const UCMainplayerState &s) {
    NativeHudAmmo result;
    const unsigned items[]={2,17,1,11};
    for(unsigned i=0;i<4;++i)if(s.owned[items[i]]){
        // PC direct-selection strip above the original ammo group. Keep the
        // original64x32 art at its0.7 HUD scale and original inactive alpha. 
        const float x=(s.owned[11]?391.f:441.f)+50.f*i;
        const uint32_t color=s.active_item==(int32_t)items[i]?0xffffffffu:0xa5ffffffu;
        result.quads[result.count++]={x-.5f,361.5f,44.8f,22.4f,0,0,1,1,i==3?14u:7+i,color};
        result.quads[result.count++]={x+13.5f,385.5f,18,18,0,0,1,1,i==3?15u:10+i,color};
    }
    return result;
}

void NativeHud::load(const UCArchive *archive,NativeRenderer &renderer) {
    static const char *names[]={"hud\\hud_ammo_numbers","hud\\hud_ammo_pistol","hud\\hud_ammo_shotgun",
        "hud\\hud_health_new01","hud\\hud_health_new02","hud\\hud_health_new03","font\\font_untextured_alpha"};
    static const char *shader_hashes[]={
        "434f5e149d79bbcc7a246645aff0570a59e5af4cda6e2b9f2d009f1383c1b48d",
        "e5e867aa24e66ecc186be07af9bb428faef646beef19f713d5cc608438910c40",
        "0184d41f0e2d58186de25ffe070a6d08a0176483d3b38186255daf1c7186a3be",
        "4b43624b25fae699cda62e8a1e3a3e796e226a8e3768c6cb79fcb37cb5dcbaee",
        "d39e3a79fd8fc71ac46fec0052d9175a3ebd4508b73e1e4360bc19a7a14f9ccb",
        "213abcfe8bbefa0acb401af1b214f05c84adaf2091928115744a0ac269194ebf",
        "d57a561a9c9165f64278fe7123f02d2983067888a00b402fafb382dd6cafc214"};
    static const char *texture_hashes[]={
        "f6b8a03a8dc25a387c992efd0a94fbc21f31cdb742ebebabe023bbcbca3dfbab",
        "816319369df6bcdb54b32488ff9e943988102674513cae41907181303e041541",
        "7d47cd072373f7ab3dd3ccba381ea3a7796dc3a6504983065e7a2f671cefebb0",
        "1b141524b7e5cd550ce465a201c15f962411e919904de7a32b4e2d5e698a4e69",
        "6618ec18df7cc21a6cdffc7700f36837df57dc51b98419ac885149e7597d1285",
        "3665998ba2813b630a7c65340fb81ff98c7dc1a5c43de7c2065dda10266773da",
        "8edc66b1627428cd92209b97d42e9b3d0e392d7a5c09bc48dcea49751f1e326f"};
    NativeHud next;
    for(unsigned i=0;i<7;++i) {
        BytesOwner shader,bytes;
        checked_resource(archive,(std::string(names[i])+".shader").c_str(),shader_hashes[i],shader);
        const auto texture_name=i==6?std::string("font\\font_untextured.tga"):std::string(names[i])+".tga=nomipmap";
        checked_resource(archive,texture_name.c_str(),texture_hashes[i],bytes);
        struct Texture {UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} texture;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&texture.value)!=UC_TEXTURE_OK)
            throw std::runtime_error("Invalid authored HUD texture");
        next.textures[i]=renderer.upload(texture.value);
    }
    NativeVertex vertices[32]{};uint32_t indices[48];
    for(unsigned q=0;q<8;++q){const uint32_t local[]={0,1,2,0,2,3};for(unsigned i=0;i<6;++i)indices[q*6+i]=q*4+local[i];}
    next.mesh=renderer.upload(vertices,32,indices,48,true);
    *this=std::move(next);
}
extern "C" int native_hud_keycap(uint32_t *,unsigned,const char *,const char *);
static constexpr NativeInputBindings::Action hud_binding_actions[]={
    NativeInputBindings::Action::pistol,NativeInputBindings::Action::shotgun,
    NativeInputBindings::Action::taser,
    NativeInputBindings::Action::molotov
};
static constexpr unsigned hud_binding_textures[]={10,11,12,15};
static ComPtr<ID3D11ShaderResourceView> hud_keycap(NativeRenderer &renderer,unsigned key,const char *caption){
    const uint32_t width=caption?384u:128u;std::vector<uint32_t> pixels(width*128);
    const unsigned artwork_key=key==NativeInputBindings::wheel_up?0x100u:key==NativeInputBindings::wheel_down?0x101u:key;
    const auto label=NativeInputBindings::key_name(key);
    if(!native_hud_keycap(pixels.data(),artwork_key,label.c_str(),caption))throw std::runtime_error("Key artwork creation failed");
    UCTextureAsset cap{};cap.format=UC_TEXTURE_BGRA8_UNORM;cap.width=width;cap.height=128;cap.mip_count=1;
    cap.storage=(uint8_t*)pixels.data();cap.storage_bytes=pixels.size()*sizeof(uint32_t);
    cap.mips[0]={width,128,width*4,width*128*4,(const uint8_t*)pixels.data()};return renderer.upload(cap);
}
void NativeHud::load_inventory(const std::filesystem::path &game,NativeRenderer &renderer){
    static const char *archives[]={"pistol.lst","policeshotgun.lst","taser.lst","1-01.ent"};
    static const char *names[]={"hud\\hud_weapons_o_pistol","hud\\hud_weapons_o_policeshotgun","hud\\hud_weapons_o_taser","hud\\hud_weapons_o_molotov"};
    static const char *sh[]={"d79d5ab98737640be65f6d78f58b9a6e17e9fc0d88aedc3d43d03c78fb0843f7",
        "06b6a1bd4f54ea46487d5ce65756f406be74017eb3a998f398e49fb1084fd0d3","37357e51132a82131357c6f557d796016d444c428e52030376e3a9f4b8cb1ab8",
        "5e99a55d60219fa8120a5e7be9d21166a27b3ed45a246e1e6996a2a93e4ec360"};
    static const char *th[]={"1dfa6528732820121b40c58a40e207d32af270b9552d1c8e4cfcf9f6d0cbf9e7",
        "fa3f8857fdcf3404231c2f637ba4a6d2b9f7f7a69b49f2948570f7fb6b58218b","f6d2c5712fa48cfa0859dedc5414b7be2f4f9830affbc8efb813bd9265153c6a",
        "2d49fa76c5b43005142d2239d0d157247aa9aa49eb94ae1b9e13ba131aad51c9"};
    std::array<ComPtr<ID3D11ShaderResourceView>,9> loaded;
    for(unsigned i=0;i<4;++i){
        uint8_t hash[32];auto archive=read_archive(game/archives[i],hash);BytesOwner shader,bytes;
        checked_resource(archive.get(),(std::string(names[i])+".shader").c_str(),sh[i],shader);
        checked_resource(archive.get(),(std::string(names[i])+".tga=nomipmap").c_str(),th[i],bytes);
        struct Texture {UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} texture;
        if(uc_texture_decode(bytes.value.data,bytes.value.size,&texture.value)!=UC_TEXTURE_OK)
            throw std::runtime_error("Invalid equipment HUD texture");
        loaded[i==3?7:i]=renderer.upload(texture.value);
    }
    const NativeInputBindings defaults;std::array<unsigned,4> keys;
    for(unsigned i=0;i<keys.size();++i){
        keys[i]=defaults.key(hud_binding_actions[i]);
        loaded[hud_binding_textures[i]-7]=hud_keycap(renderer,keys[i],nullptr);
    }
    for(unsigned i=0;i<9;++i)textures[7+i]=std::move(loaded[i]);
    binding_keys_=keys;
}
void NativeHud::update_bindings(NativeRenderer &renderer,const NativeInputBindings &bindings){
    if(!textures[7])return;
    std::array<ComPtr<ID3D11ShaderResourceView>,4> changed;std::array<unsigned,4> keys;
    for(unsigned i=0;i<keys.size();++i){
        keys[i]=bindings.key(hud_binding_actions[i]);
        if(keys[i]!=binding_keys_[i])changed[i]=hud_keycap(renderer,keys[i],nullptr);
    }
    for(unsigned i=0;i<keys.size();++i)if(changed[i])textures[hud_binding_textures[i]]=std::move(changed[i]);
    binding_keys_=keys;
}
void NativeHud::initialize_health(float current_health,uint32_t mode,uint32_t tier,float scale) {
    UCHudHealth next;
    if(!uc_hud_health_initialize(&next,mode,tier,scale)||!uc_hud_health_step(&next,current_health,mode,tier,0))
        throw std::invalid_argument("Invalid native health initialization");
    health=next;health_mode=mode;health_tier=tier;health_initialized=true;
}
void NativeHud::update_health(float current_health,float delta) {
    if(!health_initialized||!uc_hud_health_step(&health,current_health,health_mode,health_tier,delta))throw std::invalid_argument("Invalid native health update");
}
static void draw_quads(NativeHud &hud,NativeRenderer &renderer,const NativeHudAmmo &result,bool right=false) {
    if(!result.count)return;
    const auto &quads=result.quads;
    if(!hud.mesh.vertices)throw std::logic_error("Native HUD assets were not loaded");
    /* PC widescreen: preserve artwork proportions and anchor each corner.
     * Convert D3D8 integer centers to D3D11 half-integer centers exactly once. */
    const float scale=std::min(renderer.width/640.f,renderer.height/480.f);
    const float ox=right?renderer.width-640*scale:0,oy=(renderer.height-480*scale)*.5f;
    NativeVertex vertices[32]{};
    for(size_t i=0;i<result.count;++i) {
        const auto &q=quads[i];const float x=ox+(q.x+.5f)*scale,y=oy+(q.y+.5f)*scale;
        const float xy[4][2]={{x,y},{x+q.width*scale,y},{x+q.width*scale,y+q.height*scale},{x,y+q.height*scale}};
        const float uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
        for(unsigned j=0;j<4;++j){auto &v=vertices[i*4+j];v.position[0]=xy[j][0];v.position[1]=xy[j][1];v.position[2]=0;v.color=q.color;memcpy(v.uv,uv[j],8);}
    }
    renderer.update(hud.mesh,vertices,32);
    const float matrix[16]={2.f/renderer.width,0,0,0,0,-2.f/renderer.height,0,0,0,0,1,0,-1,1,0,1};
    const float white[4]={1,1,1,1};NativeDrawState state;
    state.clamp_u=state.clamp_v=state.point_filter=state.depth_disabled=true;
    state.blend_mode=1;state.vertex_exposure=true;state.write_alpha=false; /* RGBUI parent; absent exposure owner: white119 */
    for(size_t i=0;i<result.count;) {
        size_t end=i+1;while(end<result.count&&quads[end].texture==quads[i].texture)++end;
        state.point_filter=state.clamp_u=state.clamp_v=quads[i].texture!=6; // authored frame; ordinary inherited fill sampler.
        renderer.draw(hud.mesh,matrix,white,hud.textures[quads[i].texture].Get(),(uint32_t)i*6,(uint32_t)(end-i)*6,state);i=end;
    }
}
void NativeHud::draw(NativeRenderer &renderer,const UCMainplayerState &s,const UCMainplayerItemSpec &spec) {
    const auto health_quads=health_initialized?native_hud_health(health):NativeHudAmmo{},ammo_quads=native_hud_ammo(s,spec);
    draw_quads(*this,renderer,health_quads);draw_quads(*this,renderer,ammo_quads,true);
    if(textures[7])draw_quads(*this,renderer,native_hud_inventory(s),true);

}
