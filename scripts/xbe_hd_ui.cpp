#include "xbe_hd_ui.h"
#include "xbe_hd_ui_filter.h"
#include "xbe_hd_ui_artwork.h"
#include "xbe_hd_ui_reconstruct.h"
#include "kernel/disc.h"
extern "C" {
#include "native/font.h"
#include "native/texture_asset/texture_asset.h"
}
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace uc_hd_ui {
namespace {
constexpr size_t kArchiveLimit=16*1024*1024, kEntryLimit=2*1024*1024;
// Bounded startup storage for the complete UI inventory with restored 4x art.
constexpr size_t kSourceLimit=16*1024*1024, kMemoryLimit=512*1024*1024;
constexpr size_t kMissing=size_t(-1);
constexpr const char* kFonts[]={"small","medium","large","credits","time","debug"};
using Archive=std::unique_ptr<UCArchive,decltype(&uc_archive_close)>;
struct Bytes { UCArchiveBytes value{}; ~Bytes(){uc_archive_bytes_free(&value);} };
struct Texture { UCTextureAsset value{}; ~Texture(){uc_texture_free(&value);} };
struct Asset {
    uint32_t width=0,height=0,format=0;
    uint64_t hash=0;
    std::vector<uint8_t> source;
    Image image;
    bool restored=false;
};
struct Store {
    std::vector<Asset> assets;
    std::array<size_t,6> fonts{kMissing,kMissing,kMissing,kMissing,kMissing,kMissing};
    size_t buttons=kMissing;
    Statistics stats;
};
Store store;
std::once_flag initialized;

uint64_t Hash(const uint8_t* bytes,size_t size) {
    uint64_t hash=1469598103934665603ull;
    for(size_t i=0;i<size;++i){hash^=bytes[i];hash*=1099511628211ull;}
    return hash;
}
float F32(const uint8_t* bytes) {float value;std::memcpy(&value,bytes,4);return value;}
std::string Name(const UCArchiveEntry& entry) {
    std::string name(reinterpret_cast<const char*>(entry.name),entry.name_bytes-1);
    for(char& c:name){if(c>='A'&&c<='Z')c+=char('a'-'A');if(c=='/')c='\\';}
    return name;
}
Archive Open(const std::filesystem::path& game,const char* name) {
    std::vector<uint8_t> bytes;
    if(const auto* disc=kernel::MountedDisc()) {
        const auto* entry=disc->Find(name);
        if(!entry||entry->dir||entry->size<200||entry->size>kArchiveLimit||!disc->Load(name,bytes))return {nullptr,uc_archive_close};
    } else {
        std::ifstream file(game/name,std::ios::binary|std::ios::ate);
        if(!file)return {nullptr,uc_archive_close};
        auto size=file.tellg();if(size<200||size>std::streamoff(kArchiveLimit))return {nullptr,uc_archive_close};
        bytes.resize(size_t(size));file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())))return {nullptr,uc_archive_close};
    }
    UCArchive* result=nullptr;
    if(bytes.size()>kArchiveLimit||uc_archive_copy(bytes.data(),bytes.size(),&result)!=UC_AR_OK)return {nullptr,uc_archive_close};
    return {result,uc_archive_close};
}
bool Read(const UCArchive* archive,const std::string& name,Bytes& bytes) {
    for(size_t i=0;i<uc_archive_count(archive);++i) {
        const auto* entry=uc_archive_entry(archive,i);
        if(Name(*entry)==name)return entry->unpacked_bytes<=kEntryLimit&&
            uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK;
    }
    return false;
}

