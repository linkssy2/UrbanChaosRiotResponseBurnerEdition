#pragma once
#include "assets.h"
#include "character_dialogue.h"
extern "C" {
#include "../font.h"
}
class NativeMissionWorld;
class NativeMissionCharacter;
class NativeMissionPlayer;

//7D000/7CD80/7CAA0/7CB30: retained NPC and Dispatch subtitle rows. Original font/English
//printable text; subtitle type3+ and control glyphs remain unsupported.
class NativeSubtitles {
    struct Row {int32_t handle,actor;NativeCharacterDialogue *dialogue;std::string text;float color[4];unsigned lines;NativeGpuMesh mesh;};
    int add_row(const NativeDialogueLine &,NativeCharacterDialogue &,int32_t,const float *,int,int,NativeMissionWorld &,NativeMissionPlayer &,int32_t *);
    std::vector<Row> rows_;
    BytesOwner metrics_;UCFont font_{};
    ComPtr<ID3D11ShaderResourceView> texture_;
    NativeRenderer *renderer_=nullptr;int32_t next_=1;
    NativeGpuMesh fps_mesh_;float fps_value_=-1,fps_width_=0;uint32_t fps_indices_=0;
public:
    void load(const std::filesystem::path &game,NativeRenderer &);
    int add(const NativeDialogueLine &,NativeMissionCharacter &,NativeMissionWorld &,NativeMissionPlayer &,int32_t *handle);
    int add_dispatch(const NativeDialogueLine &,NativeCharacterDialogue &,int32_t,const float *,int,NativeMissionWorld &,NativeMissionPlayer &,int32_t *);
    void remove_actor(int32_t);
    void step(NativeMissionWorld &);
    void draw(NativeRenderer &,bool enabled=true);
    void draw_fps(NativeRenderer &,float fps);
    size_t size()const{return rows_.size();}
};
