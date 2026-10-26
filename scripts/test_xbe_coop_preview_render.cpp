// Offline pixels from production ImGui triangles and the user's original models.
#include "xbe_coop_preview.h"
#include "xbe_frontend_fonts.h"
#include "xbe_hd_ui.h"
#include "kernel/disc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)

static bool hd_selected=false;
namespace trial_graphics {bool HdUi(){return hd_selected;}}
namespace kernel {
const Disc* MountedDisc(){return nullptr;}
const Disc::Entry* Disc::Find(std::string_view)const{return nullptr;}
bool Disc::Load(std::string_view,std::vector<uint8_t>&)const{return false;}
}
namespace {
constexpr unsigned width=1600,height=900;
using Pixel=std::array<float,4>;
float Edge(ImVec2 a,ImVec2 b,ImVec2 p){return (p.x-a.x)*(b.y-a.y)-(p.y-a.y)*(b.x-a.x);}
Pixel Sample(const unsigned char* atlas,int w,int h,ImVec2 uv){
    const float x=std::clamp(uv.x*w-.5f,0.f,float(w-1)),y=std::clamp(uv.y*h-.5f,0.f,float(h-1));
    const int x0=int(x),y0=int(y),x1=std::min(x0+1,w-1),y1=std::min(y0+1,h-1);
    const float fx=x-x0,fy=y-y0;Pixel out{};
    for(unsigned lane=0;lane<4;++lane){
        const float a=atlas[(size_t(y0)*w+x0)*4+lane],b=atlas[(size_t(y0)*w+x1)*4+lane];
        const float c=atlas[(size_t(y1)*w+x0)*4+lane],d=atlas[(size_t(y1)*w+x1)*4+lane];
        out[lane]=((a+(b-a)*fx)*(1-fy)+(c+(d-c)*fx)*fy)/255.f;
    }
    return out;
}
void Raster(std::vector<Pixel>& pixels,const ImDrawVert& a,const ImDrawVert& b,const ImDrawVert& c,
            ImVec4 clip,const unsigned char* atlas,int w,int h){
    const float area=Edge(a.pos,b.pos,c.pos);if(std::abs(area)<1e-6f)return;
    const int x0=std::max(0,int(std::ceil(std::max(clip.x,std::min({a.pos.x,b.pos.x,c.pos.x}))-.5f)));
    const int y0=std::max(0,int(std::ceil(std::max(clip.y,std::min({a.pos.y,b.pos.y,c.pos.y}))-.5f)));
    const int x1=std::min(int(width)-1,int(std::floor(std::min(clip.z,std::max({a.pos.x,b.pos.x,c.pos.x}))-.5f)));
    const int y1=std::min(int(height)-1,int(std::floor(std::min(clip.w,std::max({a.pos.y,b.pos.y,c.pos.y}))-.5f)));
    const unsigned shifts[]={IM_COL32_R_SHIFT,IM_COL32_G_SHIFT,IM_COL32_B_SHIFT,IM_COL32_A_SHIFT};
    for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
        const ImVec2 p{float(x)+.5f,float(y)+.5f};
        const float wa=Edge(b.pos,c.pos,p)/area,wb=Edge(c.pos,a.pos,p)/area,wc=1-wa-wb;
        if(wa<0||wb<0||wc<0)continue;
        const ImVec2 uv{wa*a.uv.x+wb*b.uv.x+wc*c.uv.x,wa*a.uv.y+wb*b.uv.y+wc*c.uv.y};
        auto source=Sample(atlas,w,h,uv);
        for(unsigned lane=0;lane<4;++lane)source[lane]*=(wa*((a.col>>shifts[lane])&255)+wb*((b.col>>shifts[lane])&255)+wc*((c.col>>shifts[lane])&255))/255.f;
        auto& target=pixels[size_t(y)*width+x];
        for(unsigned lane=0;lane<3;++lane)target[lane]=source[lane]*source[3]+target[lane]*(1-source[3]);
        target[3]=1;
    }
}
bool Write(const std::filesystem::path& path,const std::vector<Pixel>& pixels){
    std::ofstream file(path,std::ios::binary);if(!file)return false;
    const auto word=[&](uint16_t v){const char b[]={char(v),char(v>>8)};file.write(b,2);};
    const auto dword=[&](uint32_t v){const char b[]={char(v),char(v>>8),char(v>>16),char(v>>24)};file.write(b,4);};
    word(0x4d42);dword(54+width*height*4);dword(0);dword(54);dword(40);dword(width);dword(height);word(1);word(32);
    dword(0);dword(width*height*4);dword(0);dword(0);dword(0);dword(0);
    for(unsigned y=height;y--;)for(unsigned x=0;x<width;++x){
        const auto& p=pixels[size_t(y)*width+x];
        const auto byte=[](float v){return char(std::clamp(v*255.f+.5f,0.f,255.f));};
        const char bgra[]={byte(p[2]),byte(p[1]),byte(p[0]),char(255)};file.write(bgra,4);
    }
    return bool(file);
}
}
int main(int argc,char** argv){
    CHECK(argc==3);ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={float(width),float(height)};io.DeltaTime=1.f/60;io.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset;
    CHECK(trial_coop_preview::Initialize(io.Fonts,std::filesystem::path(argv[1])));
    const int preview_rects=io.Fonts->CustomRects.Size;ImFont *body=nullptr,*title=nullptr;
    CHECK(TrialLoadFrontendFonts(io.Fonts,std::filesystem::path(argv[1]),body,title));
    CHECK(uc_hd_ui::Ready()&&uc_hd_ui::Font("medium")&&uc_hd_ui::Font("large")&&uc_hd_ui::Buttons());
    CHECK(body&&title&&!std::strcmp(body->GetDebugName(),"Urban Chaos medium")&&!std::strcmp(title->GetDebugName(),"Urban Chaos large"));
    hd_selected=true;ImFont* hd_body=TrialFrontendFontForUi(body);ImFont* hd_title=TrialFrontendFontForUi(title);
    CHECK(hd_body!=body&&hd_title!=title&&hd_body!=hd_title);
    CHECK(!std::strcmp(hd_body->GetDebugName(),"Urban Chaos medium HD")&&!std::strcmp(hd_title->GetDebugName(),"Urban Chaos large HD"));
    hd_selected=false;CHECK(TrialFrontendFontForUi(body)==body&&TrialFrontendFontForUi(title)==title);
    unsigned char* atlas=nullptr;int aw=0,ah=0;io.Fonts->GetTexDataAsRGBA32(&atlas,&aw,&ah);CHECK(atlas&&aw>0&&ah>0);
    struct SavedRect {ImFontAtlasCustomRect rect;std::vector<uint8_t> pixels;};
    std::vector<SavedRect> saved;size_t saved_bytes=0;
    for(int i=preview_rects;i<io.Fonts->CustomRects.Size;++i){
        const auto* rect=io.Fonts->GetCustomRectByIndex(i);CHECK(rect->IsPacked());
        SavedRect copy{*rect,std::vector<uint8_t>(size_t(rect->Width)*rect->Height*4)};
        for(unsigned y=0;y<rect->Height;++y)std::memcpy(copy.pixels.data()+size_t(y)*rect->Width*4,
            atlas+(size_t(rect->Y+y)*aw+rect->X)*4,size_t(rect->Width)*4);
        saved_bytes+=copy.pixels.size();saved.push_back(std::move(copy));
    }
    CHECK(saved.size()>=38&&saved_bytes>1024*1024);
    std::array<std::array<float,4>,2> confirm_uv{};
    for(unsigned mode=0;mode<2;++mode){hd_selected=mode!=0;float uv[4];CHECK(TrialMenuIconUV(0x101,uv));std::copy_n(uv,4,confirm_uv[mode].begin());}
    CHECK(confirm_uv[0]!=confirm_uv[1]);CHECK(trial_coop_preview::Finalize());
    CHECK(atlas==reinterpret_cast<unsigned char*>(io.Fonts->TexPixelsRGBA32)&&aw==io.Fonts->TexWidth&&ah==io.Fonts->TexHeight);
    CHECK(size_t(io.Fonts->CustomRects.Size-preview_rects)==saved.size());
    for(size_t i=0;i<saved.size();++i){
        const auto* rect=io.Fonts->GetCustomRectByIndex(preview_rects+int(i));const auto& copy=saved[i];
        CHECK(rect->X==copy.rect.X&&rect->Y==copy.rect.Y&&rect->Width==copy.rect.Width&&rect->Height==copy.rect.Height);
        for(unsigned y=0;y<rect->Height;++y)CHECK(!std::memcmp(copy.pixels.data()+size_t(y)*rect->Width*4,
            atlas+(size_t(rect->Y+y)*aw+rect->X)*4,size_t(rect->Width)*4));
    }
    for(unsigned mode=0;mode<2;++mode){
        hd_selected=mode!=0;float uv[4];CHECK(TrialMenuIconUV(0x101,uv));CHECK(!std::memcmp(uv,confirm_uv[mode].data(),sizeof uv));
        auto* font=TrialFrontendFontForUi(body);CHECK(font==(mode?hd_body:body)&&font->FindGlyphNoFallback('A'));
        CHECK(TrialFrontendFontForUi(title)==(mode?hd_title:title));
    }
    std::printf("production original/HD font coexistence: %zu font/icon rectangles, %zu pixels bytes preserved\n",saved.size(),saved_bytes);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1);
    ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::Begin("original co-op meshes",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
    const char* names[]={"T-Zero","Avenger","Banger","Magnum","Prophet"};
    for(unsigned row=0;row<2;++row)for(unsigned character=0;character<5;++character){
        const ImVec2 minimum{10.f+character*320,36.f+row*450},maximum{310.f+character*320,440.f+row*450};
        auto* draw=ImGui::GetWindowDrawList();
        draw->AddRectFilled({minimum.x,minimum.y-26},maximum,IM_COL32(25,31,31,255));
        hd_selected=row!=0;
        char label[80];std::snprintf(label,sizeof(label),"%s / %s",names[character],row?"45 DEG / HD":"FRONT / ORIGINAL");
        draw->AddText(TrialFrontendFontForUi(body),14.f,{minimum.x+8,minimum.y-20},IM_COL32(235,238,232,255),label);
        const auto& uv=confirm_uv[row];draw->AddImage(io.Fonts->TexID,{maximum.x-23,minimum.y-23},{maximum.x-3,minimum.y-3},{uv[0],uv[1]},{uv[2],uv[3]});
        CHECK(trial_coop_preview::Draw(character,minimum,maximum,row?2.5:0));
    }
    ImGui::End();ImGui::PopStyleVar();ImGui::Render();
    std::vector<Pixel> pixels(size_t(width)*height,Pixel{.035f,.045f,.045f,1});
    unsigned triangles=0;
    for(const auto* list:ImGui::GetDrawData()->CmdLists)for(const auto& command:list->CmdBuffer){
        CHECK(!command.UserCallback&&command.TextureId==io.Fonts->TexID);
        for(unsigned i=0;i+2<command.ElemCount;i+=3){
            std::array<unsigned,3> indices{};
            for(unsigned corner=0;corner<3;++corner){indices[corner]=command.VtxOffset+list->IdxBuffer[command.IdxOffset+i+corner];CHECK(indices[corner]<unsigned(list->VtxBuffer.Size));}
            Raster(pixels,list->VtxBuffer[indices[0]],list->VtxBuffer[indices[1]],list->VtxBuffer[indices[2]],command.ClipRect,atlas,aw,ah);++triangles;
        }
    }
    CHECK(triangles>1000);
    for(unsigned row=0;row<2;++row)for(unsigned character=0;character<5;++character){
        unsigned covered=0;
        for(unsigned y=36+row*450;y<440+row*450;++y)for(unsigned x=10+character*320;x<310+character*320;++x){
            const auto& p=pixels[size_t(y)*width+x];
            covered+=std::abs(p[0]-25.f/255)+std::abs(p[1]-31.f/255)+std::abs(p[2]-31.f/255)>.02f;
        }
        CHECK(covered>1000);std::printf("%s %s: %u model pixels\n",names[character],row?"quarter":"front",covered);
    }
    CHECK(Write(std::filesystem::path(argv[2]),pixels));
    std::printf("original co-op models: %u ImGui triangles, %dx%d atlas, all five front/quarter views -> %s\n",triangles,aw,ah,argv[2]);
    trial_coop_preview::Reset();ImGui::DestroyContext();
}