// Validate every cell before modifying any alpha; malformed/overlapping atlases
// keep the filtered original instead of guessing glyph boundaries.
bool FontContours(const UCArchive* metrics,const char* name,uint32_t width,uint32_t height,
                  const std::vector<uint8_t>& source,std::vector<uint8_t>& output) {
    Bytes bytes;UCFont font{};
    if(!metrics||!Read(metrics,std::string("fonts\\")+name+".abc",bytes)||
       uc_font_read(bytes.value.data,bytes.value.size,&font)||font.height<=0||font.height>128||
       std::floor(font.height)!=font.height)return false;
    using Cell=filter::font_detail::Cell;
    std::vector<Cell> cells;
    std::vector<uint8_t> occupied(size_t(width)*height,0);
    for(uint32_t i=0;i<font.count;++i) {
        const uint8_t* g=font.glyphs+i*24;
        Cell c{F32(g)*width,F32(g+4)*height,F32(g+8)*width,F32(g+12)*height};
        const int glyph_width=int16_t(filter::detail::U16(g+18));
        if(std::abs(c.x1-c.x0-glyph_width)>.01f||std::abs(c.y1-c.y0-font.height)>.01f)return false;
        for(float value:{c.x0,c.y0,c.x1,c.y1})if(std::abs(value-std::round(value))>.001f)return false;
        c={std::round(c.x0),std::round(c.y0),std::round(c.x1),std::round(c.y1)};
        if(c.x0<0||c.y0<0||c.x1>width||c.y1>height||c.x1<c.x0||c.y1<=c.y0)return false;
        if(c.x1==c.x0)continue;
        if(std::any_of(cells.begin(),cells.end(),[&](Cell old){return c.x0==old.x0&&c.y0==old.y0&&c.x1==old.x1&&c.y1==old.y1;}))continue;
        for(unsigned y=unsigned(c.y0);y<unsigned(c.y1);++y)for(unsigned x=unsigned(c.x0);x<unsigned(c.x1);++x)
            if(occupied[size_t(y)*width+x]++)return false;
        cells.push_back(c);
    }
    for(const auto& cell:cells)filter::font_detail::RestoreCell(width,source,cell,output);
    for(size_t i=0;i<output.size();i+=4)if(!output[i+3])output[i]=output[i+1]=output[i+2]=0;
    reconstruct::FontFace(name,width,height,source,cells,output);
    return true;
}
int FontIndex(const std::string& name) {
    for(unsigned i=0;i<std::size(kFonts);++i)
        if(name==std::string("fonts\\")+kFonts[i]+"%4b%.tga")return int(i);
    return -1;
}
bool UiName(const std::string& name,bool hud_only) {
    if(!(name.ends_with(".tga")||name.ends_with(".tga=nomipmap")))return false;
    if(hud_only)return name.starts_with("hud\\");
    return name.starts_with("frontend\\")||name.starts_with("font\\")||name.starts_with("fonts\\");
}
void Add(Store& out,const UCArchive* archive,size_t index,const std::string& name,const UCArchive* metrics,
         const std::filesystem::path& artwork_directory) {
    const auto* entry=uc_archive_entry(archive,index);
    Bytes bytes;Texture texture;
    if(entry->unpacked_bytes>kEntryLimit||uc_archive_read(archive,index,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK||
       uc_texture_decode(bytes.value.data,bytes.value.size,&texture.value)!=UC_TEXTURE_OK) {++out.stats.rejected;return;}
    const auto& t=texture.value;
    const uint32_t format=t.format==UC_TEXTURE_BC1_UNORM?12:t.format==UC_TEXTURE_BC3_UNORM?15:0;
    if(!format||t.width>1024||t.height>1024||t.storage_bytes>kEntryLimit){++out.stats.rejected;return;}
    const int font=FontIndex(name);
    const bool buttons=name=="font\\font_buttons.tga";
    const unsigned scale=(font>=0||buttons||name.starts_with("hud\\")||std::max(t.width,t.height)<256||
        reconstruct::LetteringScale4(name.c_str(),t.width,t.height))?4:2;
    const uint64_t hash=Hash(t.storage,t.storage_bytes);
    const auto* artwork=restored::Find(restored::kArtwork,name,t.width,t.height,format,{t.storage,t.storage_bytes});
    size_t found=kMissing;
    for(size_t i=0;i<out.assets.size();++i) {
        const auto& a=out.assets[i];
        if(a.width==t.width&&a.height==t.height&&a.format==format&&a.hash==hash&&
           a.source.size()==t.storage_bytes&&!std::memcmp(a.source.data(),t.storage,t.storage_bytes)){found=i;break;}
    }
    const bool try_restored=artwork&&(found==kMissing||!out.assets[found].restored);
    if(found==kMissing||out.assets[found].image.width<t.width*scale||try_restored) {
        size_t pixels=size_t(t.width)*t.height*4*scale*scale;
        const size_t previous=found==kMissing?0:out.assets[found].image.bgra.size();
        const size_t extra_source=found==kMissing?t.storage_bytes:0;
        if(out.stats.source_bytes+extra_source>kSourceLimit||
           out.stats.source_bytes+extra_source+out.stats.pixel_bytes-previous+pixels>kMemoryLimit){++out.stats.rejected;return;}
        std::vector<uint8_t> decoded;
        if(!filter::DecodeNative(t.width,t.height,format,{t.mips[0].data,t.mips[0].slice_pitch},decoded)){++out.stats.rejected;return;}
        Image image;bool applied=false;
        const size_t restored_pixels=size_t(t.width)*t.height*4*16;
        if(try_restored&&out.stats.source_bytes+extra_source+out.stats.pixel_bytes-previous+restored_pixels<=kMemoryLimit) {
            const auto alpha=artwork->alpha==restored::AlphaPolicy::Source?
                filter::Upscale(t.width,t.height,decoded,4):std::vector<uint8_t>{};
            applied=restored::Load(*artwork,artwork_directory,alpha,image,decoded);
        }
        // A failed optional replacement cannot downgrade an existing alias.
        if(applied||found==kMissing||out.assets[found].image.width<t.width*scale) {
            if(applied)pixels=image.bgra.size();
            else {
                image={t.width*scale,t.height*scale,filter::Upscale(t.width,t.height,decoded,scale)};
                if(image.bgra.size()!=pixels){++out.stats.rejected;return;}
                reconstruct::Hud(name.c_str(),t.width,t.height,decoded,scale,image.bgra);
                if(font>=0&&FontContours(metrics,kFonts[font],t.width,t.height,decoded,image.bgra))++out.stats.contoured_fonts;
            }
            const bool panel_2x=image.width==t.width*2;
            if(found==kMissing) {
                Asset a{t.width,t.height,format,hash,{t.storage,t.storage+t.storage_bytes},std::move(image),applied};
                out.assets.push_back(std::move(a));found=out.assets.size()-1;
                ++out.stats.images;out.stats.source_bytes+=t.storage_bytes;
                if(panel_2x)++out.stats.panels_2x;
            } else {
                const bool was_panel_2x=out.assets[found].image.width==t.width*2;
                out.assets[found].image=std::move(image);out.assets[found].restored=applied;
                if(was_panel_2x&&!panel_2x)--out.stats.panels_2x;
            }
            out.stats.pixel_bytes+=pixels-previous;
            if(applied)++out.stats.restored_images;
        }
    }
    if(font>=0&&out.fonts[font]==kMissing){out.fonts[font]=found;++out.stats.fonts;}
    if(buttons)out.buttons=found;
}
void Prepare(Store& out,const std::filesystem::path& game) {
    auto metrics=Open(game,"dialoguefonts.lst");
    const auto artwork_directory=restored::Directory();
    // Fixed known frontend, mission and equipment sources; only named UI is
    // decoded. New equipment archives must be audited before joining this list.
    for(const auto& source:detail::kSources) {
        auto archive=Open(game,source.file);if(!archive)continue;
        for(size_t i=0;i<uc_archive_count(archive.get());++i) {
            const std::string name=Name(*uc_archive_entry(archive.get(),i));
            if(!UiName(name,source.hud_only))continue;
            try {Add(out,archive.get(),i,name,metrics.get(),artwork_directory);}
            catch(const std::bad_alloc&){++out.stats.rejected;return;}
        }
    }
}
}

