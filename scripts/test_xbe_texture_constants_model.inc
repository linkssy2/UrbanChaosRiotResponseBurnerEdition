// Minimal caller model: production BindTexture and constant functions are extracted verbatim.
struct TextureDesc { uint32_t data=0,format=0,width=0,height=0,depth=1;bool cube=false,linear=false; };
struct FormatInfo { bool depth=false,compressed=false; };
struct ID3D12Resource { int id; };
struct View { uint32_t Shader4ComponentMapping,ViewDimension,Format;struct {uint32_t MipLevels;} Texture2D; };
using D3D12_SHADER_RESOURCE_VIEW_DESC=View;
constexpr uint32_t D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING=5768,D3D12_SRV_DIMENSION_TEXTURE2D=4,DXGI_FORMAT_B8G8R8A8_UNORM=87;
enum { kTssBumpEnvMat00,kTssBumpEnvMat01,kTssBumpEnvMat10,kTssBumpEnvMat11,kTssBumpEnvLScale,kTssBumpEnvLOffset };
struct State { std::array<uint32_t,4> textures{};std::array<std::array<uint32_t,6>,4> tss{}; };
struct TexInfo { unsigned kind=0;bool rect=false; };
struct DrawTextures { TexInfo info[4];View sv[4];ID3D12Resource* res[4];uint32_t width[4],height[4];bool rt[4]{},aligned_depth_alias[4]{};float k[4]{1,1,1,1},wide[4]{}; };
struct DrawConstants { float texsize[4][4]{},spread[4][4]{},bump[4][4]{},bumpl[4][2]{}; };
struct Target { unsigned variant=0; };
static std::vector<unsigned> effects;
struct Extension {
    uint64_t alt=0;
    void Sample(uint32_t,bool,int,uint64_t& out,float& scale,bool& box){effects.push_back(1);out=alt;scale=1.25f;box=false;}
};
struct Targets {
    std::unordered_map<uint64_t,Target> values;unsigned finds=0;
    auto find(uint64_t key){++finds;return values.find(key);}
    auto end(){return values.end();}
    Target& operator[](uint64_t key){return values[key];}
};
static TextureDesc headers[5];static bool valid[5];static unsigned reads;
bool Describe(uint32_t header,TextureDesc& d) { ++reads; if(!header || !valid[header])return false;d=headers[header];return true; }
const FormatInfo& Format(uint32_t format) { static FormatInfo regular{},depth{true,false},compressed{false,true};return format==99?depth:format==88?compressed:regular; }
unsigned TextureKind(const TextureDesc& d) {return d.cube?2:d.depth>1?3:1;}
uint64_t SizedTargetKey(uint32_t data,bool depth,uint32_t width,uint32_t height) {return (uint64_t(data)<<32)^uint64_t(width<<16)^height^(depth?0x8000000000000000ull:0);}
float Bits(uint32_t u) {float f;std::memcpy(&f,&u,4);return f;}
constexpr float kGlowBoxTexels=3.0f;
struct D3D12Backend {
    Extension* ext_=nullptr;Targets targets_;bool fail=false,prompt=false;
    ID3D12Resource ordinary{1},target{2},alias{3},replacement{4};
    bool IsDepthColorAlias(const TextureDesc&,const Target&){effects.push_back(2);return true;}
    ID3D12Resource* BindGuestTexture(const State&,int,const TextureDesc&,const FormatInfo&,DrawTextures&){effects.push_back(3);return fail?nullptr:prompt?&replacement:&ordinary;}
    ID3D12Resource* BindTargetTexture(const State&,int i,const TextureDesc&,Target&,bool box,DrawTextures& d){effects.push_back(box?4:5);d.rt[i]=true;d.k[i]=2;d.wide[i]=1.5f;return fail?nullptr:&target;}
    ID3D12Resource* BindDepthColorAlias(Target&,int i,DrawTextures& d){effects.push_back(6);d.rt[i]=true;d.aligned_depth_alias[i]=true;return fail?nullptr:&alias;}
    ID3D12Resource* BindTexture(const State&,int,DrawTextures&);
    void FillTextureConstants(const State&,const DrawTextures&,DrawConstants&);
#if NEW_DIMENSIONS
    void FillTexelConstants(int,uint32_t,uint32_t,const DrawTextures&,DrawConstants&);
#else
    void FillTexelConstants(int,const TextureDesc&,const DrawTextures&,DrawConstants&);
#endif
};

