#include "xbe_frontend_fonts.h"
#include "imgui.h"
#include "kernel/disc.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <utility>
#include <windows.h>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

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
int main(){
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* error)->LONG{
        std::fprintf(stderr,"Font fixture exception %08lx at executable offset %llx\n",error->ExceptionRecord->ExceptionCode,
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(error->ExceptionRecord->ExceptionAddress)-reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    std::setvbuf(stdout,nullptr,_IONBF,0);std::puts("Font fixture: startup");
    ImGui::CreateContext();auto& io=ImGui::GetIO();auto* atlas=io.Fonts;ImFont *body=nullptr,*title=nullptr;
    CHECK(!TrialLoadFrontendFonts(atlas,std::filesystem::path(/* GAME_PATH */)/"missing-font-test",body,title));
    CHECK(!body&&!title&&atlas->Fonts.empty()&&atlas->CustomRects.empty());
    std::puts("Font fixture: original assets");
    CHECK(TrialLoadFrontendFonts(atlas,/* GAME_PATH */,body,title));
    std::puts("Font fixture: metrics and atlas");
    Golden medium=Read(/* MEDIUM_PATH */),large=Read(/* LARGE_PATH */);
    CheckFont(atlas,body,medium);CheckFont(atlas,title,large);
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
    float missing_uv[4]{};CHECK(!TrialMenuIconUV(0,missing_uv));
    std::puts("Font fixture: actual glyph draw");
    atlas->SetTexID(ImTextureID(1));io.DisplaySize=ImVec2(1280,720);io.DeltaTime=1.f/60;
    ImGui::NewFrame();DrawChecks(body,medium);DrawChecks(title,large);ImGui::EndFrame();
    ImGui::DestroyContext();
    std::puts("PASS original medium/large font archive loading, ASCII/Latin-1/CP1252 glyph metrics/UVs, full RGBA atlas pixels, actual ImGui glyph vertices and missing-asset startup fallback");
}
