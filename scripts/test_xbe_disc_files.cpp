// Actual generated files.cpp + real Disc I/O; only guest VM/handle services are fixtures.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include "kernel/kernel.h"
#include "kernel/exports.h"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
alignas(64) static uint8_t memory[0x100000];
uint8_t* kernel::g_base=memory;
static kernel::KernelFn exports[400];
static std::map<uint32_t,kernel::ObjectRef> handles;
static uint32_t next_handle=0x1000,language=0;
void logx::Write(Level,const char*,...){}
namespace trial_language {uint32_t Active(){return language;}}
namespace kernel {
void RegisterExport(uint32_t n,const char*,KernelFn fn){CHECK(n<400);exports[n]=fn;}
Thread* Current(){return nullptr;}
uint32_t NewHandle(ObjectRef obj){auto h=next_handle;next_handle+=4;handles[h]=obj;return h;}
ObjectRef FromHandle(uint32_t h){auto it=handles.find(h);return it==handles.end()?nullptr:it->second;}
int32_t SetEvent(Object& obj){int old=obj.signal;obj.signal=1;return old;}
int32_t ResetEvent(Object& obj){int old=obj.signal;obj.signal=0;return old;}
void QueueApc(Thread&,uint32_t,uint32_t,uint32_t,uint32_t){}
}
#include "files.cpp"

