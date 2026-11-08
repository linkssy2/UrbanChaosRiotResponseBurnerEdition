#ifndef UC_TARGET_LOOKUP_SCENARIOS
constexpr unsigned kTex2D=1,kTexWide=4,kRsZEnable=0,kRsStencilEnable=1;
constexpr float kUpscaledMin=1.01f;
constexpr int kKeyWidthShift=34,kKeyHeightShift=48;
constexpr unsigned D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING=5768,D3D12_SRV_DIMENSION_TEXTURE2D=4;
constexpr unsigned DXGI_FORMAT_B8G8R8A8_UNORM=87,DXGI_FORMAT_R24_UNORM_X8_TYPELESS=46;
constexpr unsigned D3D12_RESOURCE_STATE_RENDER_TARGET=4,D3D12_RESOURCE_STATE_DEPTH_WRITE=16,
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE=128;
struct ID3D12Resource { unsigned id=0; };
struct Resource { std::shared_ptr<ID3D12Resource> value=std::make_shared<ID3D12Resource>();ID3D12Resource* Get(){return value.get();} explicit operator bool()const{return value->id!=0;} };
struct D3D12_CPU_DESCRIPTOR_HANDLE { unsigned ptr=0; };
struct Heap { unsigned id=0; Heap* Get(){return this;} };
static D3D12_CPU_DESCRIPTOR_HANDLE Cpu(Heap*,unsigned view,unsigned){return {view};}
struct Surface { uint32_t header=0,data=0,format=0,size=0;float present_x=1,present_y=1; };
struct State { Surface rt,zs;std::array<unsigned,2> rs{};std::array<unsigned,4> textures{}; };
struct TextureDesc { unsigned data=0,format=0,width=0,height=0,depth=1;bool cube=false,linear=false; };
struct FormatInfo { bool depth=false,compressed=false; };
struct TexInfo { unsigned kind=0,rect=0,flags=0,shadow=0; };
struct D3D12_SHADER_RESOURCE_VIEW_DESC {
    unsigned Shader4ComponentMapping=0,ViewDimension=0,Format=0;
    struct { unsigned MipLevels=0; } Texture2D;
};
struct Target { Resource res,fb;bool depth=false;unsigned gw=0,gh=0,w=0,h=0,view=0,data=0,bytes=0,state=0,fmt=87,samples=1;uint64_t epoch=0,sig=0;bool drawn=false; };
namespace uc_msaa { static unsigned Samples(Target* t){return t->samples;} }
struct DrawTextures {
    std::array<TexInfo,4> info{};std::array<D3D12_SHADER_RESOURCE_VIEW_DESC,4> sv{};
    std::array<ID3D12Resource*,4> res{};unsigned width[4]{},height[4]{};
    bool rt[4]{};float k[4]{1,1,1,1},wide[4]{};
};
static std::map<unsigned,TextureDesc> headers;
static std::set<unsigned> invalid;
static std::function<void(unsigned)> on_describe;
static bool Describe(unsigned header,TextureDesc& d) {
    if(on_describe)on_describe(header);
    auto it=headers.find(header);if(it==headers.end()||invalid.count(header))return false;d=it->second;return true;
}
static const FormatInfo& Format(unsigned f){static FormatInfo regular{},depth{true,false},compressed{false,true};return f==99?depth:f==88?compressed:regular;}
static unsigned TextureKind(const TextureDesc& d){return d.cube?2:d.depth>1?3:kTex2D;}
static unsigned ShadowDepthKind(unsigned f){return 100+f;}
namespace mem { constexpr uint32_t kContiguousBase=0x80000000,kPhysicalSize=64u<<20;uint32_t VirtualFromPhysical(uint32_t); }
static unsigned GuestPitch(unsigned width){return width*4;}
static uint64_t DirtyEpoch(unsigned d){return d+7;}
static uint64_t MemSig(unsigned d,unsigned size){return uint64_t(d)*size;}
static void SurfaceSize(const Surface&,uint32_t&,uint32_t&);
static uint64_t TargetMapKey(uint32_t,bool);
struct Extension {
    bool reject=false;uint64_t alt=0;float scale=1;bool box=true;
    std::function<void()> target_callback,sample_callback,sampled_callback;
    unsigned keys=0,samples=0,sampled=0;
    bool TargetKey(const Surface&,bool,uint64_t& out){++keys;if(target_callback)target_callback();out=alt;return !reject;}
    void Sample(unsigned,bool,int,uint64_t& out,float& k,bool& b){++samples;if(sample_callback)sample_callback();out=alt;k=scale;b=box;}
    void Sampled(const TextureDesc&,Target*,unsigned,TexInfo& i){++sampled;if(sampled_callback)sampled_callback();i.flags|=128;}
};
struct TrackedTargets:std::unordered_map<uint64_t,Target> {
    uint64_t finds=0;
    iterator find(uint64_t k){++finds;return std::unordered_map<uint64_t,Target>::find(k);}
};
struct D3D12Backend {
    struct TargetSlot { unsigned data;uint64_t key;unsigned gw,gh;std::unordered_map<uint64_t,Target>::iterator it;bool sized; };
    struct Read { ID3D12Resource* res;unsigned* state;unsigned format; };
    Extension* ext_=nullptr;TrackedTargets targets_;std::unordered_map<unsigned,uint64_t> target_used_;
    Target *bound_rt_=nullptr,*bound_ds_=nullptr,*draw_rt_=nullptr,*draw_ds_=nullptr;
    unsigned guest_w_=640,guest_h_=480,frame_=11,msaa_override_=1,next=100;
    float scale_=1;bool fail_create=false,fail_read=false,cpu_write=false,fail_fog=false;
    bool ledger=true;std::vector<unsigned> effects;std::vector<Resource> retired;
    std::vector<std::unordered_map<uint64_t,Target>::node_type> retired_nodes;
    Heap rtv_heap_,dsv_heap_;unsigned rtv_inc_=1,dsv_inc_=1;
    struct List {
        D3D12Backend* owner=nullptr;List* operator->(){return this;}
        void OMSetRenderTargets(unsigned n,const D3D12_CPU_DESCRIPTOR_HANDLE* r,int,const D3D12_CPU_DESCRIPTOR_HANDLE* d){owner->Effect(80,n,r?r->ptr:0,d?d->ptr:0);}
    } cl_{this};
    static constexpr int FALSE=0;
    ID3D12Resource guest{9000},alias{9001},feedback{9002};unsigned read_state=0;
    std::function<void()> guest_hook;
    void Effect(unsigned a,unsigned b=0,unsigned c=0,unsigned d=0){if(ledger){effects.push_back(a);effects.push_back(b);effects.push_back(c);effects.push_back(d);}}
    bool CreateTargetResource(Target& t,const Surface&){if(fail_create)return false;t.res.value->id=next++;t.w=unsigned(t.gw*scale_);t.h=unsigned(t.gh*scale_);t.samples=msaa_override_;return true;}
    bool CreateTargetView(Target& t){t.view=t.res.value->id;return true;}
    // Keep test tombstones alive to prevent allocator address reuse changing the
    // pre-existing feedback pointer comparison between the two isolated models.
    void ReleaseTarget(std::unordered_map<uint64_t,Target>::iterator it){if(bound_rt_==&it->second||bound_ds_==&it->second)bound_rt_=bound_ds_=nullptr;retired.push_back(it->second.res);retired_nodes.push_back(targets_.extract(it));}
    bool EndSmoothFog(Target& t){Effect(20,t.data);return !fail_fog;}
    bool MatchTargetSamples(Target& a,Target& b){Effect(21,a.data,b.data);return a.samples==b.samples;}
    bool LatchCpuWrite(Target& t){Effect(30,t.data);return cpu_write;}
    void UploadTarget(Target& t,const Surface&){Effect(31,t.data);bound_rt_=bound_ds_=nullptr;}
    void Transition(ID3D12Resource* r,unsigned& state,unsigned wanted){Effect(40,r?r->id:0,state,wanted);state=wanted;}
    Read ReadTarget(Target& t){Effect(50,t.data);if(fail_read)return {};if(t.samples>1){Effect(51,t.data);if(t.depth)bound_rt_=bound_ds_=nullptr;return {t.res.Get(),&read_state,t.fmt};}return {t.res.Get(),&t.state,t.fmt};}
    ID3D12Resource* Feedback(Target& t){Effect(60,t.data);auto r=ReadTarget(t);if(!r.res)return nullptr;Transition(r.res,*r.state,256);Effect(61,t.data);Transition(&feedback,read_state,128);return &feedback;}
    bool IsDepthColorAlias(const TextureDesc&,Target&){Effect(65);return true;}
    ID3D12Resource* BindDepthColorAlias(Target& t,int stage,DrawTextures& d){Effect(66,t.data);bound_rt_=bound_ds_=nullptr;d.rt[stage]=true;d.info[stage].kind=kTex2D;d.sv[stage].Format=87;return &alias;}
    ID3D12Resource* BindGuestTexture(const State&,int stage,const TextureDesc& d,const FormatInfo&,DrawTextures& out){Effect(70,d.data);if(guest_hook)guest_hook();out.info[stage].flags=64;return &guest;}
    uint64_t SizedTargetKey(uint32_t,bool,uint32_t,uint32_t)const;
    TargetSlot FindTargetSlot(const Surface&,bool,uint64_t);
    Target* FindTarget(const Surface&,bool);
    Target* EnsureTarget(const Surface&,bool);
    Target* EnsurePairedTarget(const Surface&,bool,const Surface&);
    bool BindTargets(const State&);
    ID3D12Resource* BindTexture(const State&,int,DrawTextures&);
    ID3D12Resource* BindTargetTexture(const State&,int,const TextureDesc&,Target&,bool,DrawTextures&);
    bool BindStageTexturesAndTargets(const State&,DrawTextures&);
};

