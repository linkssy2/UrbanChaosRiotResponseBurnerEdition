#include "xbe_hd_ui_restored.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace uc_hd_ui;
using namespace uc_hd_ui::restored;
static size_t checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::string Hex(std::span<const uint8_t> bytes) {
    std::array<uint8_t,32> hash{};CHECK(Digest(bytes,hash));
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto byte:hash){out+=digits[byte>>4];out+=digits[byte&15];}return out;
}
static std::vector<uint8_t> Read(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);CHECK(bool(file));
    std::vector<uint8_t> bytes(size_t(file.tellg()));file.seekg(0);
    CHECK(bool(file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))));return bytes;
}
static void Write(const std::filesystem::path& path,std::span<const uint8_t> bytes) {
    std::ofstream file(path,std::ios::binary);CHECK(bool(file));
    CHECK(bool(file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()))));
}
static void Png(const std::filesystem::path& path,unsigned width,unsigned height,const std::vector<uint8_t>& pixels) {
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> frame;ComPtr<IPropertyBag2> properties;
    CHECK(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    CHECK(SUCCEEDED(factory->CreateStream(&stream)));
    CHECK(SUCCEEDED(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE)));
    CHECK(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder)));
    CHECK(SUCCEEDED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)));
    CHECK(SUCCEEDED(encoder->CreateNewFrame(&frame,&properties)));
    CHECK(SUCCEEDED(frame->Initialize(properties.Get())));CHECK(SUCCEEDED(frame->SetSize(width,height)));
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
    CHECK(SUCCEEDED(frame->SetPixelFormat(&format))&&format==GUID_WICPixelFormat32bppBGRA);
    CHECK(SUCCEEDED(frame->WritePixels(height,width*4,UINT(pixels.size()),const_cast<BYTE*>(pixels.data()))));
    CHECK(SUCCEEDED(frame->Commit()));CHECK(SUCCEEDED(encoder->Commit()));
}
static bool Same(const Image& a,const Image& b){return a.width==b.width&&a.height==b.height&&a.bgra==b.bgra;}
static void Identity() {
    const std::array<uint8_t,3> source{'a','b','c'};
    const char* digest="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    CHECK(Hex(source)==digest);CHECK(MatchesDigest(source,digest));
    CHECK(!MatchesDigest(source,""));CHECK(!MatchesDigest(source,std::string(64,'z')));
    Artwork row{"hud\\fixture.tga",2,2,15,3,digest,"fixture.png",digest};
    const std::array rows{row};
    CHECK(Find(rows,row.name,2,2,15,source)==&rows[0]);
    CHECK(!Find(rows,"hud\\other.tga",2,2,15,source));
    CHECK(!Find(rows,row.name,3,2,15,source));CHECK(!Find(rows,row.name,2,3,15,source));
    CHECK(!Find(rows,row.name,2,2,12,source));CHECK(!Find(rows,row.name,2,2,15,std::span(source).first(2)));
    auto changed=source;changed.back()^=1;CHECK(!Find(rows,row.name,2,2,15,changed));
    for(const char* bad:{"../fixture.png","..\\fixture.png","sub/fixture.png","sub\\fixture.png",
                        "C:fixture.png","fixture.PNG","fixture.jpg","f:stream.png","",".\xff.png"})CHECK(!FileName(bad));
    CHECK(FileName("rewards-01_restored.png"));CHECK(!FileName(std::string(129,'a')+".png"));
    for(const char* name:{"lc_medal01.tga","lc_medal02.tga","lc_medal03.tga","lc_medal04.tga",
                         "lc_medal01smg.tga","reward_star.tga"})CHECK(BoundSprite(std::string("frontend\\frt_content\\")+name));
    for(const char* name:{"frontend\\frt_content\\level_image_01sm.tga","frontend\\other\\reward_star.tga",
                         "frontend\\frt_content\\lc_medal05.tga","frontend\\frt_content\\reward_star.tga.extra"})CHECK(!BoundSprite(name));
    Crop crop{};CHECK(Bounds(row,8,8,crop)&&crop.width==8&&crop.height==8);
    CHECK(Bounds(row,16,16,crop));CHECK(!Bounds(row,8,12,crop));CHECK(!Bounds(row,7,8,crop));
    CHECK(Bounds(row,6,6,crop));CHECK(Bounds(row,4,4,crop));CHECK(!Bounds(row,3,3,crop));
    CHECK(!Bounds(row,8193,8193,crop));CHECK(!Bounds(row,8192,8192,crop));
    auto menu=row;menu.width=menu.height=512;CHECK(Bounds(menu,1254,1254,crop));
    auto hud=row;hud.width=64;hud.height=32;CHECK(Bounds(hud,1760,880,crop));
    CHECK(!Bounds(menu,1254,1255,crop));CHECK(!Bounds(hud,1760,881,crop));
    hud.placement=Placement::SourceBounds;CHECK(Bounds(hud,1356,1159,crop));
    for(Crop bad:std::array{Crop{1,0,0,0},Crop{9,0,8,8},Crop{0,9,8,8},Crop{0,0,UINT_MAX,8},Crop{1,1,8,8}}) {
        row.crop=bad;CHECK(!Bounds(row,8,8,crop));
    }
    row.crop={4,4,8,8};CHECK(Bounds(row,16,16,crop));
}
static void PlacementChecks(const std::filesystem::path& directory) {
    std::vector<uint8_t> original(8*4*4),canvas(32*16*4);
    auto fill=[](std::vector<uint8_t>& pixels,unsigned width,Crop rect,uint8_t alpha) {
        for(unsigned y=rect.y;y<rect.y+rect.height;++y)for(unsigned x=rect.x;x<rect.x+rect.width;++x) {
            auto* p=pixels.data()+(size_t(y)*width+x)*4;
            p[0]=19;p[1]=53;p[2]=201;p[3]=alpha;
        }
    };
    fill(original,8,{1,1,4,2},255);original[(1*8+2)*4+3]=0;
    original[3]=16; // Faint compressed fringe must not move the authored box.
    fill(canvas,32,{12,4,10,5},255);canvas[3]=16;
    const auto path=directory/"placement.png";Png(path,32,16,canvas);
    std::string digest=Hex(Read(path));
    Artwork row{"hud\\fixture.tga",8,4,15,1,"","placement.png",digest.c_str(),{},AlphaPolicy::Png,Placement::SourceBounds};
    Image output{1,1,{1,2,3,4}};const Image sentinel=output;
    CHECK(Load(row,directory,{},output,original));CHECK(output.width==32&&output.height==16);
    Crop bounds{};CHECK(ContentBounds(32,16,output.bgra,bounds));
    CHECK(bounds.x==4&&bounds.y==4&&bounds.width==16&&bounds.height==8);
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<32;++x) {
        const auto* p=output.bgra.data()+(y*32+x)*4;
        const bool inside=x>=4&&x<20&&y>=4&&y<12;
        CHECK(p[3]==(inside?255:0));
        if(inside)CHECK(p[0]==19&&p[1]==53&&p[2]==201);
        else CHECK(!p[0]&&!p[1]&&!p[2]);
    }
    // The restored silhouette is not masked by holes in the original alpha.
    CHECK(output.bgra[(4*32+8)*4+3]==255);
    // Extra transparent rows must not alter sprite size or position.
    const auto rectangular=output;
    canvas.resize(32*24*4);Png(path,32,24,canvas);
    digest=Hex(Read(path));row.png_sha256=digest.c_str();
    CHECK(Load(row,directory,{},output,original));CHECK(Same(output,rectangular));
    canvas.resize(32*16*4);Png(path,32,16,canvas);
    digest=Hex(Read(path));row.png_sha256=digest.c_str();
    auto fail=[&](const Artwork& bad,std::span<const uint8_t> src) {
        output=sentinel;CHECK(!Load(bad,directory,{},output,src));CHECK(Same(output,sentinel));
    };
    auto bad=row;bad.alpha=AlphaPolicy::Source;fail(bad,original);
    bad=row;bad.name="frontend\\fixture.tga";fail(bad,original);
    bad=row;bad.placement=Placement(99);fail(bad,original);
    fail(row,{});fail(row,std::span(original).first(original.size()-1));
    std::vector<uint8_t> empty(original.size());fail(row,empty);
    fill(empty,8,{1,1,4,2},16);fail(row,empty);
    // Mild generated aspect variation maps to the exact authored rectangle;
    // a major shape change must fall back until the approved art is replaced.
    for(unsigned content_width:{12u,14u}) {
        std::fill(canvas.begin(),canvas.end(),0);fill(canvas,32,{12,4,content_width,5},255);
        Png(path,32,16,canvas);digest=Hex(Read(path));row.png_sha256=digest.c_str();
        if(content_width==12){CHECK(Load(row,directory,{},output,original));
            CHECK(ContentBounds(32,16,output.bgra,bounds));
            CHECK(bounds.x==4&&bounds.y==4&&bounds.width==16&&bounds.height==8);}
        else fail(row,original);
    }
    std::fill(canvas.begin(),canvas.end(),0);Png(path,32,16,canvas);
    digest=Hex(Read(path));row.png_sha256=digest.c_str();fail(row,original);
    CHECK(std::filesystem::remove(path));
}
static void ChromaChecks(const std::filesystem::path& directory) {
    const auto path=directory/"chroma.png";
    std::vector<uint8_t> pixels(128*128*4),authored(pixels.size());
    for(size_t i=0;i<pixels.size();i+=4) {
        const unsigned kind=unsigned(i/4)%8;
        pixels[i]=pixels[i+1]=pixels[i+2]=255;pixels[i+3]=255;
        authored[i]=160;authored[i+1]=80;authored[i+2]=40;authored[i+3]=255;
        if(kind==1)authored[i]=authored[i+1]=authored[i+2]=0;
        if(kind==2){authored[i]=4;authored[i+1]=2;authored[i+2]=1;}
        if(kind==3)authored[i+3]=0;
        if(kind==4)pixels[i]=pixels[i+1]=pixels[i+2]=0;
        if(kind==5){pixels[i]=pixels[i+2]=0;authored[i+3]=85;}
        if(kind==6){pixels[i]=pixels[i+1]=0;authored[i+3]=1;}
        if(kind==7){pixels[i+3]=0;pixels[i]=255;pixels[i+1]=0;pixels[i+2]=255;authored[i+3]=0;}
    }
    Png(path,128,128,pixels);std::string digest=Hex(Read(path));
    Artwork row{"frontend\\frt_content\\level_image_fixture_smg.tga",32,32,15,1,"","chroma.png",digest.c_str(),
                {},AlphaPolicy::Source,Placement::Canvas,ColorPolicy::SourceChroma};
    Image output{};CHECK(Load(row,directory,authored,output));CHECK(output.width==128&&output.height==128);
    const std::array<std::array<uint8_t,4>,8> expected{{{255,128,64,255},{0,0,0,255},{4,2,1,255},
        {0,0,0,0},{0,0,0,255},{182,91,46,85},{54,27,14,1},{0,0,0,0}}};
    for(size_t i=0;i<output.bgra.size();i+=4)for(unsigned c=0;c<4;++c){
        if(output.bgra[i+c]!=expected[(i/4)%8][c])std::fprintf(stderr,"chroma kind=%zu channel=%u got=%u expected=%u\n",(i/4)%8,c,output.bgra[i+c],expected[(i/4)%8][c]);
        CHECK(output.bgra[i+c]==expected[(i/4)%8][c]);}
    const auto good=output;
    for(auto invalid:std::array{ColorPolicy(99),ColorPolicy::Png}) {
        auto bad=row;bad.color=invalid;
        if(invalid==ColorPolicy::Png){CHECK(Load(bad,directory,authored,output));CHECK(output.bgra[0]==255&&output.bgra[1]==255&&output.bgra[2]==255);}
        else {output=good;CHECK(!Load(bad,directory,authored,output));CHECK(Same(good,output));}
    }
    for(const char* name:{"hud\\fixture_smg.tga","frontend\\frt_content\\level_image_01sm.tga",
                         "frontend\\frt_content\\icons\\smg.tga"}) {
        auto bad=row;bad.name=name;output=good;CHECK(!Load(bad,directory,authored,output));CHECK(Same(good,output));
    }
    auto bad=row;bad.alpha=AlphaPolicy::Png;CHECK(!Load(bad,directory,authored,output));
    bad=row;bad.width=64;CHECK(!Load(bad,directory,authored,output));
    CHECK(!Load(row,directory,{},output));CHECK(!Load(row,directory,std::span(authored).first(authored.size()-1),output));
    // Chroma transfer follows premultiplied downsampling: invisible magenta
    // beside a fractional edge cannot contaminate the restored luminance.
    pixels.resize(256*256*4);
    for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x) {
        auto* p=pixels.data()+(y*256+x)*4;
        p[0]=x<129?19:255;p[1]=x<129?53:0;p[2]=x<129?201:255;p[3]=x<129?255:0;
    }
    for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x) {
        auto* p=authored.data()+(y*128+x)*4;p[0]=160;p[1]=80;p[2]=40;p[3]=x<64?255:x==64?85:0;
    }
    Png(path,256,256,pixels);digest=Hex(Read(path));row.png_sha256=digest.c_str();
    CHECK(Load(row,directory,authored,output));
    for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x) {
        const auto* p=output.bgra.data()+(y*128+x)*4;
        if(x<=64)CHECK(p[0]==82&&p[1]==41&&p[2]==21&&p[3]==(x<64?255:85));
        else CHECK(!p[0]&&!p[1]&&!p[2]&&!p[3]);
    }
    CHECK(std::filesystem::remove(path));
}
int main() {
    Identity();
    // The loader must also work on a thread with an existing STA apartment.
    CHECK(SUCCEEDED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED)));
    const auto directory=std::filesystem::temp_directory_path()/
        ("uc-hd-restored-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    CHECK(std::filesystem::create_directory(directory));
    PlacementChecks(directory);
    ChromaChecks(directory);
    const std::array<uint8_t,3> source{'a','b','c'};const std::string source_hash=Hex(source);
    std::vector<uint8_t> pixels(8*8*4);
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
        const size_t i=(y*8+x)*4;pixels[i]=uint8_t(x*23);pixels[i+1]=uint8_t(y*29);pixels[i+2]=177;pixels[i+3]=uint8_t((x+y)%4*85);
    }
    const auto path=directory/"fixture.png";Png(path,8,8,pixels);
    const auto encoded=Read(path);std::string png_hash=Hex(encoded);
    Artwork row{"hud\\fixture.tga",2,2,15,source.size(),source_hash.c_str(),"fixture.png",png_hash.c_str()};
    const Image sentinel{1,1,{1,2,3,4}};Image output=sentinel;
    CHECK(Load(row,directory,{},output));CHECK(output.width==8&&output.height==8);
    for(size_t i=0;i<pixels.size();i+=4) {
        CHECK(output.bgra[i+3]==pixels[i+3]);
        for(unsigned c=0;c<3;++c)CHECK(output.bgra[i+c]==(pixels[i+3]?pixels[i+c]:0));
    }
    const auto good=output;
    auto fail=[&](const Artwork& bad,const std::filesystem::path& dir,std::span<const uint8_t> alpha={}) {
        output=sentinel;CHECK(!Load(bad,dir,alpha,output));CHECK(Same(output,sentinel));
    };
    fail(row,{});fail(row,directory/"missing");
    auto bad=row;bad.png="../fixture.png";fail(bad,directory);
    bad=row;bad.png_sha256=source_hash.c_str();fail(bad,directory);
    bad=row;bad.crop={1,0,8,8};fail(bad,directory);
    bad=row;bad.width=3;fail(bad,directory);
    bad=row;bad.alpha=AlphaPolicy(99);fail(bad,directory);
    bad=row;bad.alpha=AlphaPolicy::Source;fail(bad,directory);fail(bad,directory,std::span(pixels).first(pixels.size()-1));
    std::vector<uint8_t> alpha=pixels;for(size_t i=3;i<alpha.size();i+=4)alpha[i]=uint8_t(255-alpha[i]);
    alpha[0]=31;alpha[1]=81;alpha[2]=125;
    CHECK(Load(bad,directory,alpha,output));
    for(size_t i=0;i<alpha.size();i+=4) {
        CHECK(output.bgra[i+3]==alpha[i+3]);
        for(unsigned c=0;c<3;++c)CHECK(output.bgra[i+c]==(alpha[i+3]?(pixels[i+3]?pixels[i+c]:alpha[i+c]):0));
    }
    // The source policy never uses the PNG's alpha as an additional mask.
    CHECK(output.bgra[3]==255&&output.bgra[0]==31&&output.bgra[1]==81&&output.bgra[2]==125);
    auto corrupt=encoded;corrupt.back()^=1;Write(path,corrupt);fail(row,directory);
    const std::array<uint8_t,5> invalid{'b','o','g','u','s'};Write(path,invalid);
    png_hash=Hex(invalid);row.png_sha256=png_hash.c_str();fail(row,directory);
    // An approved canvas crop is exact, including offsets and alpha.
    std::vector<uint8_t> canvas(16*16*4,255);
    for(unsigned y=0;y<8;++y)std::memcpy(canvas.data()+((y+4)*16+4)*4,pixels.data()+y*8*4,8*4);
    Png(path,16,16,canvas);png_hash=Hex(Read(path));row.png_sha256=png_hash.c_str();row.crop={4,4,8,8};
    CHECK(Load(row,directory,{},output));CHECK(Same(output,good));
    // A higher-resolution source is reduced in premultiplied space.
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
        const size_t i=(y*16+x)*4;canvas[i]=x<8?19:255;canvas[i+1]=x<8?53:0;canvas[i+2]=x<8?201:255;canvas[i+3]=x<8?255:0;
    }
    Png(path,16,16,canvas);png_hash=Hex(Read(path));row.png_sha256=png_hash.c_str();row.crop={};
    CHECK(Load(row,directory,{},output));CHECK(output.width==8&&output.height==8);
    for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x) {
        const size_t i=(y*8+x)*4;
        if(x<4)CHECK(output.bgra[i]==19&&output.bgra[i+1]==53&&output.bgra[i+2]==201&&output.bgra[i+3]==255);
        else CHECK(!output.bgra[i]&&!output.bgra[i+1]&&!output.bgra[i+2]&&!output.bgra[i+3]);
    }
    // Approved generated art need not be an integer multiple of the original.
    canvas.resize(6*6*4);
    for(size_t i=0;i<canvas.size();i+=4){canvas[i]=17;canvas[i+1]=53;canvas[i+2]=201;canvas[i+3]=255;}
    Png(path,6,6,canvas);png_hash=Hex(Read(path));row.png_sha256=png_hash.c_str();
    CHECK(Load(row,directory,{},output));CHECK(output.width==8&&output.height==8);
    for(size_t i=0;i<output.bgra.size();i+=4)
        CHECK(output.bgra[i]==17&&output.bgra[i+1]==53&&output.bgra[i+2]==201&&output.bgra[i+3]==255);
    CHECK(std::filesystem::remove(path));CHECK(std::filesystem::remove(directory));
    CoUninitialize();
    std::printf("PASS %zu restored artwork checks: SHA256 identities, WIC alpha/crop/downsample, invalid-file fallback\n",checks);
}
