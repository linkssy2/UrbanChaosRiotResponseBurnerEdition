#include "xbe_frontend_fonts.h"
#include "xbe_frontend_text.h"
#include "xbe_pc_prompts.h"
#include "imgui.h"
#include "kernel/disc.h"
extern "C" {
#include "native/font.h"
#include "native/texture_asset/texture_asset.h"
}
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
int native_hud_keycap(uint32_t*,unsigned,const char*,const char*);

namespace {
constexpr unsigned icon_codes[]={13,'Q','D','R',37,39,1,27,2,0x101,0x102,0x103,0x104,0x105,0x201,0x202,0x203,0x204,0x205,0x301,0x302,0x303,0x304,0x305,0x401,0x402,0x403,0x404,0x405};
std::array<std::array<float,4>,29> icon_uv{};
bool icons_ready=false;
using Archive=std::unique_ptr<UCArchive,decltype(&uc_archive_close)>;
struct Bytes {UCArchiveBytes value{};~Bytes(){uc_archive_bytes_free(&value);}};
struct Texture {UCTextureAsset value{};~Texture(){uc_texture_free(&value);}};
struct FontData {
    Bytes metrics;UCFont font{};unsigned width=0,height=0;
    std::vector<uint32_t> pixels;
};
uint16_t U16(const uint8_t* p){return uint16_t(p[0]|unsigned(p[1])<<8);}
float F32(const uint8_t* p){float value;std::memcpy(&value,p,4);return value;}

Archive Open(const std::filesystem::path& game,const char* name){
    std::vector<uint8_t> bytes;
    constexpr size_t limit=4*1024*1024;
    if(const auto* disc=kernel::MountedDisc()){
        const auto* entry=disc->Find(std::string_view(name));
        if(!entry||entry->dir||entry->size>limit||!disc->Load(name,bytes))return {nullptr,uc_archive_close};
    }else{
        std::ifstream file(game/name,std::ios::binary|std::ios::ate);
        if(!file)return {nullptr,uc_archive_close};
        auto size=file.tellg();if(size<200||size>std::streamoff(limit))return {nullptr,uc_archive_close};
        bytes.resize(size_t(size));file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))return {nullptr,uc_archive_close};
    }
    UCArchive* result=nullptr;
    if(uc_archive_copy(bytes.data(),bytes.size(),&result)!=UC_AR_OK)return {nullptr,uc_archive_close};
    return {result,uc_archive_close};
}
bool Read(const UCArchive* archive,const std::string& name,Bytes& out){
    for(size_t i=0;i<uc_archive_count(archive);++i){
        const auto* e=uc_archive_entry(archive,i);
        if(e->name_bytes==name.size()+1&&!std::memcmp(e->name,name.c_str(),e->name_bytes))
            return e->unpacked_bytes<=2*1024*1024&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&out.value)==UC_AR_OK;
    }
    return false;
}
bool Load(const UCArchive* metrics,const UCArchive* art,const char* name,FontData& out){
    const std::string stem=std::string("fonts\\")+name;
    if(!Read(metrics,stem+".abc",out.metrics)||uc_font_read(out.metrics.value.data,out.metrics.value.size,&out.font))return false;
    Bytes encoded;Texture texture;
    if(!Read(art,stem+"%4b%.tga",encoded)||uc_texture_decode(encoded.value.data,encoded.value.size,&texture.value)!=UC_TEXTURE_OK)return false;
    const auto& t=texture.value;
    if(t.format!=UC_TEXTURE_BC3_UNORM||t.width<4||t.height<4||t.width>1024||t.height>1024||
       out.font.maximum<126||out.font.height>128||std::floor(out.font.height)!=out.font.height)return false;
    // Reject mismatched metrics/texture pairs rather than inventing new glyphs.
    for(unsigned i=0;i<out.font.count;++i){
        const uint8_t* g=out.font.glyphs+i*24;
        if(std::abs((F32(g+8)-F32(g))*t.width-int16_t(U16(g+18)))>.01f||
           std::abs((F32(g+12)-F32(g+4))*t.height-out.font.height)>.01f)return false;
    }
    out.width=t.width;out.height=t.height;out.pixels.resize(size_t(t.width)*t.height);
    uc_pc_prompts::DecodeBc3(t.mips[0].data,t.width,t.height,out.pixels.data());
    return true;
}
}

