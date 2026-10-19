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
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "app/app.h"
#include "xbe_shader_cache.h"
#include "nv2a_shader_disk_cache.h"
#include "nv2a_shader_compiler_identity.h"
#include "xbe_pipeline_contract.h"

namespace uc_xbe_shader_cache {
namespace {
using Microsoft::WRL::ComPtr;
struct CacheWriter {
    struct Job {std::string directory,key;std::array<uint32_t,6> contract;ComPtr<ID3DBlob> code;};
    std::mutex mutex;std::condition_variable ready,idle;std::deque<Job> jobs;size_t bytes=0;
    CacheWriter(){std::thread([this]{
        for(;;){
            Job job;
            {std::unique_lock lock(mutex);ready.wait(lock,[&]{return !jobs.empty();});job=std::move(jobs.front());jobs.pop_front();}
            nv_shader_disk_save(job.directory.c_str(),job.key.c_str(),job.contract.data(),
                job.code->GetBufferPointer(),job.code->GetBufferSize());
            {std::lock_guard lock(mutex);bytes-=job.key.size()+job.code->GetBufferSize();if(!bytes)idle.notify_all();}
        }
    }).detach();}
    void Save(const std::string& directory,const std::string& key,const std::array<uint32_t,6>& contract,ID3DBlob* code){
        if(!code||directory.empty()||!code->GetBufferSize()||code->GetBufferSize()>16*1024*1024)return;
        const size_t size=key.size()+code->GetBufferSize();std::lock_guard lock(mutex);
        // A full optional cache queue drops persistence, never stalls a draw.
        if(jobs.size()>=128||size>32*1024*1024-bytes)return;
        jobs.push_back({directory,key,contract,code});bytes+=size;ready.notify_one();
    }
};
CacheWriter& Writer(){static auto* writer=new CacheWriter;return *writer;} // Matches the process-owned audio/render workers.
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
    void Warm(const WarmSink& sink) const {
        if(directory.empty()||!compiler.valid)return;
        size_t visited=0,total=0;
        for(const auto& item:std::filesystem::directory_iterator(directory)) {
            if(++visited>2048||total>=64*1024*1024)break;
            if(item.is_symlink()||!item.is_regular_file()||item.path().extension()!=".bin")continue;
            std::ifstream file(item.path(),std::ios::binary);
            NvShaderDiskHeader h{};
            if(!file.read(reinterpret_cast<char*>(&h),sizeof(h))||h.magic!=0x5343564E||h.version!=1||
               !h.source_bytes||h.source_bytes>2*1024*1024||!h.code_bytes||h.code_bytes>16*1024*1024||
               h.contract[1]!=D3DCOMPILE_OPTIMIZATION_LEVEL1||h.contract[2]!=0)continue;
            // Stale or truncated records must not consume the useful warm-up budget.
            std::error_code error;
            if(item.file_size(error)!=sizeof(h)+uint64_t(h.source_bytes)+h.code_bytes||error)continue;
            std::string key(h.source_bytes,'\0');
            if(!file.read(key.data(),key.size()))continue;
            constexpr const char* prefix="UC-D3D12-DXBC-1:";
            if(key.compare(0,std::strlen(prefix),prefix)!=0)continue;
            size_t at=std::strlen(prefix)+64;
            auto part=[&](std::string& value,size_t limit){
                size_t n=0,digits=0;
                while(at<key.size()&&key[at]>='0'&&key[at]<='9'){
                    if(++digits>7)return false;
                    n=n*10+size_t(key[at++]-'0');if(n>limit)return false;
                }
                if(!digits||at>=key.size()||key[at++]!=':'||n>key.size()-at)return false;
                value=key.substr(at,n);at+=n;return value.find('\0')==std::string::npos;
            };
            std::string entry,profile,source;
            if(!part(entry,128)||!part(profile,128)||!part(source,2*1024*1024)||at!=key.size())continue;
            std::string expected;std::array<uint32_t,6> contract{};
            if(!Key(source,entry.c_str(),profile.c_str(),h.contract[1],h.contract[2],expected,contract)||
               expected!=key||std::memcmp(contract.data(),h.contract,sizeof(h.contract)))continue;
            const size_t bytes=size_t(h.source_bytes)+h.code_bytes;
            if(bytes>64*1024*1024-total)continue;
            total+=bytes;
            ComPtr<ID3DBlob> code;
            // Use the normal validated reader: malformed records never invoke the compiler.
            if(Load(source,entry.c_str(),profile.c_str(),h.contract[1],h.contract[2],&code))
                sink(source,entry,profile,code.Get());
        }
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
};

Store& Active() {
    // Same private writable root as the title's settings; I/O failures only miss.
    static Store store(app::ExeDir()/"shader-cache"/"d3d12",CompilerIdentity());
    return store;
}

Store& Pipelines() {
    static Store store(app::ExeDir()/"shader-cache"/"d3d12-pipelines",Active().compiler);
    return store;
}

std::string PipelineKey(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& input) {
    // This renderer has one root-signature contract and no stream-output pipelines.
    if(input.StreamOutput.NumEntries||input.StreamOutput.NumStrides)return {};
    auto d=input;std::vector<unsigned char> bytes;
    auto append=[&](const void* data,size_t size){
        if(size>16*1024*1024-bytes.size()||(!data&&size))throw std::bad_alloc();
        if(size){auto* p=static_cast<const unsigned char*>(data);bytes.insert(bytes.end(),p,p+size);}
    };
    for(auto* shader:{&d.VS,&d.PS,&d.DS,&d.HS,&d.GS}){
        append(&shader->BytecodeLength,sizeof(shader->BytecodeLength));
        append(shader->pShaderBytecode,shader->BytecodeLength);shader->pShaderBytecode=nullptr;
    }
    for(UINT i=0;i<d.InputLayout.NumElements;++i){
        auto element=d.InputLayout.pInputElementDescs[i];
        if(!element.SemanticName)throw std::bad_alloc();
        append(element.SemanticName,std::strlen(element.SemanticName)+1);
        element.SemanticName=nullptr;append(&element,sizeof(element));
    }
    d.InputLayout.pInputElementDescs=nullptr;d.pRootSignature=nullptr;d.CachedPSO={};
    d.StreamOutput.pSODeclaration=nullptr;d.StreamOutput.pBufferStrides=nullptr;
    append(&d,sizeof(d));
    std::array<unsigned char,32> hash{};
    if(BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,bytes.data(),ULONG(bytes.size()),hash.data(),ULONG(hash.size()))<0)return {};
    std::string key=UC_PIPELINE_CONTRACT;constexpr char hex[]="0123456789abcdef";
    for(auto b:hash){key+=hex[b>>4];key+=hex[b&15];}return key;
}
}

