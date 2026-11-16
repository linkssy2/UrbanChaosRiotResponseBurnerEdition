#ifdef NDEBUG
#undef NDEBUG
#endif
#include "xbe_weapon_wheel_ui.h"
#include "xbe_weapon_wheel.h"
#include "xbe_coop.h"
#include "imgui.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

namespace {
trial_weapon_wheel::Snapshot snapshots[2];
float aspect=16.f/9.f;
unsigned presentation=1,snapshot_reads=0,icon_reads=0;
bool icons=true;
std::vector<std::string> icon_names;
}
namespace trial_weapon_wheel {Snapshot GetSnapshot(unsigned player){++snapshot_reads;return snapshots[player];}}
namespace trial_graphics {float AspectRatio(){return aspect;}}
namespace trial_language {uint32_t Active(){return 0;}}
unsigned TrialCoopPresentation(){return presentation;}
ImFont* TrialFrontendBodyFont(){return nullptr;}
bool TrialWeaponIconUV(const char* name,float (&uv)[4]) {
    icon_names.emplace_back(name?name:"");
    ++icon_reads;if(!icons)return false;
    uv[0]=uv[1]=.1f;uv[2]=uv[3]=.2f;return true;
}
static trial_weapon_wheel::Snapshot Wheel(unsigned count=8,trial_weapon_wheel::Kind kind=trial_weapon_wheel::Kind::Guns) {
    trial_weapon_wheel::Snapshot s;s.available=s.open=true;s.count=count;s.current=1;s.selected=2;
    s.kind=kind;
    constexpr unsigned guns[]={2,17,20,10,24,3,4,5},tools[]={1,8,9,11,12,13,14,15};
    for(unsigned i=0;i<count&&i<trial_weapon_wheel::MaxEntries;++i) {
        s.entries[i].definition=kind==trial_weapon_wheel::Kind::Guns?guns[i%8]:tools[i%8];
        s.entries[i].usable=true;s.entries[i].loaded=15;s.entries[i].reserve=45;
    }
    return s;
}
static unsigned Frame(float width,float height,bool split) {
    trial_coop::SetActive(split);auto& io=ImGui::GetIO();io.DisplaySize={width,height};io.DeltaTime=1.f/60;
    snapshot_reads=icon_reads=0;icon_names.clear();ImGui::NewFrame();
    auto* draw=ImGui::GetForegroundDrawList();const auto flags=draw->Flags;
    TrialWeaponWheelUi();assert(draw->Flags==flags);
    const unsigned vertices=draw->VtxBuffer.Size;
    for(const auto& vertex:draw->VtxBuffer)assert(std::isfinite(vertex.pos.x)&&std::isfinite(vertex.pos.y));
    ImGui::Render();return vertices;
}
static void FillChecks(bool antialiased) {
    constexpr ImU32 ordinary=IM_COL32(26,29,28,144),selected=IM_COL32(155,35,28,176);
    constexpr ImU32 center=IM_COL32(15,17,16,160),rgb_mask=~IM_COL32_A_MASK;
    unsigned ordinary_vertices=0,selected_vertices=0,center_vertices=0,center_fringes=0;
    const auto* data=ImGui::GetDrawData();
    for(int list=0;list<data->CmdListsCount;++list)for(const auto& vertex:data->CmdLists[list]->VtxBuffer) {
        const auto rgb=vertex.col&rgb_mask;
        // Each wedge must retain its opacity through all internal shared edges.
        if(rgb==(ordinary&rgb_mask)){assert(vertex.col==ordinary);++ordinary_vertices;}
        if(rgb==(selected&rgb_mask)){assert(vertex.col==selected);++selected_vertices;}
        if(rgb==(center&rgb_mask)){
            ++center_vertices;if(!(vertex.col&IM_COL32_A_MASK))++center_fringes;
        }
    }
    assert(ordinary_vertices&&selected_vertices&&center_vertices);
    // The later center disk still follows the caller's AA-fill setting.
    assert((center_fringes!=0)==antialiased);
}
static void HeadingChecks(float width,float height,bool split) {
    const float content_width=std::min(width,height*aspect),content_height=content_width/aspect;
    const float view_height=content_height/(split?2.f:1.f),radius=std::min(content_width*.32f,view_height*.32f);
    const auto* data=ImGui::GetDrawData();
    for(unsigned player=0;player<(split?2u:1u);++player) {
        const float top=(height-content_height)*.5f+player*view_height;
        const float heading_bottom=top+view_height*.5f-radius-8.f*radius/200.f;
        unsigned vertices=0;
        for(int list=0;list<data->CmdListsCount;++list)for(const auto& vertex:data->CmdLists[list]->VtxBuffer)
            if(vertex.col==IM_COL32(234,232,213,255)&&vertex.pos.y>=top&&vertex.pos.y<heading_bottom)++vertices;
        const char* text=snapshots[player].kind==trial_weapon_wheel::Kind::Guns?"Guns wheel":"Melee / throwables wheel";
        unsigned glyphs=0;for(const char* p=text;*p;++p)glyphs+=*p!=' ';
        assert(vertices==glyphs*4); // Actual distinct heading glyphs stay above each player's ring.
    }
}
int main() {
    ImGui::CreateContext();ImGui::GetIO().IniFilename=nullptr;
    unsigned char* pixels;int width,height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    ImGui::GetIO().Fonts->SetTexID(ImTextureID(1));
    for(const auto size:{ImVec2{640,480},ImVec2{1280,720},ImVec2{2560,1440},ImVec2{3440,1440}})
        for(const float ratio:{4.f/3.f,16.f/9.f,21.f/9.f})for(bool split:{false,true})for(bool swap:{false,true}) {
            using Kind=trial_weapon_wheel::Kind;
            aspect=ratio;snapshots[0]=Wheel(8,swap?Kind::MeleeThrowables:Kind::Guns);
            snapshots[1]=Wheel(8,swap?Kind::Guns:Kind::MeleeThrowables);presentation=1;icons=true;
            assert(Frame(size.x,size.y,split)>0);assert(snapshot_reads==(split?2u:1u));
            assert(icon_reads==(split?16u:8u));
            HeadingChecks(size.x,size.y,split);
            FillChecks(true);
            for(unsigned player=0;player<(split?2u:1u);++player)for(unsigned i=0;i<8;++i)
                assert(icon_names[player*8+i]==trial_weapon_wheel::IconName(snapshots[player].entries[i].definition));
            const auto* data=ImGui::GetDrawData();unsigned atlas_commands=0;
            for(int list=0;list<data->CmdListsCount;++list)for(const auto& cmd:data->CmdLists[list]->CmdBuffer)if(cmd.ElemCount) {
                assert(cmd.ClipRect.x>=-.01f&&cmd.ClipRect.y>=-.01f&&cmd.ClipRect.z<=size.x+.01f&&cmd.ClipRect.w<=size.y+.01f);
                if(split)assert(cmd.ClipRect.w<=size.y*.5f+.01f||cmd.ClipRect.y>=size.y*.5f-.01f);
                assert(cmd.TextureId==ImTextureID(1));++atlas_commands;
            }
            assert(atlas_commands>0);
        }
    ImGui::GetStyle().AntiAliasedFill=false;
    for(bool split:{false,true}) {
        assert(Frame(1280,720,split)>0);FillChecks(false);
    }
    ImGui::GetStyle().AntiAliasedFill=true;
    aspect=16.f/9.f;snapshots[0]=snapshots[1]=Wheel();presentation=2;
    assert(Frame(1280,720,true)==0&&snapshot_reads==0);
    presentation=1;snapshots[0].open=false;snapshots[1].available=false;
    assert(Frame(1280,720,true)==0&&icon_reads==0);
    snapshots[0]=Wheel();snapshots[0].count=trial_weapon_wheel::MaxEntries+1;
    assert(Frame(1280,720,false)==0);
    for(auto kind:{trial_weapon_wheel::Kind::Guns,trial_weapon_wheel::Kind::MeleeThrowables}) {
        snapshots[0]=snapshots[1]=Wheel(0,kind);
        assert(Frame(1280,720,true)==0&&icon_reads==0); // Empty groups never divide by zero or access a selected entry.
    }
    for(unsigned count:{1u,2u,trial_weapon_wheel::MaxEntries}) {
        snapshots[0]=Wheel(count);snapshots[0].current=-1;snapshots[0].selected=-1;icons=false;
        assert(Frame(1280,720,false)>0&&icon_reads==count);
    }
    ImGui::DestroyContext();
    std::puts("PASS seamless translucent wedge geometry and AA restoration, both wheel headings and portraits independently per player, empty groups, bounded snapshots, missing-art fallback, aspect bars, split-screen clipping, hidden/invalid/cinematic gates");
}
