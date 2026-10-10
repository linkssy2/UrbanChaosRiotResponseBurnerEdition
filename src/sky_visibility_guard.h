#ifndef UC_SKY_VISIBILITY_GUARD_H
#define UC_SKY_VISIBILITY_GUARD_H
static int uc_sky_range(uint32_t p,uint32_t n){return p>=0x10000u&&!(p&3u)&&(uint64_t)p+n<=g_xbox_total_ram;}
/* Sky factory owner -> selected wrapper -> renderer, never an asset-name guess. */
static int uc_sky_visibility_guard(uint32_t owner,uint32_t renderer,unsigned index){
 if(!uc_sky_range(owner,0xE0)||MEM32(owner)!=0x465578u)return 0;
 unsigned count=MEM16(owner+0xBC);if(!count||count>5||index>=count)return 0;
 uint32_t wrapper=MEM32(owner+0x80+12*index);
 if(!uc_sky_range(wrapper,0x50)||MEM32(wrapper)!=renderer||!uc_sky_range(renderer,0x54))return 0;
 uint32_t table=MEM32(renderer);
 return uc_sky_range(table,0x18)&&MEM32(table+0x10)==0x8A080u&&MEM32(table+0x14)==0x8A090u;
}
#endif
