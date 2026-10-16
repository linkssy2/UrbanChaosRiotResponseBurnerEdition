#pragma once
#include <algorithm>
#include <cstdint>
#include "hybrid_frontend_guest.h"

enum { TRIAL_HF_NAME=6, TRIAL_HF_DELETE=8 };
struct TrialLevelRect { float x=0,y=0,w=0,h=0; };
struct TrialFrontendFrame {
    UcHybridFrontendSnapshot source{};
    char labels[55][128]{};
    float row_top=390;
    uint32_t list_kind=0;
    TrialLevelRect profile_bounds[4]{};
    char name[16]{},name_error[128]{};
    uint32_t name_capacity=0;
    uint32_t profile_delete_mask=0;
    bool delete_dialog=false;
    char delete_name[20]{};
};
struct TrialFrontendNameActionData {
    UcHybridFrontendAction identity{};
    char text[16]{};
};
inline bool TrialFrontendNameText(const char (&text)[16]) {
    for(unsigned i=0;i<16;++i){
        const unsigned c=static_cast<unsigned char>(text[i]);
        if(!c)return true;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '))return false;
    }
    return false;
}
struct TrialFrontendReleaseGate {
    uint64_t epoch=0;bool released=false;
    bool Accept(uint64_t next,bool held){
        if(epoch!=next){epoch=next;released=false;}
        const bool accepted=released;
        if(!held)released=true;
        return accepted;
    }
};
inline TrialLevelRect TrialLevelBounds(unsigned index,unsigned count) {
    const float step=std::min(30.f,276.f/std::max(1u,count));
    return {8.f,94.f+step*index,36.f,step};
}
inline bool TrialFrontendSupported(const TrialFrontendFrame& f) {
    const auto& s=f.source;
    if(!s.epoch||!s.screen||!s.owner||!s.count||s.selected>=s.count||
       (s.source_kind&UC_HF_TRANSITION))return false;
    if(s.kind==UC_HF_PROFILES){
        if(s.count!=4)return false;
        for(const auto& r:f.profile_bounds)if(!(r.w>0&&r.h>0))return false;
        return true;
    }
    if(s.kind==UC_HF_DIALOG)return f.delete_dialog&&f.delete_name[0]&&s.count==2&&
        s.rows[0].callback==0x284580&&s.rows[1].callback==0x284310;
    return (s.kind==TRIAL_HF_NAME&&s.count==1&&f.name_capacity==15)||
           (s.kind==UC_HF_LIST&&f.list_kind>=1&&f.list_kind<=3&&s.count<=20)||
           (s.kind==UC_HF_LEVELS&&s.count<=17);
}
// Keep copied presentation through a title transition; revoke input separately.
struct TrialFrontendPresentation {
    TrialFrontendFrame frame{};
    bool visible=false,transition=false;
    bool Presents(uint32_t owner,uint32_t kind)const{
        return visible&&frame.source.owner==owner&&frame.source.kind==kind;
    }
    void Publish(const TrialFrontendFrame& next,bool active){
        if(active&&TrialFrontendSupported(next)){
            frame=next;visible=true;transition=false;
        }else if(visible&&(next.source.source_kind&UC_HF_TRANSITION)){
            transition=true;
        }else{
            frame={};visible=false;transition=false;
        }
    }
};
void TrialFrontendPublish(const TrialFrontendFrame& frame);
bool TrialFrontendTakeAction(UcHybridFrontendAction& action);
bool TrialFrontendOwns(uint32_t owner,uint32_t kind);
// Rendering qualification only. Does not authorize input or guest callbacks.
bool TrialFrontendPresents(uint32_t owner,uint32_t kind);
void TrialFrontendNameAction(const TrialFrontendFrame&,unsigned kind,const char* text);
bool TrialFrontendTakeName(TrialFrontendNameActionData&);