bool TrialLoadFrontendFonts(ImFontAtlas* atlas,const std::filesystem::path& game,ImFont*& body,ImFont*& title){
    body=title=nullptr;
    icons_ready=false;
    if(!atlas||atlas->Locked)return false;
    auto metrics=Open(game,"dialoguefonts.lst"),art=Open(game,"PersistantFilesXBox.lst");
    FontData data[2];
    if(!metrics||!art||!Load(metrics.get(),art.get(),"medium",data[0])||!Load(metrics.get(),art.get(),"large",data[1]))return false;
    Bytes buttons;Texture button_texture;
    if(!Read(art.get(),"font\\font_buttons.tga",buttons)||
       uc_texture_decode(buttons.value.data,buttons.value.size,&button_texture.value)!=UC_TEXTURE_OK)return false;
    const auto& bt=button_texture.value;
    if(bt.format!=UC_TEXTURE_BC3_UNORM||bt.width!=32||bt.height!=1024)return false;
    std::vector<uint32_t> button_pixels(32*1024);
    uc_pc_prompts::DecodeBc3(bt.mips[0].data,32,1024,button_pixels.data());
    std::array<std::vector<uint32_t>,29> icons;
    const char* labels[]={"ENTER","Q","D","R","<",">","","ESC",""};
    for(unsigned i=0;i<9;++i){
        icons[i].resize(128*128);
        if(!native_hud_keycap(icons[i].data(),icon_codes[i],labels[i],nullptr))return false;
    }
    // Original button strip: A, Y, X, B, then the direction pad.
    const unsigned rows[]={1,4,5,3,2};
    for(unsigned i=9;i<14;++i)
        icons[i].assign(button_pixels.begin()+rows[i-9]*1024,button_pixels.begin()+(rows[i-9]+1)*1024);
    for(unsigned family=1;family<=3;++family)for(unsigned i=0;i<5;++i){
        auto& pixels=icons[14+(family-1)*5+i];pixels.resize(128*128);
        const unsigned sources[]={1,2,0,3,4};
        if(i==2){for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
            pixels[y*128+x]=button_pixels[(5*32+y/4)*32+x/4];}
        else TrialControllerGlyph(pixels.data(),sources[i],family);
    }
    ImFont* fonts[2]{};int rectangles[2]{},icon_rectangles[29]{};
    static const ImWchar seed_range[]={'?','?',0};
    for(unsigned i=0;i<2;++i){
        // ImGui requires a builder-owned font record. Its single seed glyph is
        // removed below; every visible menu glyph then comes from the game.
        ImFontConfig cfg;cfg.SizePixels=data[i].font.height;cfg.GlyphRanges=seed_range;
        cfg.OversampleH=cfg.OversampleV=1;
        std::snprintf(cfg.Name,sizeof(cfg.Name),"Urban Chaos %s",i?"large":"medium");
        fonts[i]=atlas->AddFontDefault(&cfg);
        rectangles[i]=atlas->AddCustomRectRegular(data[i].width,data[i].height);
    }
    for(unsigned i=0;i<29;++i)icon_rectangles[i]=atlas->AddCustomRectRegular((i<9||i>=14)?128:32,(i<9||i>=14)?128:32);
    // The largest original atlas needs room for ImGui's packing padding too.
    atlas->TexDesiredWidth=std::max(atlas->TexDesiredWidth,1024);
    if(!atlas->Build())return false;
    unsigned char* pixels=nullptr;int width=0,height=0;
    atlas->GetTexDataAsRGBA32(&pixels,&width,&height);
    if(!pixels||width<=0||height<=0)return false;
    for(const auto& r:atlas->CustomRects)
        if(!r.IsPacked()||unsigned(r.X)+r.Width>unsigned(width)||unsigned(r.Y)+r.Height>unsigned(height))return false;
    for(unsigned i=0;i<2;++i){
        const auto& d=data[i];const auto* r=atlas->GetCustomRectByIndex(rectangles[i]);
        auto* font=fonts[i];font->Glyphs.clear();font->FallbackGlyph=nullptr; font->FallbackChar='?';
        font->EllipsisChar=ImWchar(-1);font->MetricsTotalSurface=0;
        font->Ascent=d.font.baseline;font->Descent=d.font.baseline-d.font.height;
        for(unsigned y=0;y<d.height;++y)for(unsigned x=0;x<d.width;++x){
            uint32_t c=d.pixels[size_t(y)*d.width+x];
            reinterpret_cast<ImU32*>(pixels)[size_t(r->Y+y)*width+r->X+x]=IM_COL32((c>>16)&255,(c>>8)&255,c&255,c>>24);
        }
        for(unsigned c=32;c<=std::min(d.font.maximum,254u);++c){
            const auto unicode=TrialFrontendCodepoint(c);
            if(c==127||(unicode=='?'&&c!='?'))continue;
            const uint8_t* g=d.font.glyphs+24*U16(d.font.mapping+c*2);
            float bearing=int16_t(U16(g+16)),glyph_width=int16_t(U16(g+18)),advance=bearing+int16_t(U16(g+20));
            font->AddGlyph(nullptr,ImWchar(unicode),bearing-.5f,-.5f,bearing-.5f+glyph_width,d.font.height-.5f,
                (r->X+F32(g)*d.width)/width,(r->Y+F32(g+4)*d.height)/height,
                (r->X+F32(g+8)*d.width)/width,(r->Y+F32(g+12)*d.height)/height,advance);
        }
        font->BuildLookupTable();
    }
    for(unsigned i=0;i<29;++i){
        const auto* r=atlas->GetCustomRectByIndex(icon_rectangles[i]);
        for(unsigned y=0;y<r->Height;++y)for(unsigned x=0;x<r->Width;++x){
            const uint32_t c=icons[i][size_t(y)*r->Width+x];
            reinterpret_cast<ImU32*>(pixels)[size_t(r->Y+y)*width+r->X+x]=IM_COL32((c>>16)&255,(c>>8)&255,c&255,c>>24);
        }
        icon_uv[i]={float(r->X)/width,float(r->Y)/height,float(r->X+r->Width)/width,float(r->Y+r->Height)/height};
    }
    icons_ready=true;body=fonts[0];title=fonts[1];return true;
}

bool TrialMenuIconUV(unsigned code,float (&uv)[4]){
    if(code>=0x101&&code<=0x105)code+=trial_controller::family.load()*0x100;
    if(icons_ready)for(unsigned i=0;i<29;++i)if(icon_codes[i]==code){
        std::copy(icon_uv[i].begin(),icon_uv[i].end(),uv);return true;
    }
    return false;
}
