// B4CA0 compiles these reusable index streams with XGCompileDrawIndexedVertices.
// The render queue copies the indices and tracks bound resources before Draw returns.
#define SYM_UrbanChaos_RunPushBuffer 003EF870
HLE_STDCALL(UrbanChaos_RunPushBuffer, 2) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    uint32_t resource = a[0];
    if (a[1] || !TrialGuestReadable(resource,24) || (R32(resource)&0x70000u)!=0x20000u)
        x86_trap(ctx,0x3ef870,"unsupported recorded draw descriptor/fixup");
    uint32_t data=R32(resource+4),bytes=R32(resource+12);
    if(bytes>16u*1024u*1024u || !TrialGuestReadable(data,bytes))
        x86_trap(ctx,0x3ef870,"unreadable recorded draw data");
    size_t count=0;uint32_t primitive=0;
    if(!uc_control_model_indexed(HostPtr(data),bytes,65536,nullptr,0,&count,&primitive)) {
        LOGE("UC recorded draw unsupported: resource=%08X data=%08X bytes=%u head=%08X",resource,data,bytes,bytes>=4?R32(data):0);
        x86_trap(ctx,0x3ef870,"recorded draw differs from the verified indexed stream");
    }
    static std::vector<uint32_t> decoded;
    static std::vector<uint16_t> indices;
    decoded.resize(count);indices.resize(count);
    if(!uc_control_model_indexed(HostPtr(data),bytes,65536,decoded.data(),decoded.size(),&count,&primitive))
        x86_trap(ctx,0x3ef870,"recorded draw changed during decoding");
    for(size_t i=0;i<count;++i)indices[i]=uint16_t(decoded[i]);
    Walk(Dev(kDevPush));
    SyncDeferred();
    if(g_backend)g_backend->Draw(g_state,primitive,0,uint32_t(count),reinterpret_cast<const uint8_t*>(indices.data()));
    static unsigned traces=0;
    if(traces++<4)LOGR("UC recorded draw: resource=%08X topology=%u indices=%u vs=%08X stream0=%08X",resource,primitive,unsigned(count),g_state.vs_handle,g_state.streams[0].vb);
    return 0;
}
