#pragma once
#include "xbe_hd_ui_filter.h"
#include <cstring>

namespace uc_hd_ui::reconstruct {
using Cell=filter::font_detail::Cell;
namespace detail {
inline uint64_t Hash(std::span<const uint8_t> bytes) {
    uint64_t hash=1469598103934665603ull;
    for(uint8_t byte:bytes){hash^=byte;hash*=1099511628211ull;}
    return hash;
}
inline bool Valid(uint32_t width,uint32_t height,std::span<const uint8_t> source,
                  std::span<const Cell> cells,const std::vector<uint8_t>& output) {
    if(!width||!height||width>1024||height>1024||cells.empty()||cells.size()>256||
       source.size()!=size_t(width)*height*4||output.size()!=source.size()*16)return false;
    for(size_t i=0;i<cells.size();++i) {
        const auto a=cells[i];
        for(float v:{a.x0,a.y0,a.x1,a.y1})if(!std::isfinite(v)||v!=std::floor(v))return false;
        if(a.x0<0||a.y0<0||a.x1>width||a.y1>height||a.x0>=a.x1||a.y0>=a.y1)return false;
        for(size_t j=0;j<i;++j) {
            const auto b=cells[j];
            if(a.x0<b.x1&&a.x1>b.x0&&a.y0<b.y1&&a.y1>b.y0)return false;
        }
    }
    return true;
}

// Re-rasterize the original light face separately from its dark outline. Half
// the original RGB shading remains, including the title's blue/metallic tint.
// This never changes alpha, UVs, glyph metrics or pixels outside proved cells.
inline bool Face(uint32_t width,uint32_t height,std::span<const uint8_t> source,
                 std::span<const Cell> cells,std::vector<uint8_t>& output) {
    if(!Valid(width,height,source,cells,output))return false;
    std::vector<uint8_t> face(source.size(),0),layer(output.size(),0);
    for(size_t i=0;i<source.size();i+=4)
        face[i+3]=source[i+3]?std::max({source[i],source[i+1],source[i+2]}):0;
    for(const auto cell:cells)filter::font_detail::RestoreCell(width,face,cell,layer);
    for(const auto cell:cells)
      for(unsigned y=unsigned(cell.y0*4);y<unsigned(cell.y1*4);++y)
        for(unsigned x=unsigned(cell.x0*4);x<unsigned(cell.x1*4);++x) {
            const size_t i=(size_t(y)*width*4+x)*4;
            if(!output[i+3])continue;
            const unsigned old_light=std::max({output[i],output[i+1],output[i+2]});
            const float delta=(float(layer[i+3])-old_light)*.5f;
            for(unsigned c=0;c<3;++c)output[i+c]=filter::detail::Byte(output[i+c]+delta);
        }
    return true;
}
inline bool ButtonLetters(std::span<const uint8_t> source,std::vector<uint8_t>& output) {
    // Tight retail ink bounds, not the surrounding glossy spheres. Contrast
    // endpoints come from each pinned source row; preserve alpha and RGB hue.
    const std::array<Cell,4> cells{{{9,32+11,24,32+24},{9,64+12,23,64+25},
                                  {9,96+11,24,96+25},{9,128+10,24,128+24}}};
    if(!Valid(32,1024,source,cells,output))return false;
    constexpr float dark[]{89,189,93,99},light[]{190,255,189,231};
    std::vector<uint8_t> ink(source.size(),0),layer(output.size(),0);
    for(unsigned row=0;row<cells.size();++row) {
        const auto cell=cells[row];const float range=light[row]-dark[row];
        for(unsigned y=unsigned(cell.y0);y<unsigned(cell.y1);++y)
          for(unsigned x=unsigned(cell.x0);x<unsigned(cell.x1);++x) {
            const size_t i=(size_t(y)*32+x)*4;
            ink[i+3]=filter::detail::Byte((light[row]-std::max({source[i],source[i+1],source[i+2]}))*255/range);
        }
        filter::font_detail::RestoreCell(32,ink,cell,layer);
        for(unsigned y=unsigned(cell.y0*4);y<unsigned(cell.y1*4);++y)
          for(unsigned x=unsigned(cell.x0*4);x<unsigned(cell.x1*4);++x) {
            const size_t i=(size_t(y)*128+x)*4;
            const float value=float(std::max({output[i],output[i+1],output[i+2]}));
            if(!output[i+3]||!value)continue;
            const float old_ink=std::clamp((light[row]-value)/range,0.f,1.f);
            const float adjusted=std::clamp(value+(old_ink-layer[i+3]/255.f)*range,0.f,255.f);
            for(unsigned c=0;c<3;++c)output[i+c]=filter::detail::Byte(output[i+c]*adjusted/value);
        }
    }
    return true;
}
struct Asset {const char* name;uint32_t width,height;uint64_t pixels;};
inline constexpr Asset kHud[]{
    {"hud\\hud_ammo_numbers.tga=nomipmap",256,32,0x79cac540398553a9ull},
    {"hud\\hud_crosshair_pistol.tga=nomipmap",64,64,0x59154be4711008ffull},
    {"hud\\hud_crosshair_shotgun.tga=nomipmap",64,64,0x20d73c0c7cda6d24ull},
    {"hud\\hud_crosshair_assaultrifle.tga=nomipmap",64,64,0xc169dbd66157a45cull},
    {"hud\\hud_crosshair_slug.tga=nomipmap",64,64,0xf9360ba480f24b6eull},
    {"hud\\hud_crosshair_sniper.tga=nomipmap",128,128,0xc05730765d9afb1aull},
};
inline constexpr Asset kFonts[]{
    {"medium",256,256,0x00ca53bfcd9f8940ull},
    {"large",512,512,0x54780767a91bb40aull},
    {"small",256,256,0xc0181f281166b354ull},
};
// The white title subtitle can be reconstructed separately. The four authored
// localized strips receive 4x filtering: their sparse alpha cannot safely be
// contoured without breaking thin strokes/trademark marks. Publisher marks and
// the colored flame/tree artwork are never reconstructed.
inline constexpr Asset kLettering[]{
    {"frontend\\frt_content\\fe_ztlogo%24b%.tga",256,256,0x992c3746be8a31a2ull},
    {"frontend\\frt_content\\riot_spa.tga",512,32,0xf9548cfd50aeccacull},
    {"frontend\\frt_content\\riot_fre.tga",512,32,0x32d56ada61f63050ull},
    {"frontend\\frt_content\\riot_ita.tga",512,32,0xc5930c4e4dcc34fbull},
    {"frontend\\frt_content\\riot_eng.tga",256,32,0xd0ba26fa18ae6aaeull},
};
// Alpha-only arrows have no halo. Shaded marks and operation symbols keep
// their filtered alpha/shadows; only their original inner light face sharpens.
inline constexpr Asset kArrows[]{
    {"frontend\\frt_content\\arrow_left.tga",16,16,0x4b035c6aad84c1a9ull},
    {"frontend\\frt_content\\arrow_down.tga",16,16,0xca4610a9f2e72e74ull},
    {"frontend\\frt_content\\arrow_right.tga",16,16,0x4b1fae6936da82ceull},
    {"frontend\\frt_content\\arrow_up.tga",16,16,0xb8869ee44961d67dull},
};
inline constexpr Asset kSymbols[]{
    {"font\\font_buttons.tga",32,1024,0x4f5ddf32ab6a7248ull},
    {"frontend\\frt_content\\tick_lava.tga",64,64,0x53215096c7f7c1e2ull},
    {"frontend\\frt_content\\tick_gold.tga",64,64,0x2f4d8a82a04af656ull},
    {"frontend\\frt_content\\tick_silver.tga",64,64,0x3d371b4697ab1392ull},
    {"frontend\\frt_content\\tick_bronze.tga",64,64,0x85c147b2d01a013full},
    {"frontend\\frt_content\\tick.tga",64,64,0xee5e0ed89ddb1716ull},
    {"hud\\hud_radartriangledown.tga=nomipmap",16,16,0xdc37bb4a96880a10ull},
    {"hud\\hud_radardownarrow.tga=nomipmap",16,16,0x31dcdf6b8e030fccull},
    {"hud\\hud_radaruparrow.tga=nomipmap",16,16,0xd9923e5a39a98a27ull},
    {"hud\\hud_radartriangle.tga=nomipmap",16,16,0xfc3f8b9fa5975410ull},
    {"hud\\hud_radarsquare.tga=nomipmap",16,16,0x3cadea34e4c2548bull},
    {"hud\\hud_radarcircle.tga=nomipmap",16,16,0xa128e54faa299c4eull},
    {"hud\\hud_orders_pad_xbox.tga=nomipmap",64,64,0x0a5623068ec25ae5ull},
    {"hud\\hud_icon_heal_no.tga=nomipmap",64,64,0x7220f5d21600b82cull},
    {"hud\\hud_icon_heal_yes.tga=nomipmap",64,64,0x58e4c15c8a4a9b7cull},
    {"hud\\hud_orders_carry.tga=nomipmap",64,64,0x6d8077af3f0d2cdbull},
    {"hud\\hud_icon_shutter.tga=nomipmap",64,64,0xaf66862ec87a5494ull},
    {"hud\\hud_icon_extinguish.tga=nomipmap",64,64,0x49a4b6215393e99full},
    {"hud\\hud_operate_civilian.tga=nomipmap",64,64,0x8382cf6a8fb2a0f6ull},
    {"hud\\hud_icon_cuffs.tga=nomipmap",64,64,0xfc72f7b580a4eb4cull},
    {"hud\\hud_icon_cop_sweep.tga=nomipmap",64,64,0x56aa1980bcfa6514ull},
    {"hud\\hud_orders_axe.tga=nomipmap",64,64,0xd4e490bee9283fabull},
    {"hud\\hud_operate_door.tga=nomipmap",64,64,0x12c1ca5c846e2b15ull},
    {"hud\\hud_icon_heal_player.tga=nomipmap",64,64,0xea7dec44ba6f04b7ull},
    {"hud\\hud_icon_heal_npc.tga=nomipmap",64,64,0xee13e3932334f7cdull},
    {"hud\\hud_orders_cover_fireman.tga=nomipmap",64,64,0xea1fb7382d3aef93ull},
};
template<size_t N> inline const Asset* Match(const Asset (&assets)[N],const char* name,
    uint32_t width,uint32_t height,std::span<const uint8_t> source) {
    if(!name||source.size()!=size_t(width)*height*4)return nullptr;
    for(const auto& asset:assets)
        if(width==asset.width&&height==asset.height&&!std::strcmp(name,asset.name)&&Hash(source)==asset.pixels)return &asset;
    return nullptr;
}
}

// Allocate the bounded 4x canvas before decoding; pixel reconstruction below
// additionally requires the exact decoded retail fingerprint.
inline bool LetteringScale4(const char* name,uint32_t width,uint32_t height) {
    if(!name)return false;
    for(const auto& asset:detail::kLettering)
        if(width==asset.width&&height==asset.height&&!std::strcmp(name,asset.name))return true;
    return false;
}

// The existing ABC parser supplies already validated, deduplicated glyph cells.
// Only the exact retail outlined/metallic sheets receive the inner-face treatment.
inline bool FontFace(const char* name,uint32_t width,uint32_t height,
    std::span<const uint8_t> source,std::span<const Cell> cells,std::vector<uint8_t>& output) {
    return detail::Match(detail::kFonts,name,width,height,source)&&detail::Face(width,height,source,cells,output);
}

inline bool Hud(const char* name,uint32_t width,uint32_t height,
    std::span<const uint8_t> source,unsigned scale,std::vector<uint8_t>& output) {
    if(scale!=4)return false;
    if(const auto* lettering=detail::Match(detail::kLettering,name,width,height,source)) {
        if(lettering==&detail::kLettering[0]) {
            const Cell subtitle{18,215,237,240};
            return detail::Face(width,height,source,{&subtitle,1},output);
        }
        return false;
    }
    if(const auto* symbol=detail::Match(detail::kSymbols,name,width,height,source)) {
        std::array<Cell,5> cells{};size_t count=1;
        if(symbol==&detail::kSymbols[0]) {
            return detail::ButtonLetters(source,output);
        } else cells[0]={0,0,float(width),float(height)};
        return detail::Face(width,height,source,{cells.data(),count},output);
    }
    if(detail::Match(detail::kArrows,name,width,height,source)) {
        const Cell cell{0,0,float(width),float(height)};
        if(!detail::Valid(width,height,source,{&cell,1},output))return false;
        auto result=output;filter::font_detail::RestoreCell(width,source,cell,result);
        output.swap(result);return true;
    }
    const auto* asset=detail::Match(detail::kHud,name,width,height,source);
    if(!asset)return false;
    std::array<Cell,10> cells{};size_t count=1;
    if(asset==&detail::kHud[0]) {
        // Retail ammo constructor/43850: digit UVs are n*19/256,0 ..
        // (n+1)*19/256,27/32. Infinity/slash/dash and padding stay untouched.
        count=10;
        for(unsigned i=0;i<10;++i)cells[i]={float(i*19),0,float((i+1)*19),27};
    } else cells[0]={0,0,float(width),float(height)};
    const std::span<const Cell> selected(cells.data(),count);
    if(!detail::Valid(width,height,source,selected,output))return false;
    auto result=output;
    for(const auto cell:selected)filter::font_detail::RestoreCell(width,source,cell,result);
    if(asset==&detail::kHud[0])detail::Face(width,height,source,selected,result);
    output.swap(result);return true;
}
} // namespace uc_hd_ui::reconstruct