HRESULT CreatePipeline(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                       ID3D12PipelineState** result) noexcept {
    if(!device||!result||*result)return E_INVALIDARG;
    std::string key;std::array<uint32_t,6> contract{};
    try {
        auto identity=PipelineKey(desc);
        if(!identity.empty()&&Pipelines().Key(identity,"pipeline","driver",0,0,key,contract)){
            size_t size=0;
            std::unique_ptr<void,decltype(&std::free)> blob(
                nv_shader_disk_load(Pipelines().directory.c_str(),key.c_str(),contract.data(),&size),&std::free);
            if(blob){
                auto cached=desc;cached.CachedPSO={blob.get(),size};
                if(SUCCEEDED(device->CreateGraphicsPipelineState(&cached,IID_PPV_ARGS(result))))return S_OK;
                if(*result){(*result)->Release();*result=nullptr;}
            }
        }
    }catch(...){key.clear();}
    // Driver/GPU changes or corrupt cache entries cannot prevent normal creation.
    auto uncached=desc;uncached.CachedPSO={};
    HRESULT hr=device->CreateGraphicsPipelineState(&uncached,IID_PPV_ARGS(result));
    if(SUCCEEDED(hr)&&!key.empty())try {
        ComPtr<ID3DBlob> blob;
        if(SUCCEEDED((*result)->GetCachedBlob(&blob)))
            Writer().Save(Pipelines().directory,key,contract,blob.Get());
    }catch(...){}
    return hr;
}

bool Load(const std::string& source,const char* entry,const char* profile,
          uint32_t flags,uint32_t effect_flags,ID3DBlob** code) noexcept {
    try{return Active().Load(source,entry,profile,flags,effect_flags,code);}catch(...){return false;}
}
void Warm(const WarmSink& sink) noexcept {
    try{Active().Warm(sink);}catch(...){/* Optional warm-up; demand loading remains available. */}
}
void Save(const std::string& source,const char* entry,const char* profile,
          uint32_t flags,uint32_t effect_flags,ID3DBlob* code) noexcept {
    try{
        std::string key;std::array<uint32_t,6> contract{};auto& store=Active();
        if(store.Key(source,entry,profile,flags,effect_flags,key,contract))Writer().Save(store.directory,key,contract,code);
    }catch(...){}
}
void FlushWrites(){auto& writer=Writer();std::unique_lock lock(writer.mutex);writer.idle.wait(lock,[&]{return !writer.bytes;});}
}
