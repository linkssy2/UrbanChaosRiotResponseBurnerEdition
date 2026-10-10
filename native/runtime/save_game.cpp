#include "save_game.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using Bytes=std::vector<uint8_t>;
constexpr uint32_t version=1,maximum_size=32768;
constexpr char magic[]="UCNATSV1";
void put(Bytes&b,uint32_t v){for(unsigned n=0;n<4;n++)b.push_back(uint8_t(v>>(n*8)));}
void put_float(Bytes&b,float v){uint32_t bits;std::memcpy(&bits,&v,4);put(b,bits);}
uint32_t crc(const uint8_t*p,size_t size){uint32_t c=~0u;for(size_t n=0;n<size;n++){c^=p[n];for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
struct Reader {
    const Bytes &bytes;size_t at=0;
    uint32_t word(){if(bytes.size()-at<4)throw std::runtime_error("Native save is truncated");uint32_t v=0;for(unsigned n=0;n<4;n++)v|=uint32_t(bytes[at++])<<(8*n);return v;}
    float scalar(){uint32_t v=word();float f;std::memcpy(&f,&v,4);return f;}
};
void validate(const NativeSaveRecord&r){
    const auto&s=r.checkpoint;
    if(s.number<0||s.number>10000||s.count>27||s.selected< -1||s.selected>=27||
       !std::isfinite(s.health)||s.health<0||s.health>65536||s.shield>1||s.flag9>1||
       r.difficulty.mode>3||!std::isfinite(r.difficulty.adaptive)||std::fabs(r.difficulty.adaptive)>1e6f||
       !std::isfinite(r.difficulty.scale)||std::fabs(r.difficulty.scale)>1e6f||r.completed_objectives.size()>4096)
        throw std::runtime_error("Native save has invalid checkpoint data");
    for(unsigned c=0;c<3;c++)if(s.category[c]< -1||s.category[c]>=27)throw std::runtime_error("Native save has invalid equipment category");
    for(unsigned n=0;n<s.count;n++)if(s.items[n].item>=27||s.items[n].seen>1)throw std::runtime_error("Native save has invalid equipment");
    int32_t last=-1;for(auto source:r.completed_objectives){if(source<=last||source>1000000)throw std::runtime_error("Native save has invalid objective progress");last=source;}
    if(std::all_of(r.level_hash.begin(),r.level_hash.end(),[](uint8_t v){return v==0;}))throw std::runtime_error("Native save has no level revision");
}
Bytes encode(const NativeSaveRecord&r){
    validate(r);const auto&s=r.checkpoint;Bytes b(magic,magic+8);put(b,version);put(b,0);
    b.insert(b.end(),r.level_hash.begin(),r.level_hash.end());
    put(b,uint32_t(s.number));put_float(b,s.health);put(b,s.shield);put(b,s.flag9);put(b,s.count);
    for(unsigned n=0;n<s.count;n++){const auto&i=s.items[n];put(b,i.item);put(b,i.clip);put(b,i.reserve);put(b,i.seen);}
    for(auto v:s.category)put(b,uint32_t(v));put(b,uint32_t(s.selected));put(b,s.extra22);
    for(auto v:s.mission)put(b,v);for(auto v:s.statistics)put(b,v);
    put(b,r.difficulty.mode);put_float(b,r.difficulty.adaptive);put_float(b,r.difficulty.scale);
    put(b,uint32_t(r.completed_objectives.size()));for(auto v:r.completed_objectives)put(b,uint32_t(v));
    const uint32_t length=uint32_t(b.size()+4);for(unsigned n=0;n<4;n++)b[12+n]=uint8_t(length>>(n*8));
    put(b,crc(b.data(),b.size()));return b;
}
NativeSaveRecord decode(const Bytes&b){
    if(b.size()<52||std::memcmp(b.data(),magic,8))throw std::runtime_error("Native save has an unrecognized format");
    Reader in{b,8};if(in.word()!=version)throw std::runtime_error("Native save version is not supported");
    if(in.word()!=b.size())throw std::runtime_error("Native save length is invalid");
    Reader trailer{b,b.size()-4};if(trailer.word()!=crc(b.data(),b.size()-4))throw std::runtime_error("Native save checksum failed; the previous save remains in the .bak file");
    NativeSaveRecord r;std::copy_n(b.begin()+16,32,r.level_hash.begin());in.at=48;auto&s=r.checkpoint;
    s.number=int32_t(in.word());s.health=in.scalar();uint32_t shield=in.word(),flashlight=in.word();s.count=in.word();
    if(shield>1||flashlight>1||s.count>27)throw std::runtime_error("Native save inventory is invalid");
    s.shield=uint8_t(shield);s.flag9=uint8_t(flashlight);
    for(unsigned n=0;n<s.count;n++){auto&i=s.items[n];i.item=in.word();uint32_t clip=in.word(),reserve=in.word(),seen=in.word();
        if(clip>65535||reserve>65535||seen>1)throw std::runtime_error("Native save ammunition is invalid");
        i.clip=uint16_t(clip);i.reserve=uint16_t(reserve);i.seen=uint8_t(seen);}
    for(auto&v:s.category)v=int32_t(in.word());s.selected=int32_t(in.word());s.extra22=in.word();
    for(auto&v:s.mission)v=in.word();for(auto&v:s.statistics)v=in.word();
    r.difficulty.mode=in.word();r.difficulty.adaptive=in.scalar();r.difficulty.scale=in.scalar();
    const uint32_t count=in.word();if(count>4096)throw std::runtime_error("Native save objective count is invalid");
    for(uint32_t n=0;n<count;n++)r.completed_objectives.push_back(int32_t(in.word()));
    if(in.at!=b.size()-4)throw std::runtime_error("Native save has unexpected trailing data");validate(r);return r;
}
}
std::filesystem::path NativeSaveStore::default_directory(){
    wchar_t buffer[32768];DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",buffer,32768);
    if(!n||n>=32768)throw std::runtime_error("Windows local application data directory is unavailable");
    return std::filesystem::path(buffer)/L"UrbanChaosNative"/L"saves";
}
std::filesystem::path NativeSaveStore::path(unsigned slot)const{
    if(slot<1||slot>3)throw std::invalid_argument("Native save slot must be 1, 2 or 3");
    return directory_/(L"slot-"+std::to_wstring(slot)+L".sav");
}
std::optional<NativeSaveRecord> NativeSaveStore::load(unsigned slot)const{
    auto file=path(slot);if(!std::filesystem::exists(file))return std::nullopt;
    const auto size=std::filesystem::file_size(file);if(size>maximum_size)throw std::runtime_error("Native save is too large");
    Bytes bytes(size_t(size),0);std::ifstream in(file,std::ios::binary);
    if(!in.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))||in.peek()!=std::char_traits<char>::eof())
        throw std::runtime_error("Native save could not be read completely");return decode(bytes);
}
void NativeSaveStore::save(unsigned slot,const NativeSaveRecord&r)const{
    const auto bytes=encode(r);
    const auto destination=path(slot);std::filesystem::create_directories(directory_);
    const auto temporary=std::filesystem::path(destination.wstring()+L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Native save temporary file could not be created");
    DWORD written=0;bool complete=WriteFile(file,bytes.data(),DWORD(bytes.size()),&written,nullptr)&&written==bytes.size()&&FlushFileBuffers(file);
    complete=CloseHandle(file)&&complete;
    if(!complete){DeleteFileW(temporary.c_str());throw std::runtime_error("Native save write failed; previous save was kept");}
    const auto backup=std::filesystem::path(destination.wstring()+L".bak");
    bool installed=std::filesystem::exists(destination)?
        ReplaceFileW(destination.c_str(),temporary.c_str(),backup.c_str(),0,nullptr,nullptr)!=0:
        MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_WRITE_THROUGH)!=0;
    if(!installed){DeleteFileW(temporary.c_str());throw std::runtime_error("Native save replacement failed; previous save was kept");}
}
