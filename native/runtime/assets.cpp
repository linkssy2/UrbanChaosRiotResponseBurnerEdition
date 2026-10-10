#include "assets.h"
#include "../texture_asset/texture_asset.h"
#include <bcrypt.h>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
struct CatalogOwner { UCNativeMaterialCatalog value{}; ~CatalogOwner() { uc_material_catalog_free(&value); } };
struct TextureOwner { UCTextureAsset value{}; ~TextureOwner() { uc_texture_free(&value); } };

std::vector<uint8_t> read_file(const std::filesystem::path &path,size_t limit) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);stream.exceptions(std::ios::badbit|std::ios::failbit);
    auto size=stream.tellg();if(size<0||(uint64_t)size>limit)throw std::runtime_error("Native asset exceeds size limit");
    std::vector<uint8_t> bytes((size_t)size);stream.seekg(0);stream.read((char*)bytes.data(),size);return bytes;
}
ArchiveOwner read_archive(const std::filesystem::path &path,uint8_t hash[32]) {
    auto source=read_file(path,UC_AR_MAX_FILE_BYTES);
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
    auto hashed=BCryptHash(algorithm,nullptr,0,source.data(),(ULONG)source.size(),hash,32);
    BCryptCloseAlgorithmProvider(algorithm,0);if(hashed<0)throw std::runtime_error("Asset identity check failed");
    UCArchive *raw=nullptr;
    UCArchiveStatus status=uc_archive_copy(source.data(),source.size(),&raw);
    if(status!=UC_AR_OK) throw std::runtime_error(std::string("Archive: ")+uc_archive_status(status));
    return ArchiveOwner(raw,uc_archive_close);
}
static std::string resource_key(const char *name) {
    std::string key;
    for(const unsigned char *p=(const unsigned char*)name;*p;++p) {
        if(*p>=128)throw std::runtime_error("Unrecovered non-ASCII resource key");
        if(*p=='/'||*p=='\\'){if(key.empty()||key.back()!='\\')key+='\\';}
        else key+=(char)(*p>='A'&&*p<='Z'?*p+32:*p);
    }
    return key;
}
size_t find_resource(const UCArchive *archive,const char *name) {
    auto key=resource_key(name);size_t selected=SIZE_MAX;
    for(size_t i=0;i<uc_archive_count(archive);++i) {
        auto entry=uc_archive_entry(archive,i);
        if(resource_key((const char*)entry->name)==key) {
            if(selected!=SIZE_MAX||entry->selection_tag)throw std::runtime_error("Resource requires unresolved variant selection");
            selected=i;
        }
    }
    return selected;
}
std::vector<NativeMaterial> load_materials(const std::filesystem::path &path,
    const uint8_t archive_hash[32],const UCArchive *archive,char *const *names,uint32_t count,UCNativeMaterialGeometry geometry,bool parent_write_alpha,NativeRenderer &renderer,UCArchiveLookup *texture_lookup) {
    if(texture_lookup&&texture_lookup->archive!=archive)throw std::invalid_argument("Texture lookup belongs to another archive");
    auto texture_index=[&](const char *name){
        if(!texture_lookup)return find_resource(archive,name);
        const auto key=resource_key(name);size_t selected;
        if(uc_archive_lookup(texture_lookup,key.c_str(),&selected))throw std::runtime_error("Unsupported texture lookup");
        return selected;
    };
    auto bytes=read_file(path,16u*1024u*1024u);CatalogOwner catalog;
    if(uc_material_catalog_decode(bytes.data(),bytes.size(),&catalog.value)!=UC_MATERIAL_OK)
        throw std::runtime_error("Invalid offline native material catalog");
    if(memcmp(archive_hash,catalog.value.source_archive_sha256,32))throw std::runtime_error("Material catalog belongs to another archive");
    std::map<std::string,ComPtr<ID3D11ShaderResourceView>> textures;
    auto load_texture=[&](const char *name) {
        auto key=resource_key(name);auto found=textures.find(key);
        if(found!=textures.end())return found->second;
        size_t selected=texture_index(name);
        if(selected==SIZE_MAX)throw std::runtime_error(std::string("Missing native material texture: ")+name);
        TextureOwner texture;
        if(uc_texture_load(archive,selected,UC_AR_LOCAL_PAYLOAD,&texture.value)!=UC_TEXTURE_OK)
            throw std::runtime_error("Unsupported native material texture");
        auto uploaded=renderer.upload(texture.value);textures.emplace(key,uploaded);return uploaded;
    };
    std::vector<NativeMaterial> views(count);
    for(uint32_t m=0;m<count;++m) {
        const UCNativeMaterial *material=nullptr;
        const auto key=resource_key(names[m]);
        for(uint32_t i=0;i<catalog.value.count;++i) if(key==resource_key(catalog.value.materials[i].name)) {
            if(material)throw std::runtime_error("Duplicate native material name");material=&catalog.value.materials[i];
        }
        if(!material||material->kind==UC_NATIVE_MATERIAL_UNSUPPORTED)continue;
        if(material->geometry!=geometry)throw std::runtime_error("Native material geometry mismatch");
        /* Preview parent already uses CULL_NONE; the recovered NONE override
         * has the same result. Original inherited cull/depth/blend is pending. */
        auto &state=views[m].state;
        state.vertex_alpha=material->alpha_mode==UC_NATIVE_MATERIAL_ALPHA_VERTEX;
        state.write_alpha=parent_write_alpha||material->color_write==UC_NATIVE_MATERIAL_WRITE_RGBA;
        state.clamp_v=material->address_v==UC_NATIVE_MATERIAL_ADDRESS_V_CLAMP;
        state.clamp_u=material->address_u==UC_NATIVE_MATERIAL_ADDRESS_U_CLAMP;
        state.constant_color=material->kind==UC_NATIVE_MATERIAL_TEXTURE_VERTEX_CONSTANT;
        state.untextured_material=material->kind==UC_NATIVE_MATERIAL_VERTEX_CONSTANT;
        state.alpha_prepass=material->kind==UC_NATIVE_MATERIAL_ALPHA_PREPASS;
        state.prepass_two_sided=material->cull_mode==UC_NATIVE_MATERIAL_CULL_NONE;
        state.blend_mode=(uint8_t)material->blend_mode;state.depth_bias=material->depth_bias;
        state.vertex_exposure=material->vertex_exposure!=0;
        memcpy(state.uv_period,material->uv_period,sizeof(state.uv_period));
        state.rotor_uv=material->rotor_uv!=0;
        /* This development scene has no active exposure owner. Original96C66
         * supplies white raw119 in that case; do not bake exposure into assets. */
        uint32_t argb=material->constant_argb;
        state.constant[0]=((argb>>16)&255)/255.f;state.constant[1]=((argb>>8)&255)/255.f;
        state.constant[2]=(argb&255)/255.f;state.constant[3]=(argb>>24)/255.f;
        state.vertex_alpha_scale=state.vertex_alpha?state.constant[3]:1.f;
        if(state.untextured_material)continue;
        if(material->texture_selection==UC_NATIVE_MATERIAL_TEXTURE_SEQUENCE_PERIOD1) {
            size_t selected=texture_index(material->texture_name);BytesOwner sequence_bytes;
            auto sequence=std::make_shared<NativeTextureSequence>();
            if(selected==SIZE_MAX||uc_archive_read(archive,selected,UC_AR_LOCAL_PAYLOAD,&sequence_bytes.value)!=UC_AR_OK||
               uc_texture_sequence_decode(sequence_bytes.value.data,sequence_bytes.value.size,&sequence->names)!=UC_SEQUENCE_OK)
                throw std::runtime_error("Unsupported native texture sequence");
            for(uint32_t i=0;i<sequence->names.count;++i)sequence->frames.push_back(load_texture(sequence->names.names[i]));
            views[m].sequence=std::move(sequence);
        } else views[m].texture=load_texture(material->texture_name);
    }
    return views;
}
ID3D11ShaderResourceView *NativeMaterial::texture_for(const NativeTextureContext *context,size_t index) const {
    if(!sequence)return texture.Get();
    if(!context||(!context->states&&context->state_count)||
       (context->states&&index>=context->state_count))throw std::runtime_error("Texture sequence requires an explicit valid draw context");
    // Header8 alpha images copy the original model resource index directly
    // (8FF30 ->8E9A0 ->2BAFB0); no ordinary4A timer script runs here.
    if(state.alpha_prepass){
        int32_t frame=context->states?context->states[index].index:0;
        if(frame<0||size_t(frame)>=sequence->frames.size())throw std::runtime_error("Alpha image index outside texture sequence");
        return sequence->frames[size_t(frame)].Get();
    }
    UCTextureSelection selected{};
    if(uc_texture_sequence_select(&sequence->names,context->states?context->states+index:nullptr,
        context->renderer_phase,&selected)!=UC_SEQUENCE_OK)
        throw std::runtime_error("Texture sequence draw input is outside the proved domain");
    return sequence->frames[selected.index].Get();
}

void checked_resource(const UCArchive *archive,const char *name,const char *expected,BytesOwner &bytes) {
    const size_t index=find_resource(archive,name);
    if(index==SIZE_MAX||uc_archive_read(archive,index,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)
        throw std::runtime_error("Missing authored HUD resource");
    BCRYPT_ALG_HANDLE algorithm=nullptr;unsigned char hash[32];char hex[65];
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)
        throw std::runtime_error("SHA256 unavailable");
    auto status=BCryptHash(algorithm,nullptr,0,bytes.value.data,(ULONG)bytes.value.size,hash,32);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(status<0)throw std::runtime_error("HUD resource identity check failed");
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",hash[i]);
    if(strcmp(hex,expected))throw std::runtime_error("Unsupported authored HUD resource revision");
}
