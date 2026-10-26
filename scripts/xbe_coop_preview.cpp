#include "xbe_coop_preview.h"
#include "xbe_coop_preview_assets.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <numeric>

namespace trial_coop_preview {
namespace {
constexpr uint32_t marker=0x55435000;
constexpr size_t pixel_limit=8u<<20;
struct Projected {ImVec2 position;float depth,light;};
struct Cell {int index=-1;ImVec2 origin,scale;};
struct Preview {
    Model model;
    std::vector<Cell> cells;
    std::vector<Projected> projected;
    std::vector<uint32_t> order;
    float center[3]{},height=0,radius=0;
};
struct State {ImFontAtlas* atlas=nullptr;std::array<Preview,5> models;bool ready=false;};
State state;

bool Valid(Preview& preview,size_t& pixels){
    const auto& m=preview.model;
    if(m.vertices.empty()||m.vertices.size()>120000||m.triangles.empty()||m.triangles.size()>60000||
       m.images.empty()||m.images.size()>MaxImages)return false;
    preview.cells.resize(m.images.size());
    std::vector<bool> used(m.images.size());
    for(const auto& triangle:m.triangles){
        if(triangle.image>=m.images.size())return false;
        used[triangle.image]=true;
        for(auto index:triangle.indices)if(index>=m.vertices.size())return false;
    }
    for(size_t i=0;i<m.images.size();++i){
        const auto& image=m.images[i];
        if(!image.width||!image.height||image.width>MaxImageDimension||image.height>MaxImageDimension||
           image.rgba.size()!=size_t(image.width)*image.height*4)return false;
        if(used[i]){
            pixels+=size_t(image.width+4)*(image.height+4);
            if(pixels>pixel_limit)return false;
            preview.cells[i].index=-2;
        }
    }
    float low[3],high[3];std::copy_n(m.vertices[0].position,3,low);std::copy_n(low,3,high);
    for(const auto& vertex:m.vertices){
        for(unsigned axis=0;axis<3;++axis){
            if(!std::isfinite(vertex.position[axis])||std::abs(vertex.position[axis])>1e6f||
               !std::isfinite(vertex.normal[axis])||std::abs(vertex.normal[axis])>1e6f)return false;
            low[axis]=std::min(low[axis],vertex.position[axis]);high[axis]=std::max(high[axis],vertex.position[axis]);
        }
        for(float uv:vertex.uv)if(!std::isfinite(uv)||uv<0||uv>1)return false;
    }
    for(unsigned axis=0;axis<3;++axis)preview.center[axis]=(low[axis]+high[axis])*.5f;
    preview.height=high[1]-low[1];
    for(const auto& vertex:m.vertices)preview.radius=std::max(preview.radius,
        std::hypot(vertex.position[0]-preview.center[0],vertex.position[2]-preview.center[2]));
    if(preview.height<.0001f||preview.radius<.0001f)return false;
    preview.projected.resize(m.vertices.size());preview.order.resize(m.triangles.size());
    std::iota(preview.order.begin(),preview.order.end(),0u);return true;
}
bool Owned(){
    if(!state.atlas||!ImGui::GetCurrentContext()||ImGui::GetIO().Fonts!=state.atlas)return false;
    for(const auto& preview:state.models)for(size_t i=0;i<preview.cells.size();++i){
        const auto& cell=preview.cells[i];if(cell.index>=0){
            if(cell.index>=state.atlas->CustomRects.Size)return false;
            const auto* rect=state.atlas->GetCustomRectByIndex(cell.index);
            const auto& image=preview.model.images[i];
            if(rect->GlyphID!=marker+uint32_t(cell.index)||rect->Width!=image.width+4||rect->Height!=image.height+4)return false;
        }
    }
    return !state.models[0].model.vertices.empty();
}
ImU32 Color(uint32_t argb,float light){
    // Source RGB is prelight; keep brighter authored channels under preview light.
    auto channel=[&](unsigned shift){return int(std::max(float((argb>>shift)&255),light*255.f)+.5f);};
    return IM_COL32(channel(16),channel(8),channel(0),argb>>24);
}
}

void Reset(){state={};}
bool Initialize(ImFontAtlas* atlas,const std::filesystem::path& game){
    if(state.atlas==atlas&&Owned())return true;
    if(!atlas||atlas->Locked||atlas->TexID)return false;
    Reset();
    try{
        std::array<Preview,5> models;size_t pixels=0;std::string error;
        for(unsigned i=0;i<models.size();++i)
            if(!Load(game,i,models[i].model,error)||!Valid(models[i],pixels))return false;
        state.models=std::move(models);state.atlas=atlas;
        atlas->TexDesiredWidth=std::max(atlas->TexDesiredWidth,4096);
        for(auto& preview:state.models)for(size_t i=0;i<preview.cells.size();++i){
            auto& cell=preview.cells[i];if(cell.index==-2){
                const auto& image=preview.model.images[i];
                cell.index=atlas->AddCustomRectRegular(image.width+4,image.height+4);
                atlas->GetCustomRectByIndex(cell.index)->GlyphID=marker+uint32_t(cell.index);
            }
        }
        return true;
    }catch(const std::exception&){Reset();return false;}
}
bool Finalize(){
    if(!Owned()||state.atlas->Locked||state.atlas->TexID)return false;
    state.ready=false;
    unsigned char* pixels=nullptr;int width=0,height=0;
    state.atlas->GetTexDataAsRGBA32(&pixels,&width,&height);
    if(!pixels||width<=0||height<=0)return false;
    for(auto& preview:state.models)for(size_t i=0;i<preview.cells.size();++i){
        auto& cell=preview.cells[i];if(cell.index<0)continue;
        const auto* rect=state.atlas->GetCustomRectByIndex(cell.index);
        const auto& image=preview.model.images[i];
        if(!rect->IsPacked()||unsigned(rect->X)+rect->Width>unsigned(width)||unsigned(rect->Y)+rect->Height>unsigned(height))return false;
        for(unsigned y=0;y<rect->Height;++y)for(unsigned x=0;x<rect->Width;++x){
            const unsigned sx=unsigned(std::clamp(int(x)-2,0,int(image.width)-1));
            const unsigned sy=unsigned(std::clamp(int(y)-2,0,int(image.height)-1));
            const auto* source=image.rgba.data()+(size_t(sy)*image.width+sx)*4;
            auto* target=pixels+(size_t(rect->Y+y)*width+rect->X+x)*4;
            std::memcpy(target,source,4);
        }
        cell.origin={float(rect->X+2)/width,float(rect->Y+2)/height};
        cell.scale={float(image.width)/width,float(image.height)/height};
    }
    state.atlas->TexPixelsUseColors=true;state.ready=true;return true;
}
bool Draw(unsigned character,ImVec2 minimum,ImVec2 maximum,double elapsed_seconds){
    if(!state.ready||!Owned()||!state.atlas->TexID||character>=state.models.size()||
       !std::isfinite(minimum.x)||!std::isfinite(minimum.y)||!std::isfinite(maximum.x)||!std::isfinite(maximum.y)||
       maximum.x<=minimum.x||maximum.y<=minimum.y||!std::isfinite(elapsed_seconds))return false;
    auto& preview=state.models[character];const auto& model=preview.model;
    const float angle=float(std::fmod(std::max(0.,elapsed_seconds),20.)*6.283185307179586/20.);
    const float cosine=std::cos(angle),sine=std::sin(angle);
    const float scale=.9f*std::min((maximum.x-minimum.x)/(2*preview.radius),(maximum.y-minimum.y)/preview.height);
    const ImVec2 center{(minimum.x+maximum.x)*.5f,(minimum.y+maximum.y)*.5f};
    for(size_t i=0;i<model.vertices.size();++i){
        const auto& vertex=model.vertices[i];
        const float x=vertex.position[0]-preview.center[0],y=vertex.position[1]-preview.center[1],z=vertex.position[2]-preview.center[2];
        const float nx=vertex.normal[0],ny=vertex.normal[1],nz=vertex.normal[2];
        const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
        const float facing=length>.00001f?((nx*cosine+nz*sine)*-.3f+ny*.5f+(-nx*sine+nz*cosine)*.8124f)/length:0;
        preview.projected[i]={{center.x+(x*cosine+z*sine)*scale,center.y-y*scale},-x*sine+z*cosine,
            .65f+.35f*std::clamp(facing,0.f,1.f)};
    }
    auto depth=[&](uint32_t index){const auto& t=model.triangles[index];return preview.projected[t.indices[0]].depth+
        preview.projected[t.indices[1]].depth+preview.projected[t.indices[2]].depth;};
    // shortcut: average-depth sorting can overlap intersecting faces; use a depth-buffer pass if real previews show artifacts.
    std::sort(preview.order.begin(),preview.order.end(),[&](uint32_t a,uint32_t b){const auto da=depth(a),db=depth(b);return da==db?a<b:da<db;});
    auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(minimum,maximum,true);draw->PushTextureID(state.atlas->TexID);
    for(auto index:preview.order){
        const auto& triangle=model.triangles[index];const auto& cell=preview.cells[triangle.image];
        draw->PrimReserve(3,3);
        for(auto vertex_index:triangle.indices){
            const auto& vertex=model.vertices[vertex_index];const auto& projected=preview.projected[vertex_index];
            draw->PrimVtx(projected.position,{cell.origin.x+vertex.uv[0]*cell.scale.x,cell.origin.y+vertex.uv[1]*cell.scale.y},Color(vertex.color,projected.light));
        }
    }
    draw->PopTextureID();draw->PopClipRect();return true;
}
}
