// Includes actual production FindGame; crypto.cpp and disc.cpp provide real I/O.
// Run privately with retail Default.xbe and certificate-only legacy Default.xbe
// paths. Neither image is embedded, copied into the fixture, or packaged.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "app/app.h"
#include "core/log.h"
#include "kernel/disc.h"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

static std::vector<std::wstring> messages;
static std::vector<UINT> message_flags;
static int WINAPI FixtureMessageBoxW(HWND,LPCWSTR text,LPCWSTR,UINT flags){
    messages.emplace_back(text);message_flags.push_back(flags);
    return IDYES; // A forced Yes must never admit an unknown release.
}
namespace app {
static unsigned exe_dir_calls;
static std::filesystem::path fixture_exe_dir;
std::filesystem::path ExeDir(){++exe_dir_calls;return fixture_exe_dir;}
const GameInfo kGame=[] {GameInfo info{};info.short_title=L"Release assets fixture";info.missing=L"Fixture missing";return info;}();
}
namespace logx {void Write(Level,const char*,...) {}}
#define UC_BURNER_RELEASE 1
#define MessageBoxW FixtureMessageBoxW
#include "game_files.cpp"
#undef MessageBoxW

namespace fs=std::filesystem;
static std::vector<uint8_t> Read(const fs::path& path){
    std::ifstream input(path,std::ios::binary);CHECK(input.good());
    return {std::istreambuf_iterator<char>(input),{}};
}
static void Write(const fs::path& path,const std::vector<uint8_t>& data){
    std::ofstream output(path,std::ios::binary|std::ios::trunc);CHECK(output.good());
    output.write(reinterpret_cast<const char*>(data.data()),data.size());CHECK(output.good());
}
static void Env(const wchar_t* value){CHECK(SetEnvironmentVariableW(L"UCRR_GAME_ASSETS",value));}
static void IsoEnv(const wchar_t* value){CHECK(SetEnvironmentVariableW(L"UCRR_GAME_ISO",value));}
static void Iso(const fs::path& path,const std::vector<uint8_t>& xbe,uint64_t base=0){
    // Minimal real XDVDFS: one root entry and a complete, private original XBE.
    std::vector<uint8_t> volume(2048),table(40);
    const char* magic="MICROSOFT*XBOX*MEDIA";
    std::memcpy(volume.data(),magic,20);std::memcpy(volume.data()+0x7ec,magic,20);
    const uint32_t sector=33,table_size=uint32_t(table.size()),data_sector=40,size=uint32_t(xbe.size());
    std::memcpy(volume.data()+0x14,&sector,4);std::memcpy(volume.data()+0x18,&table_size,4);
    std::memcpy(table.data()+4,&data_sector,4);std::memcpy(table.data()+8,&size,4);
    table[13]=uint8_t(std::strlen(IMAGE_XBE));std::memcpy(table.data()+14,IMAGE_XBE,table[13]);
    std::ofstream output(path,std::ios::binary|std::ios::trunc);CHECK(output.good());
    output.seekp(base+32*2048);output.write(reinterpret_cast<const char*>(volume.data()),volume.size());
    output.write(reinterpret_cast<const char*>(table.data()),table.size());
    output.seekp(base+40*2048);output.write(reinterpret_cast<const char*>(xbe.data()),xbe.size());CHECK(output.good());
}
static void Reject(){
    app::GameFiles game{L"unchanged",{1,2,3}};
    const auto before=kernel::MountedDisc();
    messages.clear();message_flags.clear();
    CHECK(!app::FindGame(game));
    CHECK(game.dir==L"unchanged"&&game.xbe==std::vector<uint8_t>({1,2,3}));
    CHECK(kernel::MountedDisc()==before&&app::exe_dir_calls==0);
    CHECK(messages.size()==1&&messages[0].find(L"Setup")!=std::wstring::npos);
    CHECK(messages[0].find(L"Start it anyway")==std::wstring::npos);
    CHECK((message_flags[0]&MB_TYPEMASK)==MB_OK);
}
int wmain(int argc,wchar_t** argv){
    CHECK(argc==3);
    const fs::path retail=fs::absolute(argv[1]),legacy=fs::absolute(argv[2]);
    CHECK(fs::is_regular_file(retail)&&fs::is_regular_file(legacy));
    CHECK(retail.filename()==IMAGE_XBE&&legacy.filename()==IMAGE_XBE);
    const auto retail_bytes=Read(retail),legacy_bytes=Read(legacy);
    CHECK(app::Sha1(retail_bytes)=="fb9bbe0028b52f60a1b57b35b873093f334c2ce0");
    CHECK(app::Sha1(legacy_bytes)=="63805aa38ca20ac1db0712cba47dbcf38406ac58");
    // An actual supported assets directory exists above this apparent exe path.
    // Release discovery must not ask ExeDir or find it without Setup's variable.
    app::fixture_exe_dir=retail.parent_path().parent_path()/"out-msvc";
    IsoEnv(nullptr);Env(nullptr);Reject();
    Env(L".");Reject(); // existing relative directory still cannot be selected
    Env(retail.c_str());Reject(); // existing file is not a directory
    const auto temp=fs::temp_directory_path()/
        (L"ucrr-release-assets-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
    CHECK(!fs::exists(temp));fs::create_directories(temp);
    const auto missing=temp/L"missing";
    Env(missing.c_str());Reject();
    Env(temp.c_str());Reject(); // existing empty install requires Setup
    Write(temp/L"other.iso",{1,2,3});Reject(); // release never opens an ISO fallback
    Write(temp/IMAGE_XBE,{1,2,3});Reject();
    auto tampered=retail_bytes;CHECK(tampered.size()>0x200);tampered[0x200]^=1;
    Write(temp/IMAGE_XBE,tampered);Reject();
    for(const auto& image:{retail,legacy}){
        Env(image.parent_path().c_str());app::GameFiles game;messages.clear();
        CHECK(app::FindGame(game));
        CHECK(game.dir==image.parent_path()&&game.xbe==Read(image));
        CHECK(messages.empty()&&!kernel::MountedDisc()&&app::exe_dir_calls==0);
    }
    Env(temp.c_str());Write(temp/IMAGE_XBE,retail_bytes);
    IsoEnv(L"relative.iso");Reject();
    IsoEnv(missing.c_str());Reject();
    IsoEnv(temp.c_str());Reject();
    const auto iso=temp/L"disc-espa\u00f1ol.iso";
    IsoEnv(iso.c_str());Write(iso,{1,2,3});Reject();
    Iso(iso,{1,2,3});Reject(); // supported local XBE cannot admit wrong disc
    Iso(iso,tampered);Reject(); // same-size XBE still needs exact hash
    Iso(iso,legacy_bytes);Reject(); // two supported images must also match each other
    for(uint64_t partition:{0ull,0xFD90ull,0x30600ull,0x0FD90000ull,0x18300000ull}){
        Iso(iso,retail_bytes,partition);app::GameFiles game;messages.clear();
        CHECK(app::FindGame(game));
        CHECK(game.dir==temp&&game.xbe==retail_bytes&&messages.empty());
        CHECK(kernel::MountedDisc()&&kernel::MountedDisc()->Path()==iso);
        std::vector<uint8_t> mounted;CHECK(kernel::MountedDisc()->Load(IMAGE_XBE,mounted)&&mounted==retail_bytes);
        // Failed subsequent admission never silently unmounts or changes the source.
        IsoEnv(missing.c_str());Reject();IsoEnv(iso.c_str());
        kernel::MountDisc(nullptr);
    }
    Write(temp/IMAGE_XBE,legacy_bytes);Iso(iso,legacy_bytes);app::GameFiles game;
    CHECK(app::FindGame(game)&&kernel::MountedDisc());kernel::MountDisc(nullptr);
    Write(temp/IMAGE_XBE,tampered);Reject();
    IsoEnv(nullptr);Env(nullptr);
    CHECK(fs::remove(temp/IMAGE_XBE));
    CHECK(fs::remove(temp/L"other.iso"));
    CHECK(fs::remove(iso));
    CHECK(fs::remove(temp)); // exact fixture files only; no recursive removal
    std::puts("release assets: actual FindGame + SHA1 + Disc accepted both pinned extracted/direct sources and all five ISO layouts; wrong/missing/relative/tampered sources fail closed");
}