Result Run(unsigned kind,unsigned extent,bool extension,unsigned mutation) {
    State state;D3D12Backend backend;Extension ext;backend.ext_=extension?&ext:nullptr;
    reads=0;effects.clear();DrawTextures stages;std::memset(&stages,0xa5,sizeof(stages));
    for(unsigned i=0;i<4;++i) {
        stages.aligned_depth_alias[i]=false;
        state.textures[i]=kind==0?0:i+1;
        headers[i+1]={0x1000u*(i+1),kind==4?0x40u:0u,1u+extent*37+i,1u+extent*19+i,1,false,(i&1)!=0};
        valid[i+1]=kind!=1;
        for(unsigned j=0;j<6;++j)state.tss[i][j]=0x3f000000u+extent*0x10000u+i*0x100u+j;
        if(kind==3 || kind==4)backend.targets_[SizedTargetKey(headers[i+1].data,kind==4,headers[i+1].width,headers[i+1].height)]={};
    }
    backend.fail=kind==5;backend.prompt=kind==6;
    for(unsigned i=0;i<4;++i)stages.res[i]=backend.BindTexture(state,i,stages);
    // Model fog lookup/prompt replacement after binding: authored dimensions still drive constants.
    if(kind==7)for(unsigned i=0;i<4;++i){stages.res[i]=&backend.replacement;stages.rt[i]=true;stages.k[i]=1.25f;stages.wide[i]=0;}
    if(kind==8)for(unsigned i=0;i<4;++i){stages.res[i]=nullptr;stages.rt[i]=true;stages.k[i]=1.5f;stages.wide[i]=2;}
    if(mutation)for(unsigned i=1;i<=4;++i){headers[i].width+=23;headers[i].height+=17;if(mutation==2)valid[i]=false;}
    DrawConstants c{};backend.FillTextureConstants(state,stages,c);
    for(unsigned i=0;i<4;++i){
        const bool has=state.textures[i] && valid[i+1];
        assert(c.texsize[i][0]==(has?float(headers[i+1].width):0.0f));
        assert(c.texsize[i][1]==(has?float(headers[i+1].height):0.0f));
        if(!has || !stages.rt[i] || stages.aligned_depth_alias[i])assert(c.texsize[i][2]==0 && c.texsize[i][3]==0);
        else {assert(c.texsize[i][2]==-0.5f/(float(headers[i+1].width)*stages.k[i]));assert(c.texsize[i][3]==-0.5f/(float(headers[i+1].height)*stages.k[i]));}
        for(unsigned j=0;j<4;++j)assert(c.bump[i][j]==Bits(state.tss[i][j]));
    }
    Result result;static_assert(sizeof(c)==sizeof(result.constants));std::memcpy(result.constants.data(),&c,sizeof(c));result.reads=reads;
    for(unsigned i=0;i<4;++i)result.resources[i]=stages.res[i]?stages.res[i]->id:0;
    return result;
}

LookupResult RunLookup(unsigned format,bool cube,unsigned depth,unsigned target_mask,
                       bool extension,bool fail,unsigned presence,bool alternate) {
    State state;D3D12Backend backend;Extension ext;
    ext.alt=alternate?0x100u:0;backend.ext_=extension?&ext:nullptr;backend.fail=fail;
    state.textures[0]=presence?1:0;valid[1]=presence==2;
    headers[1]={0x1000,format,64,32,depth,cube,false};
    auto regular=SizedTargetKey(0x1000,Format(format).depth,64,32)|(extension?ext.alt:0);
    if(target_mask&1)backend.targets_[regular]={};
    if(target_mask&2)backend.targets_[SizedTargetKey(0x1000,true,64,32)]={};
    reads=0;effects.clear();DrawTextures stages{};
    auto* resource=backend.BindTexture(state,0,stages);
    LookupResult result;
    const auto& sv=stages.sv[0];
    result.output={uint32_t(resource?resource->id:0),stages.info[0].kind,stages.info[0].rect,
        sv.Shader4ComponentMapping,sv.ViewDimension,sv.Format,sv.Texture2D.MipLevels,
        stages.width[0],stages.height[0],stages.rt[0],stages.aligned_depth_alias[0]};
    std::memcpy(&result.output[11],&stages.k[0],sizeof(float));
    std::memcpy(&result.output[12],&stages.wide[0],sizeof(float));
    result.reads=reads;result.finds=backend.targets_.finds;result.effects=effects;
    return result;
}
