#include "xbe_hd_ui_reconstruct.h"
extern "C" {
#include "native/font.h"
#include "native/texture_asset/texture_asset.h"
}
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

static size_t checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
namespace r=uc_hd_ui::reconstruct;
namespace f=uc_hd_ui::filter;
static void Synthetic() {
    std::vector<uint8_t> source(12*10*4,0);
    for(unsigned y=2;y<8;++y)for(unsigned x=2;x<8;++x) {
        auto* p=source.data()+(y*12+x)*4;
        const bool edge=x==2||x==7||y==2||y==7;
        p[0]=edge?0:210;p[1]=edge?0:190;p[2]=edge?0:170;p[3]=255;
    }
    const std::array<r::Cell,1> cells{{{0,0,9,10}}};
    auto old=f::Upscale(12,10,source,4),next=old;
    CHECK(r::detail::Face(12,10,source,cells,next));CHECK(next!=old);
    unsigned changed=0;
    for(unsigned y=0;y<40;++y)for(unsigned x=0;x<48;++x) {
        const size_t i=(y*48+x)*4;CHECK(next[i+3]==old[i+3]);
        if(x>=36)CHECK(!std::memcmp(next.data()+i,old.data()+i,4));
        if(next[i]!=old[i])++changed;
        if(next[i+3])CHECK(next[i]>=next[i+1]&&next[i+1]>=next[i+2]);
    }
    CHECK(changed>0);
    for(auto bad:std::array<r::Cell,7>{{{-1,0,9,10},{0,0,13,10},{0,0,9,11},
        {0,0,0,10},{0,0,9,0},{.5f,0,9,10},{0,0,std::numeric_limits<float>::quiet_NaN(),10}}}) {
        next=old;CHECK(!r::detail::Face(12,10,source,{&bad,1},next));CHECK(next==old);
    }
    const std::array<r::Cell,2> overlapping{{{0,0,9,10},{8,0,12,10}}};
    next=old;CHECK(!r::detail::Face(12,10,source,overlapping,next));CHECK(next==old);
    CHECK(!r::detail::Face(12,10,source,{},next));CHECK(next==old);
    source.pop_back();CHECK(!r::detail::Face(12,10,source,cells,next));CHECK(next==old);
    CHECK(!r::Hud(nullptr,12,10,source,4,next));CHECK(next==old);
    CHECK(!r::FontFace("medium",12,10,source,cells,next));CHECK(next==old);
}
using Archive=std::unique_ptr<UCArchive,decltype(&uc_archive_close)>;
static Archive Open(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);CHECK(file.good());
    const auto bytes=file.tellg();CHECK(bytes>0&&bytes<16*1024*1024);
    std::vector<uint8_t> data(size_t(bytes),0);file.seekg(0);CHECK(file.read(reinterpret_cast<char*>(data.data()),data.size()));
    UCArchive* archive=nullptr;CHECK(uc_archive_copy(data.data(),data.size(),&archive)==UC_AR_OK);return {archive,uc_archive_close};
}
static std::vector<uint8_t> Read(const UCArchive* archive,const std::string& wanted) {
    for(size_t i=0;i<uc_archive_count(archive);++i) {
        const auto* entry=uc_archive_entry(archive,i);std::string name(reinterpret_cast<const char*>(entry->name));
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(c>='A'&&c<='Z'?c+'a'-'A':c);});
        if(name!=wanted)continue;
        UCArchiveBytes bytes{};CHECK(uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes)==UC_AR_OK);
        std::vector<uint8_t> result(bytes.data,bytes.data+bytes.size);uc_archive_bytes_free(&bytes);return result;
    }
    CHECK(false);return {};
}
static std::vector<uint8_t> Texture(const UCArchive* archive,const char* name,uint32_t& width,uint32_t& height) {
    const auto bytes=Read(archive,name);UCTextureAsset texture{};
    CHECK(uc_texture_decode(bytes.data(),bytes.size(),&texture)==UC_TEXTURE_OK);
    width=texture.width;height=texture.height;std::vector<uint8_t> result;
    CHECK(texture.format==UC_TEXTURE_BC3_UNORM);
    CHECK(f::DecodeNative(width,height,15,{texture.mips[0].data,texture.mips[0].slice_pitch},result));
    uc_texture_free(&texture);return result;
}
static float Float(const uint8_t* p){float value;std::memcpy(&value,p,4);return value;}
static std::vector<r::Cell> Cells(const UCArchive* archive,const char* name,uint32_t width,uint32_t height) {
    const auto bytes=Read(archive,std::string("fonts\\")+name+".abc");UCFont font{};
    CHECK(!uc_font_read(bytes.data(),bytes.size(),&font));std::vector<r::Cell> result;
    for(uint32_t i=0;i<font.count;++i) {
        const auto* glyph=font.glyphs+i*24;
        r::Cell cell{Float(glyph)*width,Float(glyph+4)*height,Float(glyph+8)*width,Float(glyph+12)*height};
        CHECK(std::abs(cell.x1-cell.x0-int16_t(f::detail::U16(glyph+18)))<.01f);
        CHECK(std::abs(cell.y1-cell.y0-font.height)<.01f);
        if(cell.x0==cell.x1)continue;
        if(std::any_of(result.begin(),result.end(),[&](auto c){return c.x0==cell.x0&&c.y0==cell.y0&&c.x1==cell.x1&&c.y1==cell.y1;}))continue;
        result.push_back(cell);
    }
    return result;
}
static void Preview(const std::filesystem::path& path,uint32_t width,uint32_t height,
    const std::vector<uint8_t>& old,const std::vector<uint8_t>& next,bool full=false) {
    // Left: previous 4x filtered/alpha-contoured output. Right: this treatment.
    const unsigned side=full?width*4:std::min(width*4,1024u),h=full?height*4:std::min(height*4,384u),w=side*2;
    std::vector<uint8_t> bmp(54+size_t(w)*h*4,0);
    auto w16=[&](size_t p,uint16_t n){std::memcpy(bmp.data()+p,&n,2);};
    auto w32=[&](size_t p,uint32_t n){std::memcpy(bmp.data()+p,&n,4);};
    bmp[0]='B';bmp[1]='M';w32(2,uint32_t(bmp.size()));w32(10,54);w32(14,40);
    w32(18,w);w32(22,uint32_t(-int(h)));w16(26,1);w16(28,32);w32(34,w*h*4);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
        const auto* p=(x<side?old:next).data()+(size_t(y)*width*4+x%side)*4;
        auto* q=bmp.data()+54+(size_t(y)*w+x)*4;const unsigned bg=((x%side/16+y/16)&1)?40:65;
        for(unsigned c=0;c<3;++c)q[c]=uint8_t((unsigned(p[c])*p[3]+bg*(255-p[3]))/255);q[3]=255;
    }
    std::ofstream file(path,std::ios::binary);CHECK(file.good());file.write(reinterpret_cast<char*>(bmp.data()),bmp.size());CHECK(file.good());
}
static void Real(const std::filesystem::path& game,const std::filesystem::path& previews) {
    const auto hud=Open(game/"40-01.ent"),fonts=Open(game/"PersistantFilesXBox.lst"),metrics=Open(game/"dialoguefonts.lst");
    const auto menu=Open(game/"99-01.ent"),operations=Open(game/"9-01.ent");
    if(!previews.empty())std::filesystem::create_directories(previews);
    for(const auto& asset:r::detail::kHud) {
        uint32_t w,h;auto source=Texture(hud.get(),asset.name,w,h);
        CHECK(w==asset.width&&h==asset.height);CHECK(r::detail::Hash(source)==asset.pixels);
        const auto old=f::Upscale(w,h,source,4);auto next=old;
        CHECK(r::Hud(asset.name,w,h,source,4,next));CHECK(next.size()==old.size());CHECK(next!=old);
        uint64_t before_area=0,after_area=0;size_t changed=0;
        for(size_t i=0;i<source.size();i+=4)before_area+=source[i+3];
        for(size_t i=0;i<next.size();i+=4){after_area+=next[i+3];changed+=std::memcmp(next.data()+i,old.data()+i,4)!=0;}
        CHECK(changed>0);
        if(w!=256)CHECK(std::abs(double(after_area)-double(before_area)*16)<double(before_area)*16*.015);
        else {
            for(unsigned y=0;y<h*4;++y)for(unsigned x=0;x<w*4;++x)if(x>=190*4||y>=27*4) {
                const size_t i=(size_t(y)*w*4+x)*4;CHECK(!std::memcmp(next.data()+i,old.data()+i,4));
            }
        }
        auto rejected=old;source[0]^=1;CHECK(!r::Hud(asset.name,w,h,source,4,rejected));CHECK(rejected==old);source[0]^=1;
        CHECK(!r::Hud(asset.name,w+1,h,source,4,rejected));CHECK(rejected==old);
        CHECK(!r::Hud(asset.name,w,h,source,2,rejected));CHECK(rejected==old);
        std::printf("HUD %s: %zu changed pixels; alpha original=%llu reconstructed=%llu\n",asset.name,changed,
            static_cast<unsigned long long>(before_area),static_cast<unsigned long long>(after_area));
        if(!previews.empty())Preview(previews/(std::string(asset.name+4).substr(0,std::strlen(asset.name+4)-13)+".bmp"),w,h,old,next);
    }
    for(const auto& asset:r::detail::kFonts) {
        uint32_t w,h;auto source=Texture(fonts.get(),(std::string("fonts\\")+asset.name+"%4b%.tga").c_str(),w,h);
        CHECK(w==asset.width&&h==asset.height);CHECK(r::detail::Hash(source)==asset.pixels);
        const auto cells=Cells(metrics.get(),asset.name,w,h);auto old=f::Upscale(w,h,source,4);
        CHECK(r::detail::Valid(w,h,source,cells,old));
        for(const auto cell:cells)f::font_detail::RestoreCell(w,source,cell,old);
        auto next=old;CHECK(r::FontFace(asset.name,w,h,source,cells,next));CHECK(next!=old);
        std::vector<uint8_t> inside(size_t(w)*h*16,0);
        for(const auto cell:cells)for(unsigned y=unsigned(cell.y0*4);y<unsigned(cell.y1*4);++y)
            for(unsigned x=unsigned(cell.x0*4);x<unsigned(cell.x1*4);++x)inside[size_t(y)*w*4+x]=1;
        size_t changed=0;
        for(size_t i=0;i<next.size();i+=4){CHECK(next[i+3]==old[i+3]);
            if(!inside[i/4])CHECK(!std::memcmp(next.data()+i,old.data()+i,4));
            changed+=std::memcmp(next.data()+i,old.data()+i,4)!=0;}
        CHECK(changed>0);auto rejected=old;source[0]^=1;
        CHECK(!r::FontFace(asset.name,w,h,source,cells,rejected));CHECK(rejected==old);source[0]^=1;
        CHECK(!r::FontFace("time",w,h,source,cells,rejected));CHECK(rejected==old);
        std::printf("FONT %s: %zu glyph cells, %zu changed pixels; alpha/unused cells exact\n",asset.name,cells.size(),changed);
        if(!previews.empty())Preview(previews/(std::string(asset.name)+".bmp"),w,h,old,next);
    }
    auto symbol=[&](const r::detail::Asset& asset,bool keep_alpha) {
        const std::string name=asset.name;
        const auto* archive=name.starts_with("font\\")?fonts.get():name.starts_with("frontend\\")?menu.get():
            name.find("radar")!=std::string::npos?hud.get():operations.get();
        uint32_t w,h;const auto source=Texture(archive,asset.name,w,h);
        CHECK(w==asset.width&&h==asset.height);CHECK(r::detail::Hash(source)==asset.pixels);
        const auto old=f::Upscale(w,h,source,4);auto next=old;
        CHECK(r::Hud(asset.name,w,h,source,4,next));CHECK(next!=old);size_t changed=0;
        for(unsigned y=0;y<h*4;++y)for(unsigned x=0;x<w*4;++x) {
            const size_t i=(size_t(y)*w*4+x)*4;
            if(keep_alpha)CHECK(next[i+3]==old[i+3]);
            if(h==1024) {
                const unsigned row=y/128,rx=x,ry=y%128;
                const bool inside=row==1?rx>=36&&rx<96&&ry>=44&&ry<96:
                    row==2?rx>=36&&rx<92&&ry>=48&&ry<100:
                    row==3?rx>=36&&rx<96&&ry>=44&&ry<100:
                    row==4?rx>=36&&rx<96&&ry>=40&&ry<96:false;
                if(!inside)CHECK(!std::memcmp(old.data()+i,next.data()+i,4));
            }
            changed+=std::memcmp(old.data()+i,next.data()+i,4)!=0;
        }
        CHECK(changed>0);auto rejected=old;auto wrong=source;wrong[0]^=1;
        CHECK(!r::Hud(asset.name,w,h,wrong,4,rejected));CHECK(rejected==old);
        CHECK(!r::Hud(asset.name,w,h,source,2,rejected));CHECK(rejected==old);
        std::printf("SYMBOL %s: %zu changed pixels; alpha %s\n",asset.name,changed,keep_alpha?"exact":"contoured");
        if(!previews.empty()) {
            const auto stem=name.substr(name.find_last_of('\\')+1);
            if(h!=1024)Preview(previews/(stem+".bmp"),w,h,old,next);
            else for(unsigned row=0;row<8;++row) {
                const size_t begin=size_t(row)*128*128*4,end=begin+128*128*4;
                Preview(previews/("button-"+std::to_string(row)+".bmp"),32,32,
                    {old.begin()+begin,old.begin()+end},{next.begin()+begin,next.begin()+end});
            }
        }
    };
    for(const auto& asset:r::detail::kSymbols)symbol(asset,true);
    for(const auto& asset:r::detail::kArrows)symbol(asset,false);
    for(const auto& asset:r::detail::kLettering) {
        uint32_t w,h;const auto source=Texture(menu.get(),asset.name,w,h);
        CHECK(w==asset.width&&h==asset.height);CHECK(r::detail::Hash(source)==asset.pixels);
        CHECK(r::LetteringScale4(asset.name,w,h));CHECK(!r::LetteringScale4(asset.name,w+1,h));
        const auto old=f::Upscale(w,h,source,4);auto next=old;
        const bool title=&asset==&r::detail::kLettering[0];
        CHECK(r::Hud(asset.name,w,h,source,4,next)==title);
        CHECK((next!=old)==title);size_t changed=0;
        for(unsigned y=0;y<h*4;++y)for(unsigned x=0;x<w*4;++x) {
            const size_t i=(size_t(y)*w*4+x)*4;
            if(title) {
                CHECK(next[i+3]==old[i+3]);
                if(x<18*4||x>=237*4||y<215*4||y>=240*4)
                    CHECK(!std::memcmp(old.data()+i,next.data()+i,4));
            }
            changed+=std::memcmp(old.data()+i,next.data()+i,4)!=0;
        }
        CHECK((changed>0)==title);
        auto rejected=old,wrong=source;wrong[0]^=1;
        CHECK(!r::Hud(asset.name,w,h,wrong,4,rejected));CHECK(rejected==old);
        CHECK(!r::Hud(asset.name,w,h,source,2,rejected));CHECK(rejected==old);
        std::printf("LETTERING %s: %zu changed pixels; %s\n",asset.name,changed,
            title?"alpha and colored title pixels exact":"4x filtering retained; sparse-alpha contour rejected");
        if(!previews.empty()) {
            if(title) {
                const size_t begin=size_t(212*4)*w*4*4,end=size_t(244*4)*w*4*4;
                Preview(previews/"title-subtitle.bmp",w,32,{old.begin()+begin,old.begin()+end},
                    {next.begin()+begin,next.begin()+end},true);
            } else Preview(previews/(std::string(asset.name+21)+".bmp"),w,h,old,next,true);
        }
    }
    CHECK(!r::LetteringScale4(nullptr,256,256));
    for(const char* name:{"frontend\\frt_content\\fe_simlogo%24b%.tga", "frontend\\frt_legal\\powered_by_gamespy_web.tga"}) {
        uint32_t w,h;const auto source=Texture(menu.get(),name,w,h);auto next=f::Upscale(w,h,source,4);const auto old=next;
        CHECK(!r::LetteringScale4(name,w,h));CHECK(!r::Hud(name,w,h,source,4,next));CHECK(next==old);
    }
    // Credits/debug are white except negligible BC alpha noise: their existing
    // validated alpha contours already provide the shape, with no inner face.
    for(const char* name:{"credits","debug"}) {
        uint32_t w,h;const auto source=Texture(fonts.get(),(std::string("fonts\\")+name+"%4b%.tga").c_str(),w,h);
        const auto cells=Cells(metrics.get(),name,w,h);auto old=f::Upscale(w,h,source,4);
        CHECK(r::detail::Valid(w,h,source,cells,old));
        for(const auto cell:cells)f::font_detail::RestoreCell(w,source,cell,old);
        for(size_t i=0;i<source.size();i+=4)if(source[i+3]>4)CHECK(std::min({source[i],source[i+1],source[i+2]})>=245);
        auto next=old;CHECK(!r::FontFace(name,w,h,source,cells,next));CHECK(next==old);
        std::printf("FONT %s: %zu validated glyph cells; existing alpha reconstruction retained\n",name,cells.size());
    }
    // Soft dot, heartbeat backing and malformed time metrics are not hard masks.
    for(const char* name:{"hud\\hud_crosshair_dot.tga=nomipmap","hud\\hud_heartbeatbg.tga=nomipmap"}) {
        uint32_t w,h;auto source=Texture(hud.get(),name,w,h);const auto old=f::Upscale(w,h,source,4);auto next=old;
        CHECK(!r::Hud(name,w,h,source,4,next));CHECK(next==old);
    }
}
int main(int argc,char** argv) {
    Synthetic();CHECK(argc>=2);
    if(std::string(argv[1])!="--synthetic")Real(argv[1],argc>2?argv[2]:"");
    std::printf("PASS HD UI reconstruction: %zu checks\n",checks);
}
