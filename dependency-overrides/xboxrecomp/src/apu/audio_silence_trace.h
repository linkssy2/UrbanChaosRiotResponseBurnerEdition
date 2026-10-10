/* Audio owner only. Failure-triggered probe, independent of startup budgets.
 * Raw snapshots, not asset identities; no guest state or routing changes. */
static void trace_silent_output(MCPXAPUState *d,
    float bins[NUM_MIXBINS][NUM_SAMPLES_PER_FRAME], bool nonzero)
{
    if(!apu_verbose_trace() || !d->host_registers)return;
    if(nonzero){d->diagnostic_zero_quanta=0;return;}
    if(d->diagnostic_zero_reports>=64)return;
    unsigned age=++d->diagnostic_zero_quanta;
    if(age!=150 && (age<1500 || age%1500))return;
    ++d->diagnostic_zero_reports;
    fprintf(stderr,"[APU-SILENT-BUSES] ms=%lld frame=%u zero_quanta=%u report=%u/64 spatial=%d monitor=%d peaks=",
        (long long)qemu_clock_get_ms(QEMU_CLOCK_REALTIME),d->ep_frame_div,age,
        d->diagnostic_zero_reports,use_spatial_stereo(d),(int)d->monitor.point);
    unsigned bad=0;
    for(unsigned b=0;b<NUM_MIXBINS;++b){
        float peak=0;
        for(unsigned i=0;i<NUM_SAMPLES_PER_FRAME;++i){
            float x=bins[b][i];if(!isfinite(x))++bad;else peak=fmaxf(peak,fabsf(x));
        }
        fprintf(stderr,"%s%u:%.7g",b?",":"",b,peak);
    }
    fprintf(stderr," nonfinite=%u heads=%08X:%08X:%08X\n",bad,
        d->regs[NV_PAPU_TVL2D],d->regs[NV_PAPU_TVL3D],d->regs[NV_PAPU_TVLMP]);
    uint32_t base=d->regs[NV_PAPU_VPVADDR];unsigned active=0;
    for(unsigned v=0;v<MCPX_HW_MAX_VOICES;++v){
        uint64_t addr=(uint64_t)base+v*NV_PAVS_SIZE;
        if(!d->ram_ptr || addr>g_apu_ram_size || NV_PAVS_SIZE>g_apu_ram_size-addr)break;
        uint32_t state=ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_PAR_STATE);
        if(!(state & NV_PAVS_VOICE_PAR_STATE_ACTIVE_VOICE))continue;
        ++active;
        fprintf(stderr,"[APU-SILENT-VOICE] report=%u voice=%u state=%08X fmt=%08X bins=%08X vol=%08X:%08X:%08X offset=%08X link=%08X ssl=%u:%u:%u:%u\n",
          d->diagnostic_zero_reports,v,state,
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_CFG_FMT),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_CFG_VBIN),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_TAR_VOLA),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_TAR_VOLB),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_TAR_VOLC),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_PAR_OFFSET),
          ldl_le_phys(address_space_memory,addr+NV_PAVS_VOICE_TAR_PITCH_LINK),
          d->vp.ssl[v].base[0],d->vp.ssl[v].count[0],d->vp.ssl[v].base[1],d->vp.ssl[v].count[1]);
    }
    fprintf(stderr,"[APU-SILENT-COVERAGE] report=%u active=%u\n",d->diagnostic_zero_reports,active);
}
