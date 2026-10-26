// Extracted production bind/retire methods; fake resources expose GPU ownership.
// Actual D3D12 upload/raster output and title visuals remain outside this fixture.
#define UC_BURNER_RELEASE
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
#include "xbe_hd_ui.h"
#include "xbe_pc_prompts.h"

#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::abort(); } } while (0)
static unsigned checks;
using UINT16=uint16_t; using ULONGLONG=uint64_t;
enum DXGI_FORMAT { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_B8G8R8A8_UNORM,
                   DXGI_FORMAT_R32G32B32A32_FLOAT, ORIGINAL_FORMAT };
enum D3D12_RESOURCE_STATES { D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
enum { D3D12_HEAP_TYPE_DEFAULT, D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
       D3D12_RESOURCE_DIMENSION_TEXTURE3D };
struct D3D12_HEAP_PROPERTIES { int type; };
struct D3D12_RESOURCE_DESC {
    int Dimension=0; uint64_t Width=0; uint32_t Height=0;
    uint16_t DepthOrArraySize=1, MipLevels=1; DXGI_FORMAT Format=DXGI_FORMAT_UNKNOWN;
    struct { unsigned Count=1; } SampleDesc;
};
struct D3D12_SHADER_RESOURCE_VIEW_DESC {
    DXGI_FORMAT Format=DXGI_FORMAT_UNKNOWN;
    unsigned ViewDimension=2;
    struct { unsigned MipLevels=1; } Texture2D;
};
struct ID3D12Resource { unsigned id; D3D12_RESOURCE_DESC desc; std::vector<uint8_t> pixels; };
template<class T> struct ComPtr {
    std::shared_ptr<T> ptr;
    T* Get()const{return ptr.get();}
    explicit operator bool()const{return bool(ptr);}
    void Reset(){ptr.reset();}
};
#define FAILED(x) ((x)!=0)
#define IID_PPV_ARGS(x) x
static unsigned creations=0, failed_creations=0;
struct Device {
    int CreateCommittedResource(const D3D12_HEAP_PROPERTIES*,int,const D3D12_RESOURCE_DESC* d,
                                D3D12_RESOURCE_STATES,void*,ComPtr<ID3D12Resource>* p) {
        ++creations;
        if(failed_creations){--failed_creations;return -1;}
        p->ptr=std::make_shared<ID3D12Resource>(ID3D12Resource{creations,*d,{}});return 0;
    }
};
static D3D12_RESOURCE_DESC Texture2DDesc(DXGI_FORMAT format,uint64_t w,uint32_t h){
    D3D12_RESOURCE_DESC d;d.Format=format;d.Width=w;d.Height=h;return d;
}
struct TextureDesc {
    uint32_t format=15,width=32,height=1024,depth=1,levels=9,pitch=0,data=1;
    bool cube=false,linear=false;
};
struct FormatInfo { int host=0;bool compressed=false,depth=false; };
static DXGI_FORMAT HostFormat(int){return ORIGINAL_FORMAT;}
namespace uc_xbe_signed {
struct TextureKey {uint32_t data,palette,signs;bool operator==(const TextureKey&)const=default;};
struct TextureKeyHash {size_t operator()(const TextureKey& k)const{return k.data^(k.palette<<1)^(k.signs<<2);}};
static uint32_t CubeSigns(const TextureDesc&,uint32_t signs){return signs;}
}
struct State {unsigned uc_prompt_variant=0,uc_prompt_player=0; std::array<std::array<unsigned,1>,4> tss{};};
static constexpr unsigned kTssColorSign=0;
struct TexInfo {unsigned shadow=0;};
struct DrawTextures {
    std::array<TexInfo,4> info{};
    std::array<D3D12_SHADER_RESOURCE_VIEW_DESC,4> sv{};
    std::array<unsigned,4> width{32,7,8,9},height{1024,11,12,13};
};
/* PRODUCTION_TEX */
static uint64_t revision=1,palette_hash=0;
static uint32_t palette_va=0;
static std::vector<uint8_t> source_bytes(44016,0x52);
namespace kernel {static void* HostPtr(uint32_t){return source_bytes.data();}}
static uint64_t MemSig(uint32_t,uint32_t){return revision;}
static uint64_t DirtyEpoch(uint32_t){return 0;}
static unsigned size_calls=0;
static uint32_t TotalSize(const TextureDesc&){++size_calls;return uint32_t(source_bytes.size());}
struct StagePalette {uint32_t colours[256]{};uint32_t va=0;uint64_t hash=0;};
static void ReadStagePalette(const State&,int,const TextureDesc&,StagePalette& p){p.va=palette_va;p.hash=palette_hash;}
static unsigned ShadowDepthKind(uint32_t){return 1;}
static void FillTextureView(const Tex& tx,const TextureDesc&,D3D12_SHADER_RESOURCE_VIEW_DESC& sv){
    sv.Format=tx.fmt;sv.Texture2D.MipLevels=tx.levels;
}
namespace uc_hd_ui {
static Image first{128,4096,std::vector<uint8_t>(size_t(128)*4096*4,0x6e)};
static Image second{64,2048,std::vector<uint8_t>(size_t(64)*2048*4,0x97)};
static const Image* selected=&first;
static unsigned matches=0;static size_t matched_bytes=0;
const Image* Match(uint32_t w,uint32_t h,uint32_t f,const uint8_t* p,size_t n){
    ++matches;matched_bytes=n;return w&&h&&f&&p?selected:nullptr;
}
}
namespace trial_graphics {static bool enabled=false;bool HdUi(){return enabled;}}
static bool prompt_active=false;static TrialPromptInput prompt_input;
static unsigned seen_player=0,seen_variant=0;
bool TrialPromptSnapshot(unsigned,TrialPromptInput& out,unsigned variant,unsigned player){
    seen_player=player;seen_variant=variant;out=prompt_input;return prompt_active;
}
static bool art_enabled=false;static unsigned art_calls=0;static ULONGLONG now=100;
static ULONGLONG GetTickCount64(){return now;}
bool TrialPromptArtwork(unsigned,const uint8_t*,const TrialPromptInput&,std::vector<uint8_t>& px,
                        unsigned& w,unsigned& h,bool hd_ui){
    ++art_calls;art_enabled=hd_ui;w=h=4;px.assign(64,hd_ui?0xaa:0x55);return true;
}
struct Extension {void Sampled(const TextureDesc&,void*,uint8_t,TexInfo&){};};
class D3D12Backend {
public:
    Device device;Device* dev_=&device;Extension* ext_=nullptr;
    struct Frame {std::vector<ComPtr<ID3D12Resource>> garbage;} frame;
    Frame& CurrentFrame(){return frame;}
    uint64_t presents_=0;
    std::unordered_map<uc_xbe_signed::TextureKey,Tex,uc_xbe_signed::TextureKeyHash> textures_;
    struct PcPromptSlot {Tex texture;TrialPromptInput input;ULONGLONG updated=0;bool hd_ui=false;
        D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COPY_DEST;};
    PcPromptSlot pc_prompts_[5];
    unsigned uploads=0,original_uploads=0;
    bool CreateTexture(Tex&,const TextureDesc&,const FormatInfo&,uc_xbe_signed::TextureKey);
    void RetireHdUi(Tex&);
    void MatchHdUi(Tex&,const TextureDesc&,const FormatInfo&,const uint8_t*,uint32_t);
    ID3D12Resource* BindHdUi(Tex&,DrawTextures&,int);
    ID3D12Resource* BindGuestTexture(const State&,int,const TextureDesc&,const FormatInfo&,DrawTextures&);
    ID3D12Resource* BindPcPrompt(unsigned,unsigned,unsigned,const TextureDesc&,DrawTextures&,int);
    void ReleaseOldTextures();
    void UploadTexture(Tex&,const TextureDesc&,const FormatInfo&,const uint8_t*,const uint32_t*,bool){++original_uploads;}
    void UploadTextureLevel(Tex& tx,uint32_t f,uint32_t l,const std::vector<uint8_t>& px,uint32_t pitch){
        CHECK(f==0&&l==0&&pitch==tx.res.Get()->desc.Width*4);++uploads;tx.res.Get()->pixels=px;
    }
    void Transition(ID3D12Resource*,D3D12_RESOURCE_STATES& state,D3D12_RESOURCE_STATES next){state=next;}
};
/* PRODUCTION_METHODS */

int main(){
    D3D12Backend b;State state;TextureDesc d;FormatInfo fi;DrawTextures stages;
    auto bind=[&]{return b.BindGuestTexture(state,0,d,fi,stages);};
    auto* original=bind();CHECK(original&&uc_hd_ui::matches==1&&b.original_uploads==1&&b.uploads==0);
    CHECK(uc_hd_ui::matched_bytes==44016); // entire authored chain, not just base
    for(int i=0;i<200;++i)CHECK(bind()==original);
    CHECK(size_calls==1&&uc_hd_ui::matches==1);
    auto& tx=b.textures_.begin()->second;
    CHECK(tx.hd_ui_image==&uc_hd_ui::first&&!tx.hd_ui_res);
    trial_graphics::enabled=true;auto* hd=bind();CHECK(hd&&hd!=original&&b.uploads==1);
    CHECK(hd->desc.Width==128&&hd->desc.Height==4096&&hd->pixels==uc_hd_ui::first.bgra);
    CHECK(stages.width[0]==32&&stages.height[0]==1024&&tx.d.width==32&&tx.d.height==1024&&tx.levels==9);
    CHECK(stages.sv[0].Format==DXGI_FORMAT_B8G8R8A8_UNORM&&stages.sv[0].Texture2D.MipLevels==1);
    for(int i=0;i<50;++i){trial_graphics::enabled=false;CHECK(bind()==original);
        CHECK(stages.sv[0].Format==ORIGINAL_FORMAT&&stages.sv[0].Texture2D.MipLevels==9);
        trial_graphics::enabled=true;CHECK(bind()==hd);}
    CHECK(b.uploads==1&&b.frame.garbage.empty());
    std::weak_ptr<ID3D12Resource> held=tx.hd_ui_res.ptr;
    uc_hd_ui::selected=nullptr;++revision;++b.presents_;CHECK(bind()==original);
    CHECK(!held.expired()&&b.frame.garbage.size()==1&&!tx.hd_ui_image&&!tx.hd_ui_res);
    b.frame.garbage.clear();CHECK(held.expired());
    uc_hd_ui::selected=&uc_hd_ui::second;++revision;++b.presents_;auto* hd2=bind();
    CHECK(hd2!=original&&hd2->desc.Width==64&&b.uploads==2);
    // CPU source changes to another recognized identity retire the old GPU image.
    held=tx.hd_ui_res.ptr;uc_hd_ui::selected=&uc_hd_ui::first;++revision;++b.presents_;CHECK(bind()!=hd2);
    CHECK(!held.expired());b.frame.garbage.clear();CHECK(held.expired());
    // Failed allocations and malformed prepared images never spin per draw.
    b.RetireHdUi(tx);tx.hd_ui_image=&uc_hd_ui::second;failed_creations=1;
    unsigned before=creations;for(int i=0;i<20;++i)CHECK(b.BindHdUi(tx,stages,0)==nullptr);
    CHECK(creations==before+1&&!tx.hd_ui_res);
    uc_hd_ui::Image malformed{128,4096,{}};b.RetireHdUi(tx);tx.hd_ui_image=&malformed;
    before=creations;CHECK(!b.BindHdUi(tx,stages,0)&&creations==before);
    // Unsupported modes stay original and never enter the manifest matcher.
    auto reject=[&]{unsigned n=uc_hd_ui::matches;b.MatchHdUi(tx,d,fi,source_bytes.data(),44016);
        CHECK(uc_hd_ui::matches==n&&!tx.hd_ui_image);};
    tx.signs=1;reject();tx.signs=0;d.cube=true;reject();d.cube=false;
    d.depth=2;reject();d.depth=1;d.linear=true;reject();d.linear=false;
    fi.depth=true;reject();fi.depth=false;d.format=0x0b;reject();d.format=15;
    Extension extension;b.ext_=&extension;reject();b.ext_=nullptr;
    // Layout replacement retains both original and HD resources until retirement.
    tx.hd_ui_image=&uc_hd_ui::first;CHECK(b.BindHdUi(tx,stages,0));held=tx.hd_ui_res.ptr;
    auto old_original=tx.res.ptr;d.width=64;CHECK(b.CreateTexture(tx,d,fi,{d.data,0,0}));
    CHECK(!held.expired()&&!tx.hd_ui_image&&tx.d.width==64);
    b.frame.garbage.clear();CHECK(held.expired());
    // Dynamic prompts have priority; toggles force immediate redraw inside 33ms.
    prompt_active=true;trial_graphics::enabled=false;now=100;
    auto* prompt=b.BindPcPrompt(1,0,0,d,stages,0);CHECK(prompt&&art_calls==1&&!art_enabled);
    now=101;trial_graphics::enabled=true;CHECK(b.BindPcPrompt(1,0,0,d,stages,0)==prompt);
    CHECK(art_calls==2&&art_enabled&&prompt->pixels[0]==0xaa);
    trial_graphics::enabled=false;CHECK(b.BindPcPrompt(1,0,0,d,stages,0)==prompt);
    CHECK(art_calls==3&&!art_enabled&&prompt->pixels[0]==0x55);
    prompt_input.held=true;CHECK(b.BindPcPrompt(2,0,0,d,stages,0));CHECK(art_calls==4);
    // Orders text, drag cue, interaction text and controller art cannot alias.
    auto* orders=b.BindPcPrompt(1,2,0,d,stages,0);
    auto* interaction=b.BindPcPrompt(1,1,0,d,stages,0);
    prompt_input.controller=true;auto* controller=b.BindPcPrompt(1,2,0,d,stages,0);
    CHECK(orders&&interaction&&controller&&orders!=prompt&&orders!=interaction&&orders!=controller);
    CHECK(orders!=b.pc_prompts_[1].texture.res.Get());
    prompt_input.controller=false;CHECK(b.BindPcPrompt(1,2,0,d,stages,0)==orders);
    // The same atlas is bound for both queued HUD owners without global mode reads.
    bind(); // finish the preceding layout/content refresh before tagging this fixture atlas
    tx.pc_prompt_id=1;state.uc_prompt_variant=1;state.uc_prompt_player=1;
    CHECK(bind()==interaction&&seen_player==1&&seen_variant==1);
    prompt_input.controller=true;state.uc_prompt_player=2;
    CHECK(bind()==controller&&seen_player==2&&seen_variant==1);
    const unsigned prior_art=art_calls;
    prompt_input.bindings[0]=4;
    CHECK(bind()==controller&&art_calls==prior_art+1); // no 33ms stale binding
    prompt_input.controller=false;state.uc_prompt_player=0;state.uc_prompt_variant=0;
    CHECK(bind()==prompt&&seen_player==0&&seen_variant==0);
    // Normal idle eviction also retains the prepared image until the frame fence.
    b.textures_.clear();b.frame.garbage.clear();d.width=32;
    Tex sample;CHECK(b.CreateTexture(sample,d,fi,{d.data,0,0}));sample.hd_ui_image=&uc_hd_ui::first;
    trial_graphics::enabled=true;CHECK(b.BindHdUi(sample,stages,0));held=sample.hd_ui_res.ptr;
    b.textures_[{1,0,0}]=std::move(sample);
    for(unsigned i=2;i<=2049;++i)b.textures_[{i,0,0}]=Tex{};
    b.presents_=660;b.ReleaseOldTextures();CHECK(b.textures_.empty()&&!held.expired());
    b.frame.garbage.clear();CHECK(held.expired());
    std::printf("PASS %u HD UI renderer ownership/toggle/identity/prompt checks\n",checks);
}