void Initialize(const std::filesystem::path& game) {
    std::call_once(initialized,[&]{
        // Failure leaves the original art available; nothing is written to disc.
        try {Prepare(store,game);} catch(const std::exception&){++store.stats.rejected;}
    });
}
const Image* Match(uint32_t width,uint32_t height,uint32_t format,const uint8_t* payload,size_t bytes) {
    if(!payload||!bytes||bytes>kEntryLimit||!width||!height||(format!=12&&format!=15))return nullptr;
    if(std::none_of(store.assets.begin(),store.assets.end(),[&](const Asset& a){
        return a.width==width&&a.height==height&&a.format==format&&a.source.size()==bytes;
    }))return nullptr;
    const uint64_t hash=Hash(payload,bytes);
    for(const auto& a:store.assets)
        if(a.width==width&&a.height==height&&a.format==format&&a.source.size()==bytes&&a.hash==hash&&
           !std::memcmp(a.source.data(),payload,bytes))return &a.image;
    return nullptr;
}
const Image* Font(const char* name) {
    if(!name)return nullptr;
    for(unsigned i=0;i<std::size(kFonts);++i)
        if(!std::strcmp(name,kFonts[i])&&store.fonts[i]!=kMissing)return &store.assets[store.fonts[i]].image;
    return nullptr;
}
const Image* Buttons(){return store.buttons==kMissing?nullptr:&store.assets[store.buttons].image;}
bool Ready(){return !store.assets.empty();}
Statistics Stats(){return store.stats;}
}
