/* Serialize a guest's direct voice-list edit with the standalone VP worker.
 * The caller's region must not wait on audio or invoke guest callbacks. */
static void apu_guest_list_begin(MCPXAPUState *d)
{
    if(!d || !d->host_registers)return;
    InterlockedIncrement(&d->mmio_waiters);
    qemu_mutex_lock(&d->lock);
    InterlockedDecrement(&d->mmio_waiters);
    const unsigned heads[]={NV_PAPU_TVL2D,NV_PAPU_TVL3D,NV_PAPU_TVLMP};
    const unsigned cursors[]={NV_PAPU_CVL2D,NV_PAPU_NVL2D,NV_PAPU_CVL3D,
        NV_PAPU_NVL3D,NV_PAPU_CVLMP,NV_PAPU_NVLMP};
    for(unsigned i=0;i<3;++i)d->regs[heads[i]]=d->host_registers[heads[i]/4];
    for(unsigned i=0;i<6;++i)d->host_registers[cursors[i]/4]=d->regs[cursors[i]];
}
static void apu_guest_list_end(MCPXAPUState *d)
{
    if(!d || !d->host_registers)return;
    const unsigned registers[]={NV_PAPU_TVL2D,NV_PAPU_CVL2D,NV_PAPU_NVL2D,
        NV_PAPU_TVL3D,NV_PAPU_CVL3D,NV_PAPU_NVL3D,
        NV_PAPU_TVLMP,NV_PAPU_CVLMP,NV_PAPU_NVLMP};
    for(unsigned i=0;i<9;++i)d->regs[registers[i]]=d->host_registers[registers[i]/4];
    qemu_cond_signal(&d->cond);
    qemu_mutex_unlock(&d->lock);
}