#else
static Target Add(D3D12Backend& b,unsigned raw,unsigned w,unsigned h,bool depth=false,unsigned samples=1) {
    Target t;t.data=mem::VirtualFromPhysical(raw);t.gw=w;t.gh=h;t.w=w*2;t.h=h*2;
    t.depth=depth;t.samples=samples;t.res.value->id=b.next++;t.view=t.res.value->id;
    b.targets_[b.SizedTargetKey(t.data,depth,w,h)]=t;return t;
}
static Result Run(unsigned kind,unsigned extent,unsigned variant,bool extension,unsigned event) {
    headers.clear();invalid.clear();on_describe={};
    D3D12Backend b;State s;Extension ext;b.ext_=extension?&ext:nullptr;
    ext.scale=(variant&1)?1.25f:1;ext.box=(variant&2)==0;ext.reject=variant==15;
    unsigned w=extent?320:640,h=extent?240:480;
    headers[1]={0x80001000,0,w,h};headers[2]={0x80002000,99,w,h};
    s.rt={1,0x1000};s.zs={2,0x2000};s.rs[0]=1;
    if(event==1)s.rt.header=0;
    if(event==2)b.fail_create=true;
    if(event==3){Add(b,0x1000,w,h);auto it=b.targets_.find(b.SizedTargetKey(0x80001000,false,w,h));b.ReleaseTarget(it);}
    if(event==4){Add(b,0x1000,w+1,h+1);}
    b.scale_=variant&4?2.f:1.f;b.msaa_override_=variant&8?4:1;
    if(!b.BindTargets(s)){Result r;r.success=false;on_describe={};return r;}
    b.cpu_write=(variant&1)!=0;b.fail_read=(variant&2)!=0;
    for(unsigned i=0;i<4;++i){
        unsigned which=(kind+i)%12,header=10+i,raw=0x3000+i*0x1000;
        TextureDesc d{mem::VirtualFromPhysical(raw),0,w*2,h*2};
        s.textures[i]=header;
        switch(which){
        case 0:s.textures[i]=0;break;
        case 1:invalid.insert(header);break;
        case 3:Add(b,raw,d.width,d.height,false,b.msaa_override_);break;
        case 4:d=headers[1];break;
        case 5:d=headers[2];break;
        case 6:d.format=99;Add(b,raw,d.width,d.height,true,b.msaa_override_);break;
        case 7:d.format=0x40;Add(b,raw,d.width,d.height,true,b.msaa_override_);break;
        case 8:d.format=88;Add(b,raw,d.width,d.height);break;
        case 9:d.cube=true;Add(b,raw,d.width,d.height);break;
        case 10:d.depth=2;Add(b,raw,d.width,d.height);break;
        default:break;
        }
        headers[header]=d;
    }
    bool did_event=false;
    on_describe=[&](unsigned header){
        if(header<10||did_event)return;did_event=true;
        // Trigger after draw_rt_ has been captured, before first stage resolution.
        if(event==5)headers[1].width+=17;
        if(event==6)invalid.insert(1);
        if(event==7){auto* saved=b.draw_rt_;auto old=b.targets_.bucket_count();
            b.targets_.rehash(old*20+100);for(unsigned i=0;i<200;++i)Add(b,0x100000+i*0x1000,32,32);
            if(saved)CHECK(saved==&b.targets_.at(b.SizedTargetKey(saved->data,false,saved->gw,saved->gh)));}
        if(event==8 && b.draw_rt_){b.draw_rt_->res.value->id+=1000;b.draw_rt_->w+=64;b.draw_rt_->h+=64;}
    };
    if(extension && event==9){ext.sample_callback=[&]{headers[1].width=w+19;};}
    if(extension && event==10){ext.sample_callback=[&]{auto key=b.SizedTargetKey(0x80001000,false,w,h);
        auto it=b.targets_.find(key);if(it!=b.targets_.end())b.ReleaseTarget(it);};}
    // Original extension semantics query the new entry, not stale draw_rt_.
    if(extension && event==11){ext.sample_callback=[&]{auto key=b.SizedTargetKey(0x80001000,false,w,h);
        auto it=b.targets_.find(key);if(it!=b.targets_.end())b.ReleaseTarget(it);Add(b,0x1000,w,h);};}
    b.effects.clear();b.targets_.finds=0;
    DrawTextures d{};Result r;r.success=b.BindStageTexturesAndTargets(s,d);
    on_describe={};
    for(unsigned i=0;i<4;++i){
        auto& v=d.sv[i];auto& n=d.info[i];
        r.output.insert(r.output.end(),{d.res[i]?d.res[i]->id:0,d.width[i],d.height[i],unsigned(d.rt[i]),n.kind,n.rect,n.flags,n.shadow,
            v.Shader4ComponentMapping,v.ViewDimension,v.Format,v.Texture2D.MipLevels,
            std::bit_cast<unsigned>(d.k[i]),std::bit_cast<unsigned>(d.wide[i])});
    }
    r.effects=b.effects;r.finds=b.targets_.finds;
    r.callback_counts={ext.keys,ext.samples,ext.sampled};
    for(auto& [key,t]:b.targets_)r.targets[key]={t.data,t.gw,t.gh,t.w,t.h,t.res.value->id,t.state,t.samples,unsigned(t.drawn)};
    for(auto& [key,value]:b.target_used_)r.used[key]=value;
    return r;
}

#endif
