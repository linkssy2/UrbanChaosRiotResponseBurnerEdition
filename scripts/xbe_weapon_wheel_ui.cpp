#include "xbe_weapon_wheel_ui.h"
#include "xbe_weapon_wheel.h"
#include "xbe_frontend_fonts.h"
#include "xbe_graphics_settings.h"
#include "xbe_coop.h"
#include "xbe_coop_cinematics.h"
#include "xbe_spanish_ui.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
constexpr float pi=3.14159265358979323846f;
constexpr ImU32 steel=IM_COL32(147,151,141,225),ivory=IM_COL32(234,232,213,255);
ImVec2 Point(ImVec2 c,float radius,float angle) {
    return {c.x+radius*std::cos(angle),c.y+radius*std::sin(angle)};
}
void Wedge(ImDrawList* draw,ImVec2 c,float inner,float outer,float begin,float end,ImU32 color) {
    const auto flags=draw->Flags;
    // Adjacent translucent quads share edges; AA fringes would expose each subdivision.
    draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
    const unsigned steps=std::max(2u,unsigned(std::ceil((end-begin)*12.f)));
    for(unsigned i=0;i<steps;++i) {
        const float a=begin+(end-begin)*i/steps,b=begin+(end-begin)*(i+1)/steps;
        const auto p=Point(c,inner,a),q=Point(c,outer,a),r=Point(c,outer,b),s=Point(c,inner,b);
        draw->AddQuadFilled(p,q,r,s,color);
    }
    draw->Flags=flags;
}
void CenterText(ImDrawList* draw,ImFont* font,const char* text,ImVec2 at,float size,float width,ImU32 color) {
    if(!text||!*text)return;
    text=trial_language::Ui(text);
    const float measured=font->CalcTextSizeA(size,100000.f,0,text).x;
    if(measured>width)size*=width/measured;
    const auto extent=font->CalcTextSizeA(size,100000.f,0,text);
    draw->AddText(font,size,{at.x-extent.x*.5f,at.y},color,text);
}
void Draw(const trial_weapon_wheel::Snapshot& wheel,ImDrawList* draw,ImFont* font,ImVec2 origin,float width,float height) {
    if(!wheel.available||!wheel.open||!wheel.count||wheel.count>trial_weapon_wheel::MaxEntries||
       !std::isfinite(width)||!std::isfinite(height)||width<64||height<64)return;
    const ImVec2 center{origin.x+width*.5f,origin.y+height*.5f};
    const float radius=std::min(width*.32f,height*.32f),inner=radius*.46f,scale=radius/200.f;
    const float arc=2*pi/wheel.count;
    draw->PushClipRect(origin,{origin.x+width,origin.y+height},true);
    const char* heading=wheel.kind==trial_weapon_wheel::Kind::Guns?"Guns wheel":"Melee / throwables wheel";
    CenterText(draw,font,heading,{center.x+scale,center.y-radius-26.f*scale},17.f*scale,radius*2.f,IM_COL32(0,0,0,192));
    CenterText(draw,font,heading,{center.x,center.y-radius-27.f*scale},17.f*scale,radius*2.f,ivory);
    draw->AddCircleFilled(center,radius+4*scale,IM_COL32(0,0,0,64),96);
    for(unsigned i=0;i<wheel.count;++i) {
        const bool selected=int(i)==wheel.selected;
        const float angle=-pi*.5f+arc*i,begin=angle-arc*.5f,end=angle+arc*.5f;
        const auto color=selected?IM_COL32(155,35,28,176):IM_COL32(26,29,28,144);
        Wedge(draw,center,inner,radius,begin,end,color);
        if(wheel.count>1)draw->AddLine(Point(center,inner,begin),Point(center,radius,begin),steel,std::max(1.f,scale));
        const auto icon=Point(center,radius*.73f,angle);
        // Keep the authored 2:1 portrait proportions, including in split-screen.
        const float icon_width=std::min(105.f*scale,std::max(18.f*scale,radius*.73f*arc*.72f));
        float uv[4]{};
        if(TrialWeaponIconUV(trial_weapon_wheel::IconName(wheel.entries[i].definition),uv)) {
            const auto tint=wheel.entries[i].usable?ivory:IM_COL32(128,128,121,170);
            draw->AddImage(ImGui::GetIO().Fonts->TexID,{icon.x-icon_width*.5f,icon.y-icon_width*.25f},
                {icon.x+icon_width*.5f,icon.y+icon_width*.25f},{uv[0],uv[1]},{uv[2],uv[3]},tint);
        } else {
            CenterText(draw,font,trial_weapon_wheel::Name(wheel.entries[i].definition),
                {icon.x,icon.y-7.f*scale},14.f*scale,icon_width,ivory);
        }
        if(int(i)==wheel.current) {
            const auto tip=Point(center,radius*.94f,angle);
            draw->AddTriangleFilled(tip,Point(center,radius*.88f,angle-.045f),Point(center,radius*.88f,angle+.045f),ivory);
        }
    }
    draw->AddCircle(center,radius,steel,96,std::max(1.f,2.f*scale));
    draw->AddCircleFilled(center,inner,IM_COL32(15,17,16,160),64);
    draw->AddCircle(center,inner,steel,64,std::max(1.f,scale));
    const int selected=wheel.selected>=0&&unsigned(wheel.selected)<wheel.count?wheel.selected:wheel.current;
    if(selected>=0&&unsigned(selected)<wheel.count) {
        const auto& entry=wheel.entries[selected];
        CenterText(draw,font,trial_weapon_wheel::Name(entry.definition),{center.x,center.y-24.f*scale},
            20.f*scale,inner*1.78f,ivory);
        if(entry.definition!=8) { // Cleaver has no ammunition; native categories also include thrown weapons.
            char ammo[48];std::snprintf(ammo,sizeof(ammo),"%u / %u",entry.loaded,entry.reserve);
            CenterText(draw,font,ammo,{center.x,center.y+1.f*scale},16.f*scale,inner*1.7f,steel);
        }
    }
    CenterText(draw,font,"Release to select",{center.x,center.y+32.f*scale},13.f*scale,inner*1.75f,steel);
    draw->PopClipRect();
}
}

void TrialWeaponWheelUi() {
    const auto* view=ImGui::GetMainViewport();
    const auto bounds=trial_graphics::ViewBounds(view->Size.x,view->Size.y);
    const bool split=trial_coop::Active();
    if(split&&TrialCoopPresentation()!=1)return;
    auto* font=TrialFrontendBodyFont();if(!font)font=ImGui::GetFont();
    auto* draw=ImGui::GetForegroundDrawList();
    for(unsigned player=0;player<(split?2u:1u);++player) {
        const float height=bounds.height/(split?2.f:1.f);
        Draw(trial_weapon_wheel::GetSnapshot(player),draw,font,
            {view->Pos.x+bounds.x,view->Pos.y+bounds.y+player*height},bounds.width,height);
    }
}
