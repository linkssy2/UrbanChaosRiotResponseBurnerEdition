#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace trial_fmv {
enum class Kind { Background, Preserve, Movie };
struct Movie { Kind kind=Kind::Preserve; uint16_t top=0, bottom=0; };
inline Movie Classify(std::string_view name) {
    if(name=="fe_bk1"||name=="fe_mp"||name=="gamelogo"||name=="splash")return {Kind::Background};
    // Exact black-row bounds from the union of every decoded frame's nonzero RGB.
    constexpr struct { std::string_view name; uint16_t top,bottom; } movies[]={
        {"attract",96,96}, {"complete",64,96}, {"credits",96,96},
        {"dolby",48,48}, {"eidoslogo",96,96}, {"rocklogo",96,96},
        {"intro_12_edit",96,96}, {"intro_21",64,96}, {"intro_22",64,96},
        {"intro_23",64,96}, {"intro_24",64,96}, {"intro_25",64,96},
        {"intro_26",64,96}, {"reward",64,96},
    };
    for(const auto& movie:movies)if(name==movie.name)return {Kind::Movie,movie.top,movie.bottom};
    // News films, PAL variants and unknown names retain their complete frame.
    return {};
}
struct Layout { float x=1, y=1, top=0, bottom=0; };
inline Layout Geometry(Movie movie,float display,uint32_t mode) {
    if(!(display>=4.f/3.f&&display<=32.f/9.f))display=4.f/3.f;
    if(movie.kind==Kind::Background)return {(std::min)(1.f,(16.f/9.f)/display),1,0,0};
    if(mode==2)return {1,1,float(movie.top),float(movie.bottom)};
    // Proportional Fill retains its existing four-row safety margin.
    const float top=mode==0?(std::max)(0,int(movie.top)-4):0;
    const float bottom=mode==0?(std::max)(0,int(movie.bottom)-4):0;
    const float picture=640/(480-top-bottom);
    return {(std::min)(1.f,picture/display),(std::min)(1.f,display/picture),top,bottom};
}
}
struct X86Context;
void TrialFmvOpen(X86Context&,uint8_t*);
void TrialFmvClose();
trial_fmv::Layout TrialFmvLayout(uint8_t*);
void TrialFmvVertex(X86Context&,uint8_t*,trial_fmv::Layout);
void TrialFmvTexcoord(X86Context&,uint8_t*,trial_fmv::Layout);
void TrialFmvBorders(X86Context&,uint8_t*,trial_fmv::Layout);
