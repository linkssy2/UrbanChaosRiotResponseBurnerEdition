#ifndef UC_PARTICLE_REGISTRATION_GUARD_H
#define UC_PARTICLE_REGISTRATION_GUARD_H
/* Returns class+1 only for the constructor/static-source graph whose original
 * null-registration cleanup is covered by particle_registration_checks. */
static unsigned uc_particle_registration_class(uint32_t r, uint32_t caller) {
    static const uint32_t primary[]={0x482FAC,0x482FE0,0x483010,0x48303C,0x483070,0x48309C};
    static const uint32_t render[]={0x482F90,0x482FC0,0x482FF0,0x483020,0x483050,0x483080};
    static const uint32_t callers[]={0x2390A3,0x23A2F6,0x23ACC6,0x23B543,0x23BC56,0x23D161};
    if(r<0x10004u||(r&3u)||(uint64_t)r+0xB8u>g_xbox_total_ram)return 0;
    unsigned i;for(i=0;i<6;i++)if(MEM32(r-4)==primary[i]&&MEM32(r)==render[i]&&caller==callers[i])break;
    if(i==6)return 0;
    uint32_t source=MEM32(r+0x1C);
    if(source<0x10000u||(source&3u)||(uint64_t)source+0x34u>g_xbox_total_ram||MEM32(source)!=0x480D7Cu)return 0;
    return i+1;
}
#endif
