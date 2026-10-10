#pragma once
#include "renderer.h"
extern "C" {
#include "../archive_reader/archive_reader.h"
#include "../material_asset.h"
#include "../texture_sequence.h"
#include "../archive_lookup.h"
}
#include <filesystem>
#include <memory>

struct BytesOwner {
    UCArchiveBytes value{};
    BytesOwner()=default;
    BytesOwner(const BytesOwner&)=delete;
    BytesOwner &operator=(const BytesOwner&)=delete;
    ~BytesOwner(){uc_archive_bytes_free(&value);}
};
using ArchiveOwner=std::unique_ptr<UCArchive,decltype(&uc_archive_close)>;
struct NativeTextureSequence {
    UCTextureSequence names{};
    std::vector<ComPtr<ID3D11ShaderResourceView>> frames;
    NativeTextureSequence()=default;
    NativeTextureSequence(const NativeTextureSequence&)=delete;
    NativeTextureSequence &operator=(const NativeTextureSequence&)=delete;
    ~NativeTextureSequence(){uc_texture_sequence_free(&names);}
};
/* Explicit renderer phase, independent of animation time. Optional states use
 * material order; absent states mean original constructor {index0,scalar1}.
 * Values remain read-only for the duration of the draw. */
struct NativeTextureContext {
    float renderer_phase;
    const UCTextureSequenceState *states=nullptr;
    size_t state_count=0;
};
struct NativeMaterial {
    ComPtr<ID3D11ShaderResourceView> texture;
    std::shared_ptr<const NativeTextureSequence> sequence;
    NativeDrawState state;
    bool supported() const {return state.untextured_material||texture!=nullptr||sequence!=nullptr;}
    ID3D11ShaderResourceView *texture_for(const NativeTextureContext *,size_t material_index) const;
};
std::vector<uint8_t> read_file(const std::filesystem::path &,size_t limit);
ArchiveOwner read_archive(const std::filesystem::path &,uint8_t hash[32]);
size_t find_resource(const UCArchive *,const char *name);
void checked_resource(const UCArchive *,const char *name,const char *sha256,BytesOwner &);
std::vector<NativeMaterial> load_materials(const std::filesystem::path &,const uint8_t hash[32],
    const UCArchive *,char *const *names,uint32_t count,UCNativeMaterialGeometry,bool parent_write_alpha,NativeRenderer &,UCArchiveLookup *texture_lookup=nullptr);
