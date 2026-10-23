#include "xbe_hd_ui.h"
#include "xbe_hd_ui_filter.h"
#include "xbe_hd_ui_artwork.h"
#include "xbe_pc_prompts.h"
#include "kernel/disc.h"
extern "C" {
#include "native/texture_asset/texture_asset.h"
}
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <string>

#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static size_t checks=0;
namespace kernel {
const Disc* MountedDisc(){return nullptr;}
const Disc::Entry* Disc::Find(std::string_view)const{return nullptr;}
bool Disc::Load(std::string_view,std::vector<uint8_t>&)const{return false;}
}
static void Filters() {
    using namespace uc_hd_ui::filter;
    CHECK(Upscale(0,1,{},4).empty());CHECK(Upscale(2,2,{},4).empty());
    CHECK(Upscale(1,1,{1,2,3,255},3).empty());
    for(unsigned scale:{2u,4u}) {
        auto solid=Upscale(1,1,{19,53,201,127},scale);
        CHECK(solid.size()==scale*scale*4);
        for(size_t i=0;i<solid.size();i+=4)CHECK(solid[i]==19&&solid[i+1]==53&&solid[i+2]==201&&solid[i+3]==127);
        auto transparent=Upscale(1,1,{255,123,45,0},scale);
        CHECK(std::all_of(transparent.begin(),transparent.end(),[](uint8_t v){return !v;}));
        std::vector<uint8_t> edge(8*4);
        for(unsigned i=0;i<8;++i){edge[i*4]=255;edge[i*4+2]=i<2?200:0;edge[i*4+3]=i<2?255:0;}
        auto filtered=Upscale(8,1,edge,scale);
        for(unsigned y=0;y<scale;++y)for(unsigned x=0;x<8*scale;++x) {
            auto* p=filtered.data()+(y*8*scale+x)*4;
            if(x>=3*scale)CHECK(p[3]==0);
            if(p[3])CHECK(p[0]==255&&p[2]==200);else CHECK(!p[0]&&!p[1]&&!p[2]);
        }
    }
    // A closed ring retains its hole and integrated stroke coverage.
    std::vector<uint8_t> ring(9*9*4,0);
    for(unsigned y=2;y<7;++y)for(unsigned x=2;x<7;++x)if(x==2||x==6||y==2||y==6) {
        const size_t p=(y*9+x)*4;ring[p]=ring[p+1]=ring[p+2]=ring[p+3]=255;
    }
    auto hd=Upscale(9,9,ring,4);font_detail::RestoreCell(9,ring,{0,0,9,9},hd);
    uint64_t area=0;for(size_t i=3;i<hd.size();i+=4)area+=hd[i];
    CHECK(std::abs(double(area)-16*255*16)<16*255*.02);
    CHECK(hd[(18*36+18)*4+3]==0);CHECK(hd[(10*36+10)*4+3]>200);
    // Contour coverage below one original alpha step must retain white colour,
    // including pixels whose first filtered alpha rounded down to zero.
    for(unsigned alpha:{1u,2u,7u,127u,255u}) {
        std::vector<uint8_t> faint(5*5*4,0);
        faint[(2*5+2)*4]=faint[(2*5+2)*4+1]=faint[(2*5+2)*4+2]=255;
        faint[(2*5+2)*4+3]=uint8_t(alpha);
        auto restored=Upscale(5,5,faint,4);
        font_detail::RestoreCell(5,faint,{0,0,5,5},restored);
        for(size_t p=0;p<restored.size();p+=4)
            if(restored[p+3])CHECK(restored[p]==255&&restored[p+1]==255&&restored[p+2]==255);
    }
    std::vector<uint8_t> decoded{1,2,3,4};
    CHECK(!DecodeNative(4,4,0,{},decoded));CHECK(decoded==std::vector<uint8_t>({1,2,3,4}));
    uint8_t bc1[8]={0,0,255,255,255,255,255,255};
    CHECK(DecodeNative(4,4,12,bc1,decoded));
    for(size_t i=3;i<decoded.size();i+=4)CHECK(decoded[i]==0);
    // Existing Urban BC3 decoder is an independent output reference.
    uint64_t random=0x5362a7c9;
    for(unsigned test=0;test<256;++test) {
        uint8_t bc3[16];for(auto& b:bc3){random^=random<<13;random^=random>>7;random^=random<<17;b=uint8_t(random);}
        CHECK(DecodeNative(4,4,15,bc3,decoded));uint32_t expected[16];
        uc_pc_prompts::DecodeBc3(bc3,4,4,expected);CHECK(!std::memcmp(expected,decoded.data(),64));
    }
}
static void Preview(const std::filesystem::path& path,const UCTextureAsset& t,const uc_hd_ui::Image& image,bool full=false) {
    std::vector<uint8_t> original;
    CHECK(uc_hd_ui::filter::DecodeNative(t.width,t.height,t.format==UC_TEXTURE_BC1_UNORM?12:15,
          {t.mips[0].data,t.mips[0].slice_pitch},original));
    const unsigned scale=image.width/t.width;
    const unsigned side=full?std::min(image.width,768u):image.width;
    const unsigned w=side*2,h=full?image.height*side/image.width:std::min(image.height,512u);
    std::vector<uint8_t> bmp(54+size_t(w)*h*4,0);
    auto u16=[&](size_t p,uint16_t n){std::memcpy(bmp.data()+p,&n,2);};
    auto u32=[&](size_t p,uint32_t n){std::memcpy(bmp.data()+p,&n,4);};
    bmp[0]='B';bmp[1]='M';u32(2,uint32_t(bmp.size()));u32(10,54);u32(14,40);
    u32(18,w);u32(22,uint32_t(-int(h)));u16(26,1);u16(28,32);u32(34,w*h*4);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
        const unsigned sx=(x%side)*image.width/side,sy=y*image.width/side;
        const auto* p=x<side?original.data()+(size_t(sy/scale)*t.width+sx/scale)*4:
            image.bgra.data()+(size_t(sy)*image.width+sx)*4;
        auto* out=bmp.data()+54+(size_t(y)*w+x)*4;
        const unsigned bg=((x/16+y/16)&1)?40:65;
        for(unsigned c=0;c<3;++c)out[c]=uint8_t((unsigned(p[c])*p[3]+bg*(255-p[3]))/255);
        out[3]=255;
    }
    std::ofstream file(path,std::ios::binary);CHECK(file.good());file.write(reinterpret_cast<char*>(bmp.data()),bmp.size());CHECK(file.good());
}
int main(int argc,char** argv) {
    Filters();CHECK(argc>=2);
    bool require_restored=false;std::filesystem::path preview;
    for(int i=2;i<argc;++i)if(std::string_view(argv[i])=="--require-restored")require_restored=true;
        else {CHECK(preview.empty());preview=argv[i];}
    const bool missing=std::string(argv[1])=="--missing";
    const std::filesystem::path game=missing?"__absent_hd_ui_fixture__":argv[1];
    const auto begin=std::chrono::steady_clock::now();uc_hd_ui::Initialize(game);
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    if(missing){CHECK(!uc_hd_ui::Ready()&&!uc_hd_ui::Buttons()&&!uc_hd_ui::Font("medium"));
        CHECK(uc_hd_ui::Stats().images==0);std::printf("PASS missing archives fallback; %zu checks\n",checks);return 0;}
    CHECK(uc_hd_ui::Ready());const auto stats=uc_hd_ui::Stats();
    std::printf("prepared: images=%zu fonts=%zu contours=%zu restored=%zu pixels=%zu source=%zu rejected=%zu seconds=%.3f\n",
        stats.images,stats.fonts,stats.contoured_fonts,stats.restored_images,stats.pixel_bytes,stats.source_bytes,stats.rejected,seconds);
    CHECK(stats.images>200&&stats.images<400);CHECK(stats.fonts==6&&stats.contoured_fonts==5);
    CHECK(stats.source_bytes<=16*1024*1024&&stats.source_bytes+stats.pixel_bytes<=512*1024*1024);
    CHECK(stats.rejected==0);CHECK(!uc_hd_ui::Font("bogus")&&!uc_hd_ui::Font(nullptr));
    CHECK(uc_hd_ui::Buttons()&&uc_hd_ui::Buttons()->width==128&&uc_hd_ui::Buttons()->height==4096);
    std::set<const uc_hd_ui::Image*> unique;
    std::set<const uc_hd_ui::restored::Artwork*> approved;
    std::set<const uc_hd_ui::Image*> restored;
    std::set<const uc_hd_ui::restored::Artwork*> invalid_artwork;
    std::set<const uc_hd_ui::Image*> equipment_images;
    unsigned total=0,full_chain=0,previews=0,equipment_entries=0;
    for(const auto& source:uc_hd_ui::detail::kSources) {
        const char* archive_name=source.file;
        const bool equipment=source.hud_only&&std::string_view(archive_name).ends_with(".lst");
        std::ifstream file(game/archive_name,std::ios::binary|std::ios::ate);CHECK(file.good());
        std::vector<uint8_t> data(size_t(file.tellg()));file.seekg(0);CHECK(file.read(reinterpret_cast<char*>(data.data()),data.size()));
        UCArchive* archive=nullptr;CHECK(uc_archive_copy(data.data(),data.size(),&archive)==UC_AR_OK);
        for(size_t i=0;i<uc_archive_count(archive);++i) {
            const auto* entry=uc_archive_entry(archive,i);std::string name(reinterpret_cast<const char*>(entry->name));
            std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(c>='A'&&c<='Z'?c+'a'-'A':c);});
            const bool is_hud=name.starts_with("hud\\");
            if(name=="font\\font_untextured.tga")continue;
            if(!(name.ends_with(".tga")||name.ends_with(".tga=nomipmap"))||
               (source.hud_only?!is_hud:
                !(name.starts_with("font\\")||name.starts_with("fonts\\")||name.starts_with("frontend\\"))))continue;
            UCArchiveBytes bytes{};UCTextureAsset t{};
            CHECK(uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes)==UC_AR_OK);
            CHECK(uc_texture_decode(bytes.data,bytes.size,&t)==UC_TEXTURE_OK);
            const uint32_t format=t.format==UC_TEXTURE_BC1_UNORM?12:15;
            const auto* image=uc_hd_ui::Match(t.width,t.height,format,t.storage,t.storage_bytes);CHECK(image);
            if(equipment) {
                CHECK(is_hud&&t.width==64&&t.height==32&&format==15&&t.storage_bytes==2048);
                CHECK(image->width==256&&image->height==128);
                equipment_images.insert(image);++equipment_entries;
            }
            unique.insert(image);++total;if(t.mip_count>1)++full_chain;
            const bool font=name.starts_with("fonts\\"),buttons=name=="font\\font_buttons.tga";
            const unsigned scale=font||buttons||is_hud||std::max(t.width,t.height)<256?4:2;
            CHECK(image->width>=t.width*scale&&image->height>=t.height*scale);
            CHECK(image->bgra.size()==size_t(image->width)*image->height*4);
            if(require_restored)for(const auto& row:uc_hd_ui::restored::kArtwork) {
                if(!uc_hd_ui::restored::Find({&row,1},name,t.width,t.height,format,{t.storage,t.storage_bytes}))continue;
                if(approved.contains(&row)||invalid_artwork.contains(&row))continue;
                std::vector<uint8_t> decoded,alpha;uc_hd_ui::Image expected;
                CHECK(uc_hd_ui::filter::DecodeNative(t.width,t.height,format,{t.mips[0].data,t.mips[0].slice_pitch},decoded));
                if(row.alpha==uc_hd_ui::restored::AlphaPolicy::Source)
                    alpha=uc_hd_ui::filter::Upscale(t.width,t.height,decoded,4);
                if(!uc_hd_ui::restored::Load(row,uc_hd_ui::restored::Directory(),alpha,expected,decoded)) {
                    std::fprintf(stderr,"Approved artwork did not load: %s (%s)\n",row.name,row.png);
                    invalid_artwork.insert(&row);continue;
                }
                if(image->width!=expected.width||image->height!=expected.height||image->bgra!=expected.bgra) {
                    std::fprintf(stderr,"Approved artwork was not selected by service: %s (%s)\n",row.name,row.png);
                    invalid_artwork.insert(&row);continue;
                }
                approved.insert(&row);restored.insert(image);
            }
            // Retail time.abc describes 37px-tall cells in a 32px atlas. Its
            // out-of-range UVs must keep the complete filtered original.
            if(name=="fonts\\time%4b%.tga") {
                std::vector<uint8_t> decoded;
                CHECK(uc_hd_ui::filter::DecodeNative(t.width,t.height,format,{t.mips[0].data,t.mips[0].slice_pitch},decoded));
                CHECK(image->bgra==uc_hd_ui::filter::Upscale(t.width,t.height,decoded,4));
                CHECK(uc_hd_ui::Font("time")==image);
            }
            CHECK(!uc_hd_ui::Match(t.width+1,t.height,format,t.storage,t.storage_bytes));
            CHECK(!uc_hd_ui::Match(t.width,t.height,0,t.storage,t.storage_bytes));
            CHECK(!uc_hd_ui::Match(t.width,t.height,format,t.storage,t.storage_bytes-1));
            t.storage[t.storage_bytes-1]^=1;CHECK(!uc_hd_ui::Match(t.width,t.height,format,t.storage,t.storage_bytes));t.storage[t.storage_bytes-1]^=1;
            CHECK(uc_hd_ui::Match(t.width,t.height,format,t.storage,t.storage_bytes)==image);
            const bool complete_art=(is_hud&&(name.find("ammo_")!=std::string::npos||name.find("badge_")!=std::string::npos||name.find("weapons_")!=std::string::npos))||
                name.find("lc_medal")!=std::string::npos||name.find("reward_star")!=std::string::npos||
                name.find("level_image_01sm")!=std::string::npos||name.find("level_image_essm")!=std::string::npos||
                name.find("reward_image_assaultclip_fadesm")!=std::string::npos||
                name.find("frt_backdrop_memory")!=std::string::npos||name.find("frt_backdrop_rcgeneric")!=std::string::npos;
            if(!preview.empty()&&(name=="fonts\\medium%4b%.tga"||name=="fonts\\large%4b%.tga"||buttons||complete_art||
                        (is_hud&&name.find("health")!=std::string::npos))) {
                std::filesystem::create_directories(preview);
                Preview(preview/("pair-"+std::to_string(previews++)+".bmp"),t,*image,complete_art);
                std::printf("preview %u: %s (left nearest original; right HD)\n",previews-1,name.c_str());
            }
            uc_texture_free(&t);uc_archive_bytes_free(&bytes);
        }
        uc_archive_close(archive);
    }
    CHECK(unique.size()==stats.images&&unique.size()==298);CHECK(total>stats.images&&full_chain>100);
    // Real mission material payloads must never resolve to HD HUD replacements.
    const struct {const char* archive;const char* first;const char* second;} world_fixtures[]={
        {"1-01.ent","untextured.tga","particles\\fireball.tga"},
        {"9-01.ent","particles\\soft_circle.tga",nullptr},
        {"2-01.ent","particles\\soft_circle.tga",nullptr},
    };
    for(const auto& fixture:world_fixtures) {
        std::ifstream file(game/fixture.archive,std::ios::binary|std::ios::ate);CHECK(file.good());
        std::vector<uint8_t> data(size_t(file.tellg()));file.seekg(0);CHECK(file.read(reinterpret_cast<char*>(data.data()),data.size()));
        UCArchive* archive=nullptr;CHECK(uc_archive_copy(data.data(),data.size(),&archive)==UC_AR_OK);
        unsigned checked=0;
        for(size_t i=0;i<uc_archive_count(archive);++i){
            const auto* entry=uc_archive_entry(archive,i);
            const std::string name(reinterpret_cast<const char*>(entry->name));
            if(name!=fixture.first&&(!fixture.second||name!=fixture.second))continue;
            UCTextureAsset t{};CHECK(uc_texture_load(archive,i,UC_AR_LOCAL_PAYLOAD,&t)==UC_TEXTURE_OK);
            const unsigned format=t.format==UC_TEXTURE_BC1_UNORM?12:15;
            CHECK(!uc_hd_ui::Match(t.width,t.height,format,t.storage,t.storage_bytes));
            uc_texture_free(&t);++checked;
        }
        uc_archive_close(archive);CHECK(checked==(fixture.second?2u:1u));
    }
    CHECK(equipment_entries==13&&equipment_images.size()==10);
    std::printf("PASS equipment HUD: %u entries, %zu distinct images; only named HUD textures loaded\n",
                equipment_entries,equipment_images.size());
    if(require_restored) {
        CHECK(!uc_hd_ui::restored::kArtwork.empty());
        for(const auto& row:uc_hd_ui::restored::kArtwork)
            if(!approved.contains(&row)&&!invalid_artwork.contains(&row))
                std::fprintf(stderr,"Approved artwork source identity was not found: %s (%s)\n",row.name,row.png);
        CHECK(invalid_artwork.empty());
        CHECK(approved.size()==uc_hd_ui::restored::kArtwork.size());
        CHECK(restored.size()==stats.restored_images);
        std::printf("PASS all %zu approved artwork records applied (%zu distinct source textures)\n",approved.size(),restored.size());
    }
    const auto* stable=uc_hd_ui::Font("medium");uc_hd_ui::Initialize("__different_path_after_initialization__");CHECK(uc_hd_ui::Font("medium")==stable);
    std::printf("HD_UI_RESULT {\"images\":%zu,\"source_bytes\":%zu,\"pixel_bytes\":%zu,\"fonts\":%zu,\"contoured_fonts\":%zu,\"restored_images\":%zu,\"panels_2x\":%zu,\"rejected\":%zu,\"archive_entries_checked\":%u,\"full_chain_entries\":%u,\"prepare_seconds\":%.6f,\"checks\":%zu}\n",
        stats.images,stats.source_bytes,stats.pixel_bytes,stats.fonts,stats.contoured_fonts,stats.restored_images,stats.panels_2x,stats.rejected,total,full_chain,seconds,checks);
}
