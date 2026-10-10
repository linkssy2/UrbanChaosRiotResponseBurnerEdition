#include "checkpoint_messages.h"
extern "C" {
#include "../checkpoint_message.h"
#include "../texture_asset/texture_asset.h"
}
#include <algorithm>
#include <cstring>
#include <stdexcept>
struct NativeCheckpointMessages::Impl {
    UCCheckpointMessages state{};BytesOwner metrics;UCFont font{};
    ComPtr<ID3D11ShaderResourceView> texture;NativeGpuMesh mesh;std::string text;
};
NativeCheckpointMessages::NativeCheckpointMessages():p_(new Impl){}
NativeCheckpointMessages::~NativeCheckpointMessages()=default;
void NativeCheckpointMessages::load(const std::filesystem::path&game,NativeRenderer&r,const std::string&text){
    auto&s=*p_;if(s.texture)throw std::runtime_error("Checkpoint display already loaded");uint8_t hash[32];
    auto fonts=read_archive(game/"dialoguefonts.lst",hash),art=read_archive(game/"PersistantFilesXBox.lst",hash);
    checked_resource(fonts.get(),"fonts\\large.abc","6e32bbc082dcec904a1fd6a58038a6076022aeaeb4d8bf09add18616b0288892",s.metrics);
    if(uc_font_read(s.metrics.value.data,s.metrics.value.size,&s.font))throw std::runtime_error("Checkpoint font metrics");
    BytesOwner shader,bytes;checked_resource(art.get(),"fonts\\large.shader","5eb4cb2c8d9d5b59bc9223846b3db4c7d2fe27311f6dc4d57c666830f58eac7c",shader);
    checked_resource(art.get(),"fonts\\large%4b%.tga","92e62ec8844cc57571939189d6919abc17de25973b40f8ab4b3252f13359b0df",bytes);
    struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} image;
    if(uc_texture_decode(bytes.value.data,bytes.value.size,&image.value))throw std::runtime_error("Checkpoint font texture");s.texture=r.upload(image.value);
    s.text=text;
    UCCheckpointMessages validate{};uint8_t accepted=0;if(uc_checkpoint_message_add(&validate,s.text.c_str(),&accepted)||!accepted)throw std::runtime_error("Unsupported checkpoint text");
    std::vector<NativeVertex> vertices(128*4*4);std::vector<uint32_t> indices(128*4*6);const uint32_t order[]={0,1,2,0,2,3};
    for(unsigned i=0;i<128*4;i++)for(unsigned j=0;j<6;j++)indices[i*6+j]=i*4+order[j];s.mesh=r.upload(vertices.data(),vertices.size(),indices.data(),indices.size(),true);
}
int NativeCheckpointMessages::add(){return add(p_->text.c_str());}
int NativeCheckpointMessages::add(const char*text){auto&s=*p_;uint8_t accepted=0;return !s.texture?1:uc_checkpoint_message_add(&s.state,text,&accepted);}
int NativeCheckpointMessages::step(float delta){return uc_checkpoint_message_step(&p_->state,delta);}
size_t NativeCheckpointMessages::size()const{return p_->state.count;}
void NativeCheckpointMessages::draw(NativeRenderer&r){
    auto&s=*p_;if(!s.state.count)return;UCCheckpointMessagePlan p{};
    if(!s.texture||uc_checkpoint_message_plan(&s.state,&s.font,640,&p))throw std::runtime_error("Checkpoint display plan");
    UCFontQuad quads[128];size_t count=0;float height=0;
    if(uc_font_layout_flags(&s.font,s.state.rows[0].text,p.x,p.y,p.scale,0,640,4,0,quads,128,&count,&height))throw std::runtime_error("Checkpoint text layout");
    std::vector<NativeVertex> vertices(s.mesh.vertex_count);size_t n=0;
    for(size_t i=0;i<count;i++){UCFontDraw draws[4];size_t total=0;if(uc_font_blur_quad(quads+i,p.blur,0xffffffff,draws,&total))throw std::runtime_error("Checkpoint blur");
        for(size_t k=0;k<total;k++){const auto&q=draws[k].quad;const float xy[4][2]={{q.x0,q.y0},{q.x1,q.y0},{q.x1,q.y1},{q.x0,q.y1}},uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
            for(unsigned j=0;j<4;j++){auto&v=vertices[n++];v.position[0]=xy[j][0]+.5f;v.position[1]=xy[j][1]+.5f;std::memcpy(v.uv,uv[j],8);v.color=draws[k].argb;}}}
    if(!n)return;r.update(s.mesh,vertices.data(),vertices.size());
    const float scale=std::min(r.width/640.f,r.height/480.f),ox=(r.width-640*scale)*.5f,oy=(r.height-480*scale)*.5f;
    const float matrix[16]={2*scale/r.width,0,0,0,0,-2*scale/r.height,0,0,0,0,1,0,-1+2*ox/r.width,1-2*oy/r.height,0,1},white[4]={1,1,1,1};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;state.vertex_exposure=true;
    r.draw(s.mesh,matrix,white,s.texture.Get(),0,uint32_t(n/4*6),state);
}
