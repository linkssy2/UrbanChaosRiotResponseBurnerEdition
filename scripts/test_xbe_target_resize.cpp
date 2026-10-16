// Production target-replacement method against a resource/view lifetime ledger.
// This is not a GPU pixel test: unchanged identities prove no content discard.
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>
#include "xbe_target_resize.h"
namespace trial_graphics { static bool pending=true;void SetScalePending(bool b){pending=b;} }
struct Resource { unsigned id=0;void Reset(){id=0;} explicit operator bool()const{return id!=0;} Resource* Get(){return this;} };
struct Target {
    bool depth=false;unsigned gw=640,gh=480,w=640,h=480,view=0,fmt=1,data=0,bytes=1228800,state=0;
    uint64_t epoch=11,sig=22;bool drawn=true;std::vector<unsigned> read{1,2,3};
    float range=1;uint64_t tag=55;Resource res,fb;
};
struct Surface { Target* target=nullptr;bool* exists=nullptr;unsigned gw=640,gh=480; };
static void SurfaceSize(const Surface& s,unsigned& w,unsigned& h){w=s.gw;h=s.gh;}
struct Viewport { float x=0,y=0,w=640,h=480; };
struct State { Viewport viewport;Surface rt,zs; };
struct ClearArgs { unsigned flags=0,count=0;const uint8_t* rects=nullptr; };
struct Extension { void MakeTarget(const Surface&,bool,unsigned,unsigned,float&,unsigned&){} };
struct Frame { std::vector<Resource> garbage; };
struct CopyList {};
class D3D12Backend {
public:
    float scale_=1;unsigned guest_w_=640,guest_h_=480;
    Extension* ext_=nullptr;std::vector<unsigned> free_rtv_{10},free_dsv_{20};Frame frame;
    Target *bound_rt_=nullptr,*bound_ds_=nullptr;
    unsigned aliases_released=0,smooth_released=0,allocations=0,fail_allocation=0;
    unsigned copies_drawn=0,copies_region=0;
    bool CreateTargetResource(Target& t,const Surface&) {
        if(++allocations==fail_allocation)return false;
        t.w=std::max(1u,unsigned(std::lround(t.gw*scale_)));
        t.h=std::max(1u,unsigned(std::lround(t.gh*scale_)));t.res.id=100+allocations;return true;
    }
    bool CreateTargetView(Target& t) {
        auto& p=t.depth?free_dsv_:free_rtv_;if(p.empty())return false;
        t.view=p.back();p.pop_back();return true;
    }
    Frame& CurrentFrame(){return frame;}
    void ReleaseDepthAlias(Target&){++aliases_released;}
    void ReleaseSmoothFog(Target&){++smooth_released;}
    Target* FindTarget(const Surface& s,bool){return s.exists&&*s.exists?s.target:nullptr;}
    Target* EnsureTarget(const Surface& s,bool depth){
        if(auto* t=FindTarget(s,depth))return t;
        if(!s.target||!s.exists)return nullptr;
        s.target->depth=depth;
        if(!CreateTargetResource(*s.target,s)||!CreateTargetView(*s.target))return nullptr;
        *s.exists=true;return s.target;
    }
    Target* EnsurePairedTarget(const Surface&,bool,const Surface&);
    void ResizeClearedTargets(const State&,const ClearArgs&,Target*,Target*);
    void CopyByDraw(const Target&,Resource*,unsigned&,Target&,const CopyList&){++copies_drawn;}
    void CopyByRegion(const Target&,Resource*,unsigned&,Target&,const CopyList&){++copies_region;}
    void CopyRoute(Target* src_t,Target* dst_t,const CopyList& l){
// GENERATED_COPY_ROUTE
    }
};
// GENERATED_RESIZE
static Target Color(){Target t;t.res.id=1;t.fb.id=3;t.view=4;t.data=0x10000;return t;}
static Target Depth(){auto t=Color();t.depth=true;t.res.id=2;t.fb.id=0;t.view=5;t.data=0x20000;return t;}
int main(){
    State s;ClearArgs all{0xf3};D3D12Backend b;auto c=Color(),d=Depth();
    b.scale_=2;b.bound_rt_=&c;b.bound_ds_=&d;
    b.ResizeClearedTargets(s,all,&c,&d);
    assert(c.w==1280&&d.w==1280&&c.h==960&&d.h==960);
    assert(c.data==0x10000&&d.data==0x20000&&c.epoch==11&&c.sig==22&&c.read.size()==3);
    assert(c.tag==55&&c.range==1&&!c.fb&&b.frame.garbage.size()==3&&b.aliases_released==2);
    assert(!b.bound_rt_&&!b.bound_ds_&&!trial_graphics::pending);
    b.scale_=0.5f;b.ResizeClearedTargets(s,all,&c,&d);
    assert(c.w==320&&d.w==320&&c.h==240&&d.h==240);
    // A second Apply that returns to the retained size is no longer pending.
    trial_graphics::pending=true;b.ResizeClearedTargets(s,all,&c,&d);
    assert(!trial_graphics::pending&&b.allocations==4);
    // Cached color and depth/stencil stay intact under partial or missing masks.
    for(unsigned flags:{0xf0u,1u,2u,3u,0x70u,0xf1u,0xf2u}){
        D3D12Backend x;auto a=Color(),z=Depth();x.scale_=2;
        x.ResizeClearedTargets(s,{flags},&a,&z);
        assert(a.res.id==1&&z.res.id==2&&x.allocations==0&&x.frame.garbage.empty());
    }
    // Standalone color and depth targets may resize under their own complete masks.
    {D3D12Backend x;auto a=Color();x.scale_=2;x.ResizeClearedTargets(s,{0xf0},&a,nullptr);assert(a.w==1280);}
    {D3D12Backend x;auto z=Depth();x.scale_=2;x.ResizeClearedTargets(s,{3},nullptr,&z);assert(z.w==1280);}
    for(bool explicit_rect:{false,true}){
        D3D12Backend x;auto a=Color(),z=Depth();x.scale_=2;State partial=s;
        int32_t rect[4]={1,0,640,480};ClearArgs clear=all;
        if(explicit_rect){clear.count=1;clear.rects=reinterpret_cast<uint8_t*>(rect);}
        else partial.viewport.w=639;
        x.ResizeClearedTargets(partial,clear,&a,&z);
        assert(a.res.id==1&&z.res.id==2&&x.allocations==0);
    }
    {D3D12Backend x;auto a=Color(),z=Depth();x.scale_=2;int32_t rect[4]={0,0,640,480};
     x.ResizeClearedTargets(s,{0xf3,1,reinterpret_cast<uint8_t*>(rect)},&a,&z);assert(a.w==z.w&&a.w==1280);}
    // Failed second allocation cannot partially replace the pair or lose content.
    {D3D12Backend x;auto a=Color(),z=Depth();x.scale_=2;x.fail_allocation=2;
     x.ResizeClearedTargets(s,all,&a,&z);assert(a.res.id==1&&z.res.id==2&&x.frame.garbage.empty());}
    {D3D12Backend x;auto a=Color(),z=Depth();x.scale_=2;x.free_dsv_.clear();
     x.ResizeClearedTargets(s,all,&a,&z);assert(a.res.id==1&&z.res.id==2&&x.allocations==0);}
    // New attachments inherit a retained companion's size, while the requested
    // scale remains pending for the next complete paired overwrite.
    for(bool new_depth:{false,true}){
        D3D12Backend x;auto a=Color(),z=Depth();bool ca=new_depth,za=!new_depth;
        State pair;pair.rt={&a,&ca};pair.zs={&z,&za};x.scale_=new_depth?.5f:2.f;
        if(new_depth){a.w=1280;a.h=960;}
        auto* made=new_depth?x.EnsurePairedTarget(pair.zs,true,pair.rt):x.EnsurePairedTarget(pair.rt,false,pair.zs);
        assert(made&&a.w==z.w&&a.h==z.h&&x.scale_==(new_depth?.5f:2.f));
        // Supply another view for the subsequent replacement, as real pools do.
        (new_depth?x.free_dsv_:x.free_rtv_).push_back(30);
        auto ar=a.res.id,zr=z.res.id;
        x.ResizeClearedTargets(pair,{new_depth?3u:0xf0u},&a,&z);
        assert(a.res.id==ar&&z.res.id==zr&&a.w==z.w);
        x.ResizeClearedTargets(pair,all,&a,&z);
        assert(a.w==z.w&&a.w==(new_depth?320u:1280u));
    }
    {D3D12Backend x;auto from=Color(),to=Color();CopyList copy;
     x.CopyRoute(&from,&to,copy);assert(x.copies_region==1&&x.copies_drawn==0);
     to.w=1280;to.h=960;x.CopyRoute(&from,&to,copy);
     x.CopyRoute(&to,&from,copy);assert(x.copies_drawn==2);
     to.w=640;to.h=481;x.CopyRoute(&from,&to,copy);assert(x.copies_drawn==3);
     auto geometry=uc_target_resize::DrawScale(from,to);
     assert(geometry.x==1&&geometry.y>1&&geometry.taps==1);
     geometry=uc_target_resize::DrawScale(to,from);
     assert(geometry.x==1&&geometry.y==1&&geometry.taps==2);
     // Differently sized guest surfaces at an equal scale retain the fast copy.
     to.gw=320;to.gh=240;to.w=320;to.h=240;x.CopyRoute(&from,&to,copy);
     assert(x.copies_region==2);
     to.fmt=2;x.CopyRoute(&from,&to,copy);assert(x.copies_drawn==4);}
    std::puts("target resize: actual replacement method, full paired clears, preservation, scale up/down and allocation rollback passed");
}
