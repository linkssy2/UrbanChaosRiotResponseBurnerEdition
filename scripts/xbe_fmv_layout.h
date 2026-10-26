#pragma once
#include <cstdint>
#include <string_view>

namespace trial_fmv {
enum class Kind { Background, Preserve, Movie };
inline Kind Classify(std::string_view name) {
    if(name=="fe_bk1"||name=="fe_mp"||name=="gamelogo"||name=="splash")return Kind::Background;
    if(name.substr(0,4)=="pal_")name.remove_prefix(4);
    constexpr std::string_view movies[]={"attract","complete","credits","reward","intro_12_edit","outro_13",
        "intro_1","intro_2","intro_3","intro_4","intro_5","intro_6","intro_8","intro_9",
        "intro_11","intro_12","intro_13","intro_21","intro_22","intro_23","intro_24","intro_25","intro_26"};
    for(auto movie:movies)if(name==movie)return Kind::Movie;
    // Logos and unrecognized films retain the complete picture, including text.
    return Kind::Preserve;
}
struct Layout { float x=1, y=1; };
inline Layout Geometry(Kind kind,bool wide,uint32_t mode) {
    if(!wide||kind==Kind::Background)return {};
    return kind==Kind::Movie&&mode==0 ? Layout{1,4.f/3.f} : Layout{.75f,1};
}
}
struct X86Context;
void TrialFmvOpen(X86Context&,uint8_t*);
void TrialFmvClose();
trial_fmv::Layout TrialFmvLayout(uint8_t*);
void TrialFmvVertex(X86Context&,uint8_t*,trial_fmv::Layout);
void TrialFmvBorders(X86Context&,uint8_t*,trial_fmv::Layout);
