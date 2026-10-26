// Real ImGui atlas/draw-list fixture; synthetic geometry, no GPU or game launch.
#include "xbe_coop_preview.h"
#include "xbe_coop_preview_assets.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

static unsigned load_calls=0;
namespace trial_coop_preview {
bool Load(const std::filesystem::path& path,unsigned character,Model& out,std::string& error){
    ++load_calls;
    if(path=="missing"&&character==2){error="Missing fixture";return false;}
    Model m;
    Image image;image.width=image.height=2;
    const uint8_t alpha[]={0,126,127,200};
    for(unsigned i=0;i<4;++i){image.rgba.push_back(uint8_t(50+character*20));image.rgba.push_back(70);image.rgba.push_back(90);image.rgba.push_back(alpha[i]);}
    m.images.push_back(image);for(unsigned i=0;i<4;++i)image.rgba[i*4]=200;m.images.push_back(image);
    for(unsigned triangle=0;triangle<3;++triangle){
        const float z=triangle==0?.6f:triangle==1?-.6f:0;
        const float positions[3][2]={{-.5f,-1},{.5f,-1},{0,1}};
        for(const auto& position:positions){
            Vertex v{};v.position[0]=position[0];v.position[1]=position[1];v.position[2]=z;
            v.normal[2]=1;v.uv[0]=.5f;v.uv[1]=.5f;v.color=triangle==1?0xff10ff30:0x80102030;m.vertices.push_back(v);
        }
        Triangle t{};for(unsigned i=0;i<3;++i)t.indices[i]=triangle*3+i;
        t.image=triangle==1?1:0;m.triangles.push_back(t);
    }
    if(path=="bad-index")m.triangles[0].indices[0]=100;
    if(path=="bad-uv")m.vertices[0].uv[0]=std::numeric_limits<float>::quiet_NaN();
    if(path=="large")m.triangles.resize(24000,m.triangles[0]);
    out=std::move(m);return true;
}
}
struct Frame {std::vector<ImDrawVert> vertices;std::vector<ImDrawCmd> commands;unsigned indices=0;};
static Frame Draw(double time,bool two=true){
    ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({640,480});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::Begin("preview",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings);
    CHECK(trial_coop_preview::Draw(0,{20,20},{300,420},time));
    if(two)CHECK(trial_coop_preview::Draw(1,{340,20},{620,420},time));
    CHECK(!trial_coop_preview::Draw(5,{20,20},{300,420},time));
    CHECK(!trial_coop_preview::Draw(0,{20,20},{20,420},time));
    CHECK(!trial_coop_preview::Draw(0,{20,20},{300,420},std::numeric_limits<double>::infinity()));
    ImGui::End();ImGui::PopStyleVar();ImGui::Render();
    Frame out;const auto* data=ImGui::GetDrawData();CHECK(data&&data->Valid);
    for(const auto* list:data->CmdLists){
        out.vertices.insert(out.vertices.end(),list->VtxBuffer.begin(),list->VtxBuffer.end());
        for(const auto& command:list->CmdBuffer)if(command.ElemCount){
            out.commands.push_back(command);out.indices+=command.ElemCount;
            CHECK(command.TextureId==ImGui::GetIO().Fonts->TexID);
            for(unsigned i=0;i<command.ElemCount;++i){
                const unsigned index=command.VtxOffset+list->IdxBuffer[command.IdxOffset+i];CHECK(index<unsigned(list->VtxBuffer.Size));
                const auto& v=list->VtxBuffer[index];CHECK(std::isfinite(v.pos.x)&&std::isfinite(v.pos.y));
                CHECK(v.pos.x>=command.ClipRect.x&&v.pos.x<=command.ClipRect.z&&v.pos.y>=command.ClipRect.y&&v.pos.y<=command.ClipRect.w);
                CHECK(v.uv.x>=0&&v.uv.x<=1&&v.uv.y>=0&&v.uv.y<=1);
            }
        }
    }
    return out;
}
static void Build(const char* path){
    auto* atlas=ImGui::GetIO().Fonts;atlas->Clear();trial_coop_preview::Reset();atlas->SetTexID(0);atlas->AddFontDefault();
    CHECK(trial_coop_preview::Initialize(atlas,path));CHECK(atlas->Build());CHECK(trial_coop_preview::Finalize());
    atlas->SetTexID((ImTextureID)(intptr_t)1);
}
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={640,480};io.DeltaTime=1.f/60;
    io.BackendFlags|=ImGuiBackendFlags_RendererHasVtxOffset;
    auto* atlas=io.Fonts;atlas->AddFontDefault();
    CHECK(!trial_coop_preview::Initialize(atlas,"missing"));CHECK(atlas->CustomRects.Size==0);
    CHECK(!trial_coop_preview::Initialize(atlas,"bad-index"));CHECK(atlas->CustomRects.Size==0);
    CHECK(!trial_coop_preview::Initialize(atlas,"bad-uv"));CHECK(atlas->CustomRects.Size==0);
    CHECK(trial_coop_preview::Initialize(atlas,"normal"));const unsigned calls=load_calls;
    CHECK(trial_coop_preview::Initialize(atlas,"normal"));CHECK(load_calls==calls);
    CHECK(atlas->CustomRects.Size==10);const int font_pixels=atlas->AddCustomRectRegular(3,3);CHECK(atlas->Build());
    unsigned char* pixels=nullptr;int width=0,height=0;atlas->GetTexDataAsRGBA32(&pixels,&width,&height);
    const auto* font_rect=atlas->GetCustomRectByIndex(font_pixels);
    auto* sentinel=pixels+(size_t(font_rect->Y)*width+font_rect->X)*4;
    sentinel[0]=17;sentinel[1]=34;sentinel[2]=51;sentinel[3]=68;
    CHECK(trial_coop_preview::Finalize());CHECK(sentinel[0]==17&&sentinel[1]==34&&sentinel[2]==51&&sentinel[3]==68);
    for(unsigned image=0;image<2;++image){
        const auto* rect=atlas->GetCustomRectByIndex(image);const uint8_t alpha[]={0,126,127,200};
        CHECK(rect->Width==6&&rect->Height==6);
        for(unsigned y=0;y<6;++y)for(unsigned x=0;x<6;++x){
            const auto* p=pixels+(size_t(rect->Y+y)*width+rect->X+x)*4;
            const unsigned source=(y<3?0:2)+(x<3?0:1);CHECK(p[0]==(image?200:50)&&p[1]==70&&p[2]==90);
            CHECK(p[3]==alpha[source]);
        }
    }
    atlas->SetTexID((ImTextureID)(intptr_t)1);CHECK(!trial_coop_preview::Finalize());
    auto first=Draw(0);CHECK(first.vertices.size()==18&&first.indices==18&&first.commands.size()==2);
    CHECK(first.commands[0].ClipRect.x==20&&first.commands[0].ClipRect.z==300);
    CHECK(first.commands[1].ClipRect.x==340&&first.commands[1].ClipRect.z==620);
    CHECK(((first.vertices[0].col>>IM_COL32_G_SHIFT)&255)==255&&((first.vertices[0].col>>IM_COL32_R_SHIFT)&255)<255);
    CHECK(((first.vertices[6].col>>IM_COL32_A_SHIFT)&255)==128);
    const unsigned order[]={1,0,0};
    for(unsigned triangle=0;triangle<3;++triangle){
        const auto* rect=atlas->GetCustomRectByIndex(order[triangle]);const auto& v=first.vertices[triangle*3];
        CHECK(std::abs(v.uv.x-float(rect->X+3)/width)<1e-6f&&std::abs(v.uv.y-float(rect->Y+3)/height)<1e-6f);
    }
    auto rotated=Draw(5);bool changed=false;
    for(size_t i=0;i<first.vertices.size();++i)changed|=std::abs(first.vertices[i].pos.x-rotated.vertices[i].pos.x)>.1f;
    CHECK(changed);auto full=Draw(20);CHECK(full.vertices.size()==first.vertices.size());
    for(size_t i=0;i<first.vertices.size();++i){CHECK(first.vertices[i].pos.x==full.vertices[i].pos.x);CHECK(first.vertices[i].col==full.vertices[i].col);}
    CHECK(load_calls==calls);
    Build("large");auto large=Draw(1,false);CHECK(large.vertices.size()==72000&&large.indices==72000);
    bool offset=false;for(const auto& command:large.commands)offset|=command.VtxOffset>0;CHECK(offset);
    Build("normal");CHECK(Draw(0).vertices.size()==18);
    atlas->SetTexID(0);atlas->Clear();CHECK(!trial_coop_preview::Finalize());
    trial_coop_preview::Reset();ImGui::DestroyContext();
    std::puts("co-op previews: atlas padding/fractional alpha, actual ImGui triangles, rotation, depth order, two panels, clipping, index rollover and rebuild passed");
}
