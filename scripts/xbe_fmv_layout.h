#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace trial_fmv {
enum class Kind { Background, Preserve, Movie };
struct Movie { Kind kind=Kind::Preserve; uint16_t top=0, bottom=0; };
inline Movie Classify(std::string_view name) {
    if(name=="fe_bk1"||name=="fe_mp"||name=="gamelogo"||name=="splash")return {Kind::Background};
    // Every decoded frame audited; retain four black rows beyond the nonzero RGB union.
    constexpr struct { std::string_view name; uint16_t top,bottom; } movies[]={
        {"attract",92,92}, {"complete",60,92}, {"credits",92,92},
        {"dolby",44,44}, {"eidoslogo",92,92}, {"rocklogo",92,92},
        {"intro_12_edit",92,92}, {"intro_21",60,92}, {"intro_22",60,92},
        {"intro_23",60,92}, {"intro_24",60,92}, {"intro_25",60,92},
        {"intro_26",60,92}, {"reward",60,92},
    };
    for(const auto& movie:movies)if(name==movie.name)return {Kind::Movie,movie.top,movie.bottom};
    // News films, PAL variants and unknown names retain their complete frame.
    return {};
}
struct Layout { float x=1, y=1, top=0, bottom=0; };
inline Layout Geometry(Movie movie,bool wide,uint32_t mode) {
    if(movie.kind==Kind::Background)return {};
    const float top=mode==0?movie.top:0, bottom=mode==0?movie.bottom:0;
    const float picture=640/(480-top-bottom), display=wide?16.f/9:4.f/3;
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
