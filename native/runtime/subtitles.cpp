#include "subtitles.h"
#include "mission_character.h"
#include "player_mission.h"
extern "C" {
#include "../texture_asset/texture_asset.h"
#include "../subtitle_policy.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
void NativeSubtitles::load(const std::filesystem::path&game,NativeRenderer&r){
    uint8_t hash[32];auto fonts=read_archive(game/"dialoguefonts.lst",hash);
    checked_resource(fonts.get(),"fonts\\small.abc","afb8cc9e0083ee773ee61057c2b5e27fd89c8adf3870d63b900d1dcb14cd01ad",metrics_);
    if(uc_font_read(metrics_.value.data,metrics_.value.size,&font_))throw std::runtime_error("Subtitle font metrics");
    auto art=read_archive(game/"PersistantFilesXBox.lst",hash);BytesOwner shader,bytes;
    checked_resource(art.get(),"fonts\\small.shader","0c00c756df416d5d7312fe168a3132d7f1152423c58a633bb399e6a70579e208",shader);
    checked_resource(art.get(),"fonts\\small%4b%.tga","19496ff6ff46c36474827d94087e70ef8a496020f439b8e301c1ccfcc25ae8a0",bytes);
    struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}} texture;
    if(uc_texture_decode(bytes.value.data,bytes.value.size,&texture.value))throw std::runtime_error("Subtitle font texture");
    texture_=r.upload(texture.value);renderer_=&r;
}
int NativeSubtitles::add(const NativeDialogueLine&line,NativeMissionCharacter&actor,NativeMissionWorld&world,NativeMissionPlayer&player,int32_t*handle){
    auto*d=actor.dialogue();if(!handle||!d||!texture_||line.variants.empty()||d->subtitle_type()>2)return 1;
    int32_t kind=0,dispatch=0;
    if(d->subtitle_type()==1){int found=actor.property("character type",&kind);if(found<0)return 1;
        if(kind<1||kind>5){found=actor.property("is dispatch",&dispatch);if(found<0)return 1;}}
    return add_row(line,*d,actor.entity.id,actor.entity.transform+12,kind,dispatch,world,player,handle);
}
int NativeSubtitles::add_dispatch(const NativeDialogueLine&line,NativeCharacterDialogue&d,int32_t id,const float*position,int kind,NativeMissionWorld&w,NativeMissionPlayer&p,int32_t*out){
    return add_row(line,d,id,position,kind,1,w,p,out);
}
int NativeSubtitles::add_row(const NativeDialogueLine&line,NativeCharacterDialogue&dialogue,int32_t id,const float*position,int kind,int dispatch,NativeMissionWorld&world,NativeMissionPlayer&player,int32_t*handle){
    auto*d=&dialogue;if(!handle||!texture_||line.variants.empty()||d->subtitle_type()>2)return 1;
    const int32_t*ids=nullptr;uint32_t count=0;if(uc_entity_names_view(world.names(),"SUBTITLE",&ids,&count))return 1;
    *handle=0;
    const float*ap=uc_entity_event_lookup(&world.registry(),id)?position:nullptr;
    const float*pp=uc_entity_event_lookup(&world.registry(),player.entity.id)?player.entity.view.transform+12:nullptr;
    if(!uc_subtitle_admit(1,count!=0,line.always_subtitle,pp,ap))return 0;
    //7D000 caller21A9C0 explicitly passes variant0, independent of audio choice.
    Row row{next_,id,d,line.variants[0].text,{1,1,1,1},1,{}};
    uc_subtitle_color(d->subtitle_type(),kind,dispatch,row.color);
    std::vector<UCFontQuad> quads(row.text.size()+1);size_t n=0;float height=0;
    if(uc_font_layout(&font_,row.text.c_str(),50,0,1,50,435,0,quads.data(),quads.size(),&n,&height)||next_==INT32_MAX)return 1;
    row.lines=1+unsigned(std::lround(height/font_.line_height));
    if(n){
        std::vector<NativeVertex> vertices(n*4);std::vector<uint32_t> indices(n*6);
        for(size_t i=0;i<n;i++){
            const auto&q=quads[i];const float xy[4][2]={{q.x0,q.y0},{q.x1,q.y0},{q.x1,q.y1},{q.x0,q.y1}};
            const float uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
            for(unsigned j=0;j<4;j++){auto&v=vertices[i*4+j];v.position[0]=xy[j][0]+.5f;v.position[1]=xy[j][1]+.5f;std::memcpy(v.uv,uv[j],8);}
            const uint32_t local[]={0,1,2,0,2,3};for(unsigned j=0;j<6;j++)indices[i*6+j]=uint32_t(i)*4+local[j];
        }
        row.mesh=renderer_->upload(vertices.data(),vertices.size(),indices.data(),indices.size());
    }
    rows_.push_back(std::move(row));*handle=next_++;return 0;
}
void NativeSubtitles::remove_actor(int32_t id){
    rows_.erase(std::remove_if(rows_.begin(),rows_.end(),[&](const Row&r){return r.actor==id;}),rows_.end());
}
void NativeSubtitles::step(NativeMissionWorld&world){
    rows_.erase(std::remove_if(rows_.begin(),rows_.end(),[&](const Row&r){return !uc_entity_event_lookup(&world.registry(),r.actor)||r.dialogue->subtitle_ready(r.handle);}),rows_.end());
}
void NativeSubtitles::draw(NativeRenderer&r,bool enabled){
    if(!enabled||rows_.empty())return;
    unsigned lines=0;for(const auto&row:rows_)lines+=row.lines;
    float y=404.f-float(double(lines-1)*font_.line_height)-2.f;
    const float scale=std::min(r.width/640.f,r.height/480.f),ox=(r.width-640*scale)*.5f,oy=(r.height-480*scale)*.5f;
    float matrix[16]={2.f*scale/r.width,0,0,0,0,-2.f*scale/r.height,0,0,0,0,1,0,-1+2.f*ox/r.width,1,0,1};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;state.vertex_exposure=true;
    //Exact small.shader: SRC_ALPHA/INV_SRC_ALPHA, exposure vertex, ordinary
    //bilinear wrap sampler; RGBUI parent disables depth and alpha writes.
    for(const auto&row:rows_){
        matrix[13]=1-2.f*(oy+y*scale)/r.height;
        if(row.mesh.vertices)r.draw(row.mesh,matrix,row.color,texture_.Get(),0,0,state);
        y+=row.lines*font_.line_height;
    }
}
void NativeSubtitles::draw_fps(NativeRenderer&r,float fps){
    if(!texture_||!std::isfinite(fps)||fps<0)throw std::runtime_error("FPS display input");
    fps=std::round(std::min(fps,9999.f)*10)/10;
    if(fps!=fps_value_){
        char text[24];std::snprintf(text,sizeof text,"%.1f FPS",fps);UCFontQuad quads[24];size_t n=0;float height;
        if(uc_font_layout(&font_,text,0,0,1,0,640,0,quads,24,&n,&height))throw std::runtime_error("FPS text layout");
        NativeVertex vertices[96]{};uint32_t indices[144];const uint32_t order[]={0,1,2,0,2,3};fps_width_=0;
        for(unsigned i=0;i<24;i++)for(unsigned j=0;j<6;j++)indices[i*6+j]=i*4+order[j];
        for(size_t i=0;i<n;i++){const auto&q=quads[i];fps_width_=std::max(fps_width_,q.x1);
            const float xy[4][2]={{q.x0,q.y0},{q.x1,q.y0},{q.x1,q.y1},{q.x0,q.y1}},uv[4][2]={{q.u0,q.v0},{q.u1,q.v0},{q.u1,q.v1},{q.u0,q.v1}};
            for(unsigned j=0;j<4;j++){auto&v=vertices[i*4+j];v.position[0]=xy[j][0]+.5f;v.position[1]=xy[j][1]+.5f;std::memcpy(v.uv,uv[j],8);}}
        if(!fps_mesh_.vertices)fps_mesh_=r.upload(vertices,96,indices,144,true);else r.update(fps_mesh_,vertices,96);
        fps_indices_=uint32_t(n*6);fps_value_=fps;
    }
    const float scale=r.height/720.f,margin=16*scale;
    const float matrix[16]={2*scale/r.width,0,0,0,0,-2*scale/r.height,0,0,0,0,1,0,1-2*(margin+fps_width_*scale)/r.width,1-2*margin/r.height,0,1};
    const float white[]={1.f,1.f,1.f,1.f};NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;state.vertex_exposure=true;
    r.draw(fps_mesh_,matrix,white,texture_.Get(),0,fps_indices_,state);
}
