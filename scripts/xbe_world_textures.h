#pragma once
#include "xbe_hd_ui_restored.h"
#include <sstream>

// Optional local world pack; exact Xbox mip identity, never a filename guess.
namespace uc_world_textures {
struct Entry { uint32_t width,height,format,bytes; std::string source,target; };
struct Image { uint32_t width=0,height=0; std::vector<std::vector<uint8_t>> mips; };
inline bool Hex(const std::string& s) {return s.size()==64&&s.find_first_not_of("0123456789abcdef")==s.npos;}
inline bool Parse(const std::string& line,Entry& e) {
    std::istringstream in(line);std::string extra;
    return bool(in>>e.width>>e.height>>e.format>>e.bytes>>e.source>>e.target)&&!(in>>extra)&&
        e.width&&e.height&&e.width<=4096&&e.height<=4096&&
        !(e.width&(e.width-1))&&!(e.height&(e.height-1))&&
        (e.format==12||e.format==14||e.format==15)&&e.bytes&&e.bytes<=32*1024*1024&&Hex(e.source)&&Hex(e.target);
}
inline std::filesystem::path Directory() {return uc_hd_ui::restored::Directory().parent_path()/"world-textures";}
inline std::vector<Entry> ReadEntries(const std::filesystem::path& directory) {
    std::vector<Entry> rows;std::ifstream f(directory/"manifest.txt",std::ios::ate);
    if(!f||f.tellg()>1024*1024)return rows;f.seekg(0);std::string line;
    while(std::getline(f,line)) {
        Entry e;if(rows.size()>=4096||line.size()>256||!Parse(line,e))return {};
        std::error_code error;auto size=std::filesystem::file_size(directory/(e.target+".uctex"),error);
        if(error||size<20||size>24*1024*1024)return {};
        rows.push_back(std::move(e));
    }
    return rows;
}
inline const std::vector<Entry>& Entries(){static const auto entries=ReadEntries(Directory());return entries;}
inline bool Available(){return !Entries().empty();}
inline const Entry* Match(uint32_t w,uint32_t h,uint32_t format,const uint8_t* p,size_t bytes) {
    std::array<uint8_t,32> digest{};bool hashed=false;std::string hex;
    for(const auto& e:Entries()) {
        if(e.width!=w||e.height!=h||e.format!=format||e.bytes!=bytes)continue;
        if(!hashed){if(!uc_hd_ui::restored::Digest({p,bytes},digest))return nullptr;
            constexpr char digits[]="0123456789abcdef";for(auto b:digest){hex+=digits[b>>4];hex+=digits[b&15];}hashed=true;}
        if(e.source==hex)return &e;
    }
    return nullptr;
}
inline bool Decode(std::span<const uint8_t> b,Image& out) {
    out={};if(b.size()<20||std::memcmp(b.data(),"UCWT0001",8))return false;
    uint32_t w,h,n;std::memcpy(&w,b.data()+8,4);std::memcpy(&h,b.data()+12,4);std::memcpy(&n,b.data()+16,4);
    if(!w||!h||w>4096||h>4096||(w&(w-1))||(h&(h-1))||!n||n>13)return false;
    uint32_t expected=1;for(uint32_t d=std::max(w,h);d>1;d>>=1)++expected;
    if(n!=expected)return false;
    Image image;image.width=w;image.height=h;size_t at=20;
    for(uint32_t i=0;i<n;++i){size_t size=size_t((w+3)/4)*((h+3)/4)*16;
        if(size>b.size()-at)return false;
        image.mips.emplace_back(b.begin()+at,b.begin()+at+size);at+=size;w=std::max(1u,w/2);h=std::max(1u,h/2);}
    if(at!=b.size())return false;out=std::move(image);return true;
}
inline bool Load(const Entry& e,Image& image) {
    try {
        std::ifstream f(Directory()/(e.target+".uctex"),std::ios::binary|std::ios::ate);
        if(!f)return false;auto size=f.tellg();if(size<20||size>24*1024*1024)return false;
        std::vector<uint8_t> b(size_t(size),uint8_t{});f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),size))return false;
        return uc_hd_ui::restored::MatchesDigest(b,e.target)&&Decode(b,image)&&
            image.width>=e.width&&image.height>=e.height&&uint64_t(image.width)*e.height==uint64_t(image.height)*e.width;
    } catch(const std::bad_alloc&) {return false;}
}
}
