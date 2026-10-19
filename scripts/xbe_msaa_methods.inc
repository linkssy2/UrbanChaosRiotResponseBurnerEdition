// Private backend implementation. The cache follows GPU image identity through
// smooth-fog swaps, not a guest address or unordered_map node that can be reused.
uint32_t D3D12Backend::DesiredSamples(const Target& t) const {
    if(msaa_override_)return msaa_override_;
    const uint32_t n=trial_graphics::MsaaSamples();
    return !ext_ && uc_msaa::Primary(t,guest_w_,guest_h_) && n<32 &&
        (msaa_support_&(1u<<n)) ? n : 1;
}

void D3D12Backend::InvalidateMsaa(Target& t) {
    auto it=msaa_reads_.find(t.res.Get());
    if(it!=msaa_reads_.end())it->second.dirty=true;
}

void D3D12Backend::ReleaseMsaa(ID3D12Resource* resource) {
    auto it=msaa_reads_.find(resource);
    if(it==msaa_reads_.end())return;
    if(it->second.depth)free_rtv_.push_back(it->second.view);
    CurrentFrame().garbage.push_back(it->second.res);
    msaa_reads_.erase(it);
}

bool D3D12Backend::CreateMatchedColor(Target& t,uint32_t samples) {
    // GetDesc can report an MSAA-specific allocation alignment. Rebuild the
    // texture description so changing sample count lets D3D12 choose it anew.
    auto desc=Texture2DDesc(t.fmt,t.w,t.h);
    desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;desc.SampleDesc.Count=samples;
    D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_CLEAR_VALUE clear{};clear.Format=t.fmt;
    ComPtr<ID3D12Resource> image;
    if(FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&image))))return false;
    t.res=std::move(image);t.state=D3D12_RESOURCE_STATE_RENDER_TARGET;
    t.samples=ext_ ? 0 : samples;
    return true;
}

// An alternate retained color image can meet a depth image already changed by
// another complete clear. Convert color to that companion's count, retaining
// its dimensions and contents; do not discard depth or apply a new setting to it.
bool D3D12Backend::MatchTargetSamples(Target& color,const Target& depth) {
    const uint32_t samples=uc_msaa::Samples(&depth);
    if(uc_msaa::Samples(&color)==samples)return true;
    if(color.depth||color.w!=depth.w||color.h!=depth.h||
       color.fmt!=DXGI_FORMAT_B8G8R8A8_UNORM||samples>=32||
       !(msaa_support_&(1u<<samples))||free_rtv_.empty())return false;
    Target replacement=color;replacement.fb.Reset();
    if(!CreateMatchedColor(replacement,samples))return false;
    if(!CreateTargetView(replacement))return false;
    if(!CopySmoothFogImage(color,replacement)) {
        // CreateTargetView already recorded a clear of this image.
        CurrentFrame().garbage.push_back(replacement.res);
        free_rtv_.push_back(replacement.view);
        return false;
    }
    ReleaseSmoothFog(color);ReleaseDepthAlias(color);ReleaseMsaa(color.res.Get());
    CurrentFrame().garbage.push_back(color.res);
    if(color.fb)CurrentFrame().garbage.push_back(color.fb);
    free_rtv_.push_back(color.view);
    color=std::move(replacement);bound_rt_=bound_ds_=nullptr;
    return true;
}

D3D12Backend::TargetRead D3D12Backend::ReadTarget(Target& t) {
    const auto format=t.depth?DXGI_FORMAT_R24_UNORM_X8_TYPELESS:t.fmt;
    if(uc_msaa::Samples(&t)==1)return {t.res.Get(),&t.state,format};
    auto found=msaa_reads_.find(t.res.Get());
    if(found==msaa_reads_.end()) {
        if(t.depth&&free_rtv_.empty())return {};
        MsaaRead read;read.depth=t.depth;
        read.format=t.depth?DXGI_FORMAT_R32_FLOAT:t.fmt;
        auto td=Texture2DDesc(read.format,t.w,t.h);
        td.Flags=t.depth?D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:D3D12_RESOURCE_FLAG_NONE;
        read.state=t.depth?D3D12_RESOURCE_STATE_RENDER_TARGET:D3D12_RESOURCE_STATE_RESOLVE_DEST;
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        if(FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read.state,nullptr,IID_PPV_ARGS(&read.res))))return {};
        if(t.depth) {
            read.view=free_rtv_.back();free_rtv_.pop_back();
            dev_->CreateRenderTargetView(read.res.Get(),nullptr,Cpu(rtv_heap_.Get(),read.view,rtv_inc_));
        }
        found=msaa_reads_.emplace(t.res.Get(),std::move(read)).first;
    }
    auto& read=found->second;
    if(read.dirty) {
        if(t.depth) {
            auto* pso=EnsurePassPipeline(uc_msaa::kDepthRead,"psmain",read.format,-1);
            if(!pso)return {};
            Transition(t.res.Get(),t.state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Transition(read.res.Get(),read.state,D3D12_RESOURCE_STATE_RENDER_TARGET);
            SetBlitTarget(Cpu(rtv_heap_.Get(),read.view,rtv_inc_),t.w,t.h);
            uint32_t slot=AllocSrvSlots(1);
            auto sv=uc_msaa::DepthView(false);
            dev_->CreateShaderResourceView(t.res.Get(),&sv,Cpu(srv_heap_.Get(),slot,srv_inc_));
            Alloc cb=Upload(256,256);DrawFullScreen(pso,cb.gpu,slot);
            bound_rt_=bound_ds_=nullptr;
        } else {
            Transition(t.res.Get(),t.state,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            Transition(read.res.Get(),read.state,D3D12_RESOURCE_STATE_RESOLVE_DEST);
            cl_->ResolveSubresource(read.res.Get(),0,t.res.Get(),0,t.fmt);
        }
        read.dirty=false;
    }
    return {read.res.Get(),&read.state,read.format};
}