static uint32_t Call(unsigned ordinal,std::initializer_list<uint32_t> args){
    X86Context ctx{};ctx.esp=0xF0000;kernel::W32(ctx.esp,0x1234);unsigned i=0;
    for(auto value:args)kernel::W32(ctx.esp+4+4*i++,value);
    CHECK(exports[ordinal]);exports[ordinal](ctx,memory);CHECK(ctx.esp==0xF0004+4*i);return ctx.eax;
}
static void Name(const std::string& name,uint32_t root=0){
    using namespace kernel;CHECK(name.size()<400);std::memcpy(memory+0x500,name.data(),name.size());
    W16(0x400,uint16_t(name.size()));W16(0x402,uint16_t(name.size()));W32(0x404,0x500);
    W32(0x300,root);W32(0x304,0x400);W32(0x308,0);
}
static uint32_t Open(const std::string& name,uint32_t options=0x20,bool nt_open=false,uint32_t root=0){
    Name(name,root);
    auto result=nt_open?Call(202,{0x200,0x80100080,0x300,0x220,1,options}):
        Call(190,{0x200,0x80100080,0x300,0x220,0,0x80,1,1,options});
    CHECK(result==kernel::kSuccess);return kernel::R32(0x200);
}
static void Read(const std::string& path,const std::string& expected,bool nt_open=false,uint32_t root=0){
    using namespace kernel;const auto h=Open(path,0x20,nt_open,root);W64(0x280,0);
    CHECK(Call(219,{h,0,0,0,0x220,0x1000,1024,0x280})==kSuccess);
    CHECK(R32(0x224)==expected.size()&&std::string(reinterpret_cast<char*>(memory+0x1000),R32(0x224))==expected);
    CHECK(Call(211,{h,0x220,0x800,0x18,5})==kSuccess&&R64(0x808)==expected.size());
    handles.erase(h);
}
static void Write(const fs::path& path,const std::string& text){
    fs::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary|std::ios::trunc);
    CHECK(out.good());out.write(text.data(),text.size());CHECK(out.good());
}
static void Iso(const fs::path& path){
    const std::vector<std::pair<std::string,std::string>> files={
        {"1-01.ent",std::string(200,'E')},{"original.bin","Disc original"},{"coopcharacters.lst","Disc decoy"}};
    std::vector<uint8_t> volume(2048),table;
    const char* magic="MICROSOFT*XBOX*MEDIA";
    std::memcpy(volume.data(),magic,20);std::memcpy(volume.data()+0x7ec,magic,20);
    for(size_t i=0;i<files.size();++i){
        const auto& [name,data]=files[i];const auto at=table.size(),size=(14+name.size()+3)&~size_t(3);
        table.resize(at+size);const uint16_t next=i+1<files.size()?uint16_t((at+size)/4):0;
        const uint32_t sector=uint32_t(40+i),bytes=uint32_t(data.size());
        std::memcpy(table.data()+at+2,&next,2);std::memcpy(table.data()+at+4,&sector,4);
        std::memcpy(table.data()+at+8,&bytes,4);table[at+13]=uint8_t(name.size());
        std::memcpy(table.data()+at+14,name.data(),name.size());
    }
    const uint32_t sector=33,bytes=uint32_t(table.size());
    std::memcpy(volume.data()+0x14,&sector,4);std::memcpy(volume.data()+0x18,&bytes,4);
    std::ofstream out(path,std::ios::binary|std::ios::trunc);CHECK(out.good());
    out.seekp(32*2048);out.write(reinterpret_cast<char*>(volume.data()),volume.size());
    out.write(reinterpret_cast<char*>(table.data()),table.size());
    for(size_t i=0;i<files.size();++i){out.seekp((40+i)*2048);out.write(files[i].second.data(),files[i].second.size());}
    CHECK(out.good());
}
static void Listing(const std::string& name,size_t size){
    using namespace kernel;const auto h=Open("D:\\",0x21);Name(name);
    CHECK(Call(207,{h,0,0,0,0x220,0x800,512,1,0x400,1})==kSuccess);
    CHECK(R32(0x83c)==name.size()&&R64(0x828)==size);
    CHECK(std::string(reinterpret_cast<char*>(memory+0x840),name.size())==name);
    CHECK(Call(207,{h,0,0,0,0x220,0x800,512,1,0x400,0})==kNoMoreFiles);
    handles.erase(h);Name("D:\\"+name);
    CHECK(Call(210,{0x300,0x800})==kSuccess&&R64(0x828)==size);
}
static uint32_t Rename(uint32_t h,const std::string& path){
    using namespace kernel;W32(0x700,0);W32(0x704,0);W16(0x708,uint16_t(path.size()));
    W16(0x70a,uint16_t(path.size()));W32(0x70c,0x780);std::memcpy(memory+0x780,path.data(),path.size());
    return Call(226,{h,0x220,0x700,0x80,10});
}
static void ReadOnly(const std::string& path){
    using namespace kernel;Name(path);
    for(uint32_t access:{0x40000000u,0x10000000u,2u,4u,0x10u,0x100u,0x10000u,0x40000u,0x80000u}){
        CHECK(Call(190,{0x200,access,0x300,0x220,0,0x80,1,1,0x20})==kAccessDenied);
        CHECK(Call(202,{0x200,access,0x300,0x220,1,0x20})==kAccessDenied);
    }
    for(uint32_t disposition:{0u,2u,4u,5u})
        CHECK(Call(190,{0x200,0x80100080,0x300,0x220,0,0x80,1,disposition,0x20})==kAccessDenied);
    CHECK(Call(190,{0x200,0x80100080,0x300,0x220,0,0x80,1,1,0x1020})==kAccessDenied);
    CHECK(Call(202,{0x200,0x80100080,0x300,0x220,1,0x1020})==kAccessDenied);
    CHECK(Call(195,{0x300})==kAccessDenied);
    const auto h=Open(path);CHECK(FileFromHandle(h)->read_only);W64(0x280,0);
    CHECK(Call(236,{h,0,0,0,0x220,0x1000,1,0x280})==kAccessDenied);
    for(uint32_t cls:{13u,19u,20u})CHECK(Call(226,{h,0x220,0x280,8,cls})==kAccessDenied);
    CHECK(Rename(h,"E:\\overwrite.bin")==kAccessDenied);handles.erase(h);
}
int main(){
    using namespace kernel;
    const auto initial=fs::current_path();
    const auto root=fs::temp_directory_path()/(L"ucrr-disc-files-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    CHECK(!fs::exists(root));fs::create_directories(root);fs::current_path(root);
    const auto assets=root/"assets";fs::create_directories(assets);
    Write(assets/"original.bin","Stale local original");Write(assets/"1-01.ent",std::string(200,'L'));
    for(const auto* name:kPreparedArchives)Write(assets/name,std::string("Prepared ")+name);
    // Real Spanish overlay validation, including voice-over-text precedence.
    std::string text_manifest,voice_manifest="UCSPANISHVOICES 1 29\n";
    for(int n=1;n<=27;++n){const auto name=std::to_string(n)+"-01.ent";
        Write(root/"translations/es-zarf"/name,std::string(200,'S'));
        Write(root/"translations/es-voices"/name,std::string(200,'V'));
        text_manifest+=name+" 200\n";voice_manifest+=name+" 200\n";
    }
    Write(root/"translations/es-zarf/manifest.txt",text_manifest);
    Write(root/"translations/es-voices/1.saf","Voice stream");voice_manifest+="1.saf 12\n";
    Write(root/"translations/es-voices/movies/test.xmv","Voice movie");voice_manifest+="movies/test.xmv 11\n";
    Write(root/"translations/es-voices/manifest.txt",voice_manifest);
    Iso(root/"game.iso");auto disc=std::make_unique<Disc>();std::string error;CHECK(disc->Open(root/"game.iso",error));
    MountDisc(std::move(disc));MountDefaults(assets.string(),(root/"data").string());
    CreateSymbolicLink("\\??\\E:","\\Device\\Harddisk0\\Partition1");
    Read("D:\\original.bin","Disc original");Read("D:\\1-01.ent",std::string(200,'E'));
    for(const auto* name:kPreparedArchives){
        const auto expected=std::string("Prepared ")+name;
        Read(std::string("D:\\")+name,expected);Read(std::string("D:\\")+name,expected,true);
        Read("D:\\"+Upper(name),expected);Listing(name,expected.size());ReadOnly(std::string("D:\\")+name);
    }
    // The main generated archive may collide with a disc entry; other three
    // are absent from the ISO. Both paths have identical guest semantics.
    CHECK(!MountedDisc()->Find("coopinjection.lst"));
    CHECK(!ResolvePath("D:\\coopinjection.lst",0).empty());
    CHECK(ResolvePath("D:\\original.bin",0).empty());
    Read("D:\\.\\coopcharacters.lst","Disc decoy"); // no expanded local override scope
    auto dir=Open("D:\\",0x21);Read("coopinjection.lst","Prepared coopinjection.lst",false,dir);handles.erase(dir);
    std::vector<uint8_t> content;CHECK(ReadGameFile("D:\\coopinjection.lst",content));
    CHECK(std::string(content.begin(),content.end())=="Prepared coopinjection.lst");
    Write(assets/"unlisted.lst","Must not shadow disc");CHECK(!ReadGameFile("D:\\unlisted.lst",content));
    language=4;Read("D:\\1-01.ent",std::string(200,'V'));Listing("1-01.ent",200);ReadOnly("D:\\1-01.ent");
    Read("D:\\1.saf","Voice stream");Read("D:\\Movies\\test.xmv","Voice movie");
    language=0;Read("D:\\1-01.ent",std::string(200,'E'));
    Write(root/"data/E/save.bin","Save data");Read("E:\\save.bin","Save data");
    auto save=Open("E:\\save.bin");CHECK(!FileFromHandle(save)->read_only);
    CHECK(Rename(save,"D:\\coopinjection.lst")==kAccessDenied);
    W64(0x280,0);memory[0x1000]='N';CHECK(Call(236,{save,0,0,0,0x220,0x1000,1,0x280})==kSuccess);handles.erase(save);
    Read("E:\\save.bin","Nave data");
    CHECK(fs::remove(assets/"coopcharacters.lst"));Name("D:\\coopcharacters.lst");
    CHECK(Call(190,{0x200,0x80100080,0x300,0x220,0,0x80,1,1,0x20})==kObjectNameNotFound);
    CHECK(!ReadGameFile("D:\\coopcharacters.lst",content)); // never fall back to decoy
    Name("D:\\coopcharacters.lst");CHECK(Call(210,{0x300,0x800})==kObjectNameNotFound);
    MountDisc(nullptr);Read("D:\\original.bin","Stale local original");
    Read("D:\\coopinjection.lst","Prepared coopinjection.lst");
    language=4;Read("D:\\1-01.ent",std::string(200,'V'));
    CHECK(handles.empty());fs::current_path(initial);
    // Only this newly-created private fixture directory can be removed.
    CHECK(fs::weakly_canonical(root).parent_path()==fs::weakly_canonical(fs::temp_directory_path()));
    CHECK(root.filename().wstring().find(L"ucrr-disc-files-")==0);fs::remove_all(root);
    std::puts("disc files: actual ISO/extracted reads, prepared additions, Spanish voice/text priority, metadata, read-only guards and writable saves passed");
}
