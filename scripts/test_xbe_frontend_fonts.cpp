#include "xbe_frontend_fonts.h"
#include "xbe_hd_ui.h"
#include "imgui.h"
#include "kernel/disc.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <windows.h>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool hd_selected=false;
namespace trial_graphics {bool HdUi(){return hd_selected;}}

// The fixture reads actual extracted user archives. Disc loading is exercised
// through the production disc owner during game startup, not faked here.
namespace kernel {
const Disc* MountedDisc(){return nullptr;}
const Disc::Entry* Disc::Find(std::string_view) const{return nullptr;}
bool Disc::Load(std::string_view,std::vector<uint8_t>&)const{return false;}
}
struct Golden {
    float nominal=0,line=0,baseline=0;uint32_t width=0,height=0;
    float glyph[223][7]{};std::vector<uint8_t> pixels;
};
static Golden Read(const wchar_t* path){
    std::ifstream file(path,std::ios::binary);CHECK(file.good());Golden g;
    CHECK(file.read(reinterpret_cast<char*>(&g.nominal),4));
    CHECK(file.read(reinterpret_cast<char*>(&g.line),4));
    CHECK(file.read(reinterpret_cast<char*>(&g.baseline),4));
    CHECK(file.read(reinterpret_cast<char*>(&g.width),4));
    CHECK(file.read(reinterpret_cast<char*>(&g.height),4));
    CHECK(file.read(reinterpret_cast<char*>(g.glyph),sizeof(g.glyph)));
    g.pixels.resize(size_t(g.width)*g.height*4);CHECK(file.read(reinterpret_cast<char*>(g.pixels.data()),g.pixels.size()));
    CHECK(file.peek()==std::ifstream::traits_type::eof());return g;
}
static bool Near(float a,float b){return std::abs(a-b)<.0001f;}
static Golden HdGolden(const Golden& original,const char* name){
    const auto* image=uc_hd_ui::Font(name);CHECK(image);
    CHECK(image->width==original.width*4&&image->height==original.height*4);
    Golden result=original;result.width=image->width;result.height=image->height;
    result.pixels=image->bgra;
    for(size_t i=0;i<result.pixels.size();i+=4)std::swap(result.pixels[i],result.pixels[i+2]);
    return result;
}
static void CheckFont(ImFontAtlas* atlas,ImFont* font,const Golden& golden){
    CHECK(font&&Near(font->FontSize,golden.nominal)&&Near(font->Ascent,golden.baseline));
    const ImFontAtlasCustomRect* rect=nullptr;
    for(const auto& r:atlas->CustomRects)if(r.Width==golden.width&&r.Height==golden.height){CHECK(!rect);rect=&r;}
    CHECK(rect&&rect->IsPacked());
    unsigned char* pixels;int width,height;atlas->GetTexDataAsRGBA32(&pixels,&width,&height);
    for(unsigned y=0;y<golden.height;++y)
        CHECK(!std::memcmp(pixels+((rect->Y+y)*width+rect->X)*4,golden.pixels.data()+size_t(y)*golden.width*4,golden.width*4));
    for(unsigned c=32;c<=254;++c){
        if(c>=127&&c<160)continue;
        const auto* glyph=font->FindGlyphNoFallback(ImWchar(c));const auto* e=golden.glyph[c-32];
        CHECK(glyph&&Near(glyph->AdvanceX,e[0]));
        CHECK(Near(glyph->X0,e[1]-.5f)&&Near(glyph->X1,e[1]-.5f+e[2]));
        CHECK(Near(glyph->Y0,-.5f)&&Near(glyph->Y1,golden.nominal-.5f));
        CHECK(Near(glyph->U0,(rect->X+e[3]*golden.width)/width)&&Near(glyph->V0,(rect->Y+e[4]*golden.height)/height));
        CHECK(Near(glyph->U1,(rect->X+e[5]*golden.width)/width)&&Near(glyph->V1,(rect->Y+e[6]*golden.height)/height));
        CHECK(glyph->Colored==false);
    }
    for(const auto pair: {std::pair<unsigned,unsigned>{0x8c,0x0152},{0x9c,0x0153},{0x93,0x201c},{0x94,0x201d},{0x96,0x2013}}){
        const auto* glyph=font->FindGlyphNoFallback(ImWchar(pair.second));const auto* e=golden.glyph[pair.first-32];
        CHECK(glyph&&Near(glyph->AdvanceX,e[0]));
        CHECK(Near(glyph->X0,e[1]-.5f)&&Near(glyph->X1,e[1]-.5f+e[2]));
    }
    CHECK(font->FallbackGlyph==font->FindGlyphNoFallback('?'));
    CHECK(!font->FindGlyphNoFallback(' ')->Visible);
    float advance=0;for(const char* s="SELECT ASSIGNMENT";*s;++s)advance+=golden.glyph[*s-32][0];
    CHECK(Near(font->CalcTextSizeA(font->FontSize,FLT_MAX,0,"SELECT ASSIGNMENT").x,advance));
}
static void DrawChecks(ImFont* font,const Golden& golden){
    auto* draw=ImGui::GetForegroundDrawList();int start=draw->VtxBuffer.Size;
    draw->AddText(font,font->FontSize,ImVec2(10,10),IM_COL32_WHITE,"GRAPHICS");
    CHECK(draw->VtxBuffer.Size-start==8*4);float x=10;unsigned i=0;
    for(const char* s="GRAPHICS";*s;++s,++i){
        const auto* g=golden.glyph[*s-32];auto* v=&draw->VtxBuffer[start+i*4];
        CHECK(Near(v[0].pos.x,x+g[1]-.5f)&&Near(v[0].pos.y,9.5f));
        CHECK(Near(v[2].pos.x,x+g[1]-.5f+g[2])&&Near(v[2].pos.y,9.5f+golden.nominal));
        x+=g[0];
    }
    // UTF-8 must select the exact original accented byte glyphs, not '?'
    // fallback or two separate characters. Exercise actual ImGui vertices.
    start=draw->VtxBuffer.Size;x=10;
    draw->AddText(font,font->FontSize,ImVec2(10,60),IM_COL32_WHITE,"\xc3\xa9\xc3\xb2\xc3\xbc\xc3\x9f\xc3\xa7");
    CHECK(draw->VtxBuffer.Size-start==5*4);
    const unsigned accents[]={0xe9,0xf2,0xfc,0xdf,0xe7};
    for(unsigned j=0;j<5;++j){const auto* g=golden.glyph[accents[j]-32];const auto* v=&draw->VtxBuffer[start+j*4];
        CHECK(Near(v[0].pos.x,x+g[1]-.5f)&&Near(v[2].pos.x,x+g[1]-.5f+g[2]));
        const auto* glyph=font->FindGlyphNoFallback(ImWchar(accents[j]));CHECK(glyph);
        CHECK(Near(v[0].uv.x,glyph->U0)&&Near(v[0].uv.y,glyph->V0));x+=g[0];}
}
static void MissingHostFontChecks(){
    // A fresh process has usable HUD art but no ABC metrics. Host-font recovery
    // must not prevent the independently prepared HUD/button images from loading.
    const auto directory=std::filesystem::temp_directory_path()/
        ("uc-hd-font-fallback-"+std::to_string(GetCurrentProcessId()));
    CHECK(!std::filesystem::exists(directory));
    CHECK(std::filesystem::create_directory(directory));
    const auto archive=directory/"PersistantFilesXBox.lst";
    CHECK(std::filesystem::copy_file(std::filesystem::path(/* GAME_PATH */)/"PersistantFilesXBox.lst",archive));
    ImGui::CreateContext();auto* atlas=ImGui::GetIO().Fonts;ImFont *body=nullptr,*title=nullptr;
    CHECK(!uc_hd_ui::Ready());
    const bool loaded=TrialLoadFrontendFonts(atlas,directory,body,title);
    CHECK(std::filesystem::remove(archive));CHECK(std::filesystem::remove(directory));
    CHECK(!loaded&&!body&&!title&&atlas->Fonts.empty()&&atlas->CustomRects.empty());
    CHECK(uc_hd_ui::Ready()&&uc_hd_ui::Buttons());
    CHECK(uc_hd_ui::Buttons()->width==128&&uc_hd_ui::Buttons()->height==4096);
    ImGui::DestroyContext();
    std::puts("PASS missing host-font metrics preserve prepared HD HUD/button art");
}
int main(int argc,char** argv){
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* error)->LONG{
        std::fprintf(stderr,"Font fixture exception %08lx at executable offset %llx\n",error->ExceptionRecord->ExceptionCode,
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(error->ExceptionRecord->ExceptionAddress)-reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    if(argc==2&&!std::strcmp(argv[1],"--font-fallback")){MissingHostFontChecks();return 0;}
    CHECK(argc==1);
    std::setvbuf(stdout,nullptr,_IONBF,0);std::puts("Font fixture: startup");
    ImGui::CreateContext();auto& io=ImGui::GetIO();auto* atlas=io.Fonts;ImFont *body=nullptr,*title=nullptr;
    std::puts("Font fixture: original assets");
    CHECK(TrialLoadFrontendFonts(atlas,/* GAME_PATH */,body,title));
    std::puts("Font fixture: metrics and atlas");
    Golden medium=Read(/* MEDIUM_PATH */),large=Read(/* LARGE_PATH */);
    CheckFont(atlas,body,medium);CheckFont(atlas,title,large);
    CHECK(uc_hd_ui::Ready());
    hd_selected=true;
    auto* hd_body=TrialFrontendFontForUi(body);auto* hd_title=TrialFrontendFontForUi(title);
    CHECK(hd_body!=body&&hd_title!=title&&hd_body!=hd_title);
    Golden hd_medium=HdGolden(medium,"medium"),hd_large=HdGolden(large,"large");
    CheckFont(atlas,hd_body,hd_medium);CheckFont(atlas,hd_title,hd_large);
    hd_selected=false;
    CHECK(TrialFrontendFontForUi(body)==body&&TrialFrontendFontForUi(title)==title);
    for(unsigned code:{13u,unsigned('Q'),unsigned('D'),unsigned('R'),37u,39u,1u,27u,2u,0x101u,0x102u,0x103u,0x104u,0x105u}){
        float uv[4]{};CHECK(TrialMenuIconUV(code,uv));
        CHECK(uv[0]>=0&&uv[1]>=0&&uv[2]<=1&&uv[3]<=1&&uv[2]>uv[0]&&uv[3]>uv[1]);
        unsigned visible=0;
        for(int y=int(uv[1]*atlas->TexHeight);y<int(uv[3]*atlas->TexHeight);++y)
            for(int x=int(uv[0]*atlas->TexWidth);x<int(uv[2]*atlas->TexWidth);++x)
                visible+=atlas->TexPixelsRGBA32[y*atlas->TexWidth+x]>>24?1:0;
        CHECK(visible>100);
    }
    float previous_uv[4]{};CHECK(TrialMenuIconUV(0x101,previous_uv));
    for(unsigned family=1;family<=3;++family){
        trial_controller::family=family;
        for(unsigned code=0x101;code<=0x105;++code){
            float uv[4]{};CHECK(TrialMenuIconUV(code,uv));
            CHECK(uv[2]>uv[0]&&uv[3]>uv[1]);
            if(code==0x101){CHECK(std::memcmp(previous_uv,uv,sizeof(uv)));std::memcpy(previous_uv,uv,sizeof(uv));}
        }
    }
    trial_controller::family=0;
    const auto* atlas_pixels=atlas->TexPixelsRGBA32;
    const int atlas_width=atlas->TexWidth,atlas_height=atlas->TexHeight,atlas_rects=atlas->CustomRects.Size;
    for(unsigned code:{13u,1u,0x101u,0x102u,0x103u,0x104u,0x105u,0x201u,0x301u,0x401u}){
        float original[4],hd[4];hd_selected=false;CHECK(TrialMenuIconUV(code,original));
        hd_selected=true;CHECK(TrialMenuIconUV(code,hd));
        if(code>=0x101&&code<=0x105){
            CHECK(Near((hd[2]-hd[0])*atlas_width,128)&&Near((hd[3]-hd[1])*atlas_height,128));
            CHECK(Near((original[2]-original[0])*atlas_width,32)&&Near((original[3]-original[1])*atlas_height,32));
        }else CHECK(!std::memcmp(original,hd,sizeof(hd))); // Native PC/PS/Nintendo artwork stays 128px.
    }
    // Navigation uses the same authored D-pad for every controller family.
    float hd_dpad[4]{};hd_selected=true;CHECK(TrialMenuIconUV(0x103,hd_dpad));
    const auto* buttons=uc_hd_ui::Buttons();CHECK(buttons&&buttons->width==128);
    // Compare actual pixels, not just sizes: host codes are A,B,D-pad,X,Y.
    const unsigned source_rows[]={1,4,5,3,2};
    for(unsigned i=0;i<5;++i){
        float uv[4]{};CHECK(TrialMenuIconUV(0x101+i,uv));
        const unsigned left=unsigned(std::lround(uv[0]*atlas_width)),top=unsigned(std::lround(uv[1]*atlas_height));
        for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x){
            const auto* p=buttons->bgra.data()+((size_t(source_rows[i])*128+y)*128+x)*4;
            CHECK(atlas_pixels[(size_t(top+y)*atlas_width+left+x)]==IM_COL32(p[2],p[1],p[0],p[3]));
        }
    }
    for(unsigned code:{0x203u,0x303u,0x403u}){
        float original[4]{},hd[4]{};
        hd_selected=false;CHECK(TrialMenuIconUV(code,original));
        hd_selected=true;CHECK(TrialMenuIconUV(code,hd));
        CHECK(!std::memcmp(hd,hd_dpad,sizeof(hd))&&std::memcmp(original,hd,sizeof(hd)));
    }
    for(bool enabled:{false,true,false,true}){
        hd_selected=enabled;CHECK(TrialFrontendFontForUi(body)==(enabled?hd_body:body));
        CHECK(TrialFrontendFontForUi(title)==(enabled?hd_title:title));
        CHECK(atlas->TexPixelsRGBA32==atlas_pixels&&atlas->TexWidth==atlas_width&&
              atlas->TexHeight==atlas_height&&atlas->CustomRects.Size==atlas_rects);
    }
    hd_selected=false;
    float missing_uv[4]{};CHECK(!TrialMenuIconUV(0,missing_uv));
    std::puts("Font fixture: actual glyph draw");
    atlas->SetTexID(ImTextureID(1));io.DisplaySize=ImVec2(1280,720);io.DeltaTime=1.f/60;
    ImGui::NewFrame();DrawChecks(body,medium);DrawChecks(title,large);
    DrawChecks(hd_body,medium);DrawChecks(hd_title,large);ImGui::EndFrame();
    ImFontAtlas missing_atlas;ImFont *missing_body=nullptr,*missing_title=nullptr;
    CHECK(!TrialLoadFrontendFonts(&missing_atlas,std::filesystem::path(/* GAME_PATH */)/"missing-font-test",missing_body,missing_title));
    CHECK(!missing_body&&!missing_title&&missing_atlas.Fonts.empty()&&missing_atlas.CustomRects.empty());
    ImGui::DestroyContext();
    std::puts("PASS original/HD medium/large atlas pixels, unchanged ASCII/Latin-1/CP1252 glyph metrics and draw vertices, resident toggle, button variants and missing-asset startup fallback");
}
