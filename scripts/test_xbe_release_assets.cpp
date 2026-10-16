// Includes actual production FindGame; crypto.cpp supplies the real SHA1.
// Run privately with retail Default.xbe and certificate-only legacy Default.xbe
// paths. Neither image is embedded, copied into the fixture, or packaged.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
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
namespace kernel {
static unsigned mounts,opens;
Disc::~Disc()=default;
bool Disc::Open(const std::filesystem::path&,std::string&){++opens;return false;}
bool Disc::Load(std::string_view,std::vector<uint8_t>&) const {return false;}
void MountDisc(std::unique_ptr<Disc> disc){CHECK(!disc);++mounts;}
}
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
static void Reject(){
    app::GameFiles game{L"unchanged",{1,2,3}};
    const auto before=kernel::mounts;
    messages.clear();message_flags.clear();
    CHECK(!app::FindGame(game));
    CHECK(game.dir==L"unchanged"&&game.xbe==std::vector<uint8_t>({1,2,3}));
    CHECK(kernel::mounts==before&&kernel::opens==0&&app::exe_dir_calls==0);
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
    Env(nullptr);Reject();
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
        const auto before=kernel::mounts;
        CHECK(app::FindGame(game));
        CHECK(game.dir==image.parent_path()&&game.xbe==Read(image));
        CHECK(messages.empty()&&kernel::mounts==before+1&&kernel::opens==0&&app::exe_dir_calls==0);
    }
    Env(nullptr);
    CHECK(fs::remove(temp/IMAGE_XBE));
    CHECK(fs::remove(temp/L"other.iso"));
    CHECK(fs::remove(temp)); // exact fixture files only; no recursive removal
    std::puts("release assets: production FindGame + real SHA1 accepted both pinned files; missing, relative, tampered, unknown and ancestor/ISO fallback rejected");
}
