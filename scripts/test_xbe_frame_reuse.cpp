#include "xbe_frame_reuse.h"
#include "xbe_upload_spill.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    uc_frame_reuse::UploadSpill spill;
    auto r=spill.Get(1,16);CHECK(r.page==0&&r.offset==0);
    r=spill.Get(256,256);CHECK(r.page==0&&r.offset==256);
    r=spill.Get(spill.page_bytes-512,16);CHECK(r.page==0&&r.offset==512);
    r=spill.Get(spill.page_bytes,16);CHECK(r.page==1&&r.offset==0);
    r=spill.Get(1,16);CHECK(r.page==spill.page_count&&spill.Exhausted());
    spill.Reset();CHECK(!spill.Exhausted());
    r=spill.Get(spill.page_bytes+1,16);CHECK(r.page==spill.page_count);
    spill.Reset();r=spill.Get(spill.page_bytes,16);CHECK(r.page==0&&r.offset==0);
    uc_frame_reuse::Qualification<std::array<uint32_t,2>> fog;
    unsigned checks=0;
    auto check=[&]{++checks;return 3u;};
    CHECK(fog.Get(1,{1,2},check)==3 && checks==1);
    for(int i=0;i<10000;++i)CHECK(fog.Get(1,{1,2},check)==3);
    CHECK(checks==1);
    CHECK(fog.Get(1,{1,3},check)==3 && checks==2); // Forced hash collision.
    CHECK(fog.Get(2,{0,0},[]{return 0u;})==0);
    CHECK(fog.Get(2,{0,0},[]{std::abort();return 1u;})==0); // Cache rejection too.
    for(uint64_t i=3;i<1027;++i)fog.Get(i,{uint32_t(i),0},check);
    CHECK(fog.Get(1,{1,2},check)==3);

    std::puts("PASS bounded upload spill and exact fog keys/rejections/collisions");
}
