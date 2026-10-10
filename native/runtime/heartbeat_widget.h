#pragma once
#include "hud_heartbeat_draw.h"
class NativeDialogueText;
struct UCFont;
// Original171A0, label flags4/scale1 printable domain.
float native_heartbeat_label_width(const UCFont&,const char*);
struct NativeHeartbeatWidgetPlan {
    NativeHeartbeatQuads shapes;size_t label_after=0;
    float label_x=0,label_y=0;uint32_t label_color=0;bool label_visible=false;
};
// Whole71D70 and the70F80 child graph. Quads retain Xbox half-pixel corners;
// the PC renderer converts those once. Optional retained order icon follows7200D.
NativeHeartbeatWidgetPlan native_heartbeat_widget_plan(const UCHudHeartbeat&,int32_t actor,
    float x,float y,const float color[4],float order_label_width=-1);
// The actual53-column NPC70F80 widget, including background, damage layers and
// label. The enclosing four-slot71390 manager supplies identity/order/position.
class NativeHeartbeatWidget {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeHeartbeatWidget();~NativeHeartbeatWidget();
    void load(const std::filesystem::path&,NativeRenderer&,NativeDialogueText&,const UCArchive*level);
    // Actual71200 domain: actor,resource,0,0. Previoushealth is the real218120
    // result (or0 for absent actor); no profile death-message mutation requested.
    int bind(int32_t actor,const char*resource,float previous_health);
    void clear();bool active()const;
    int32_t actor()const;
    void step(const UCHudHeartbeatActor&,float,UCGameRandom&,void(*sound)(void*),void*);
    void action_feedback(ID3D11ShaderResourceView*);
    bool has_feedback()const;
    void draw(NativeRenderer&,float x,float y,const float color[4],bool order_available=false);
    const UCHudHeartbeat&history()const;
};
