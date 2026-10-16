#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <wrl/client.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include "app/app.h"
#include "xbe_shader_cache.h"
#include "nv2a_shader_disk_cache.h"
#include "nv2a_shader_compiler_identity.h"

namespace uc_xbe_shader_cache {
namespace {
using Microsoft::WRL::ComPtr;
struct Identity {
    std::array<uint32_t,6> contract{};
    std::array<unsigned char,32> digest{};
    bool valid=false;
};

Identity CompilerIdentity() noexcept {
    // Reuse the old export-owner check, and distinguish compiler binaries with
    // the same PE timestamp/size. Unknown or unreadable compiler means no cache.
    try {
        Identity result;
        HMODULE module=nv_shader_compiler_identity(result.contract.data());
        if(!module)return {};
        std::array<wchar_t,32768> path{};
        DWORD length=GetModuleFileNameW(module,path.data(),DWORD(path.size()));
        if(!length||length>=path.size())return {};
        std::ifstream file(std::filesystem::path(path.data()),std::ios::binary|std::ios::ate);
        if(!file)return {};
        const auto size=file.tellg();
        if(size<=0||size>64*1024*1024)return {};
        std::vector<unsigned char> bytes(static_cast<size_t>(size));
        file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))return {};
        if(BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,bytes.data(),ULONG(bytes.size()),
                      result.digest.data(),ULONG(result.digest.size()))<0)return {};
        result.contract[0]=0x55434431; // private D3D12 adapter contract, independent of EXE rebuilds
        result.contract[5]=D3D_COMPILER_VERSION;
        result.valid=true;
        return result;
    }catch(...){return {};}
}

struct Store {
    std::string directory;
    Identity compiler;
    Store(const std::filesystem::path& path,Identity identity):compiler(identity) {
        if(!compiler.valid)return;
        std::error_code error;
        std::filesystem::create_directories(path,error);
        if(!error)directory=path.string();
    }
    bool Key(const std::string& source,const char* entry,const char* profile,
             uint32_t flags,uint32_t effect_flags,std::string& key,std::array<uint32_t,6>& contract) const {
        if(directory.empty()||!compiler.valid||!entry||!profile||source.size()>2*1024*1024-1024||
           source.find('\0')!=std::string::npos||std::strlen(entry)>128||std::strlen(profile)>128)return false;
        contract=compiler.contract;contract[1]=flags;contract[2]=effect_flags;
        key="UC-D3D12-DXBC-1:";
        constexpr char hex[]="0123456789abcdef";
        for(auto byte:compiler.digest){key+=hex[byte>>4];key+=hex[byte&15];}
        // Lengths keep identity unambiguous. The old cache compares this whole
        // key exactly, not just the filename hash or the compiler fingerprint.
        auto part=[&](const std::string& text){key+=std::to_string(text.size());key+=':';key+=text;};
        part(entry);part(profile);part(source);
        return true;
    }
    bool Load(const std::string& source,const char* entry,const char* profile,
              uint32_t flags,uint32_t effect_flags,ID3DBlob** code) const {
        if(!code||*code)return false;
        std::string key;std::array<uint32_t,6> contract{};
        if(!Key(source,entry,profile,flags,effect_flags,key,contract))return false;
        size_t size=0;
        std::unique_ptr<void,decltype(&std::free)> bytes(
            nv_shader_disk_load(directory.c_str(),key.c_str(),contract.data(),&size),&std::free);
        if(!bytes)return false;
        ComPtr<ID3D11ShaderReflection> reflection;
        if(FAILED(D3DReflect(bytes.get(),size,IID_PPV_ARGS(&reflection))))return false;
        ComPtr<ID3DBlob> blob;
        if(FAILED(D3DCreateBlob(size,&blob)))return false;
        std::memcpy(blob->GetBufferPointer(),bytes.get(),size);
        *code=blob.Detach();
        return true;
    }
    void Save(const std::string& source,const char* entry,const char* profile,
              uint32_t flags,uint32_t effect_flags,ID3DBlob* code) const {
        if(!code)return;
        std::string key;std::array<uint32_t,6> contract{};
        if(Key(source,entry,profile,flags,effect_flags,key,contract))
            nv_shader_disk_save(directory.c_str(),key.c_str(),contract.data(),
                                code->GetBufferPointer(),code->GetBufferSize());
    }
};

Store& Active() {
    // Same private writable root as the title's settings; I/O failures only miss.
    static Store store(app::ExeDir()/"shader-cache"/"d3d12",CompilerIdentity());
    return store;
}
}

bool Load(const std::string& source,const char* entry,const char* profile,
          uint32_t flags,uint32_t effect_flags,ID3DBlob** code) noexcept {
    try{return Active().Load(source,entry,profile,flags,effect_flags,code);}catch(...){return false;}
}
void Save(const std::string& source,const char* entry,const char* profile,
          uint32_t flags,uint32_t effect_flags,ID3DBlob* code) noexcept {
    try{Active().Save(source,entry,profile,flags,effect_flags,code);}catch(...){}
}
}
