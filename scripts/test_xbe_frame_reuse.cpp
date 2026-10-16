#include "xbe_frame_reuse.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
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

    uc_frame_reuse::Geometry<unsigned> geometry;
    unsigned uploads=0;std::vector<std::vector<uint8_t>> snapshots;
    std::vector<uint8_t> bytes(16384,17);
    auto upload=[&]{snapshots.push_back(bytes);return ++uploads;};
    auto get=[&]{auto id=geometry.Get(bytes.data(),bytes.size(),upload);CHECK(snapshots[id-1]==bytes);return id;};
    CHECK(get()==1);for(int i=0;i<1000;++i)CHECK(get()==1);
    bytes[7000]^=1;CHECK(get()==2);CHECK(get()==2); // Unsampled interior write.
    bytes[0]^=1;CHECK(get()==3);
    geometry.Reset();CHECK(get()==4); // Old upload offsets cannot survive a submission.
    bytes.resize(8192);CHECK(get()==5);CHECK(get()==5);
    bytes.resize(64);CHECK(get()==6);CHECK(get()==7);
    bytes.resize(262145);CHECK(get()==8);CHECK(get()==9);
    // Distinct pointers sharing a direct-cache slot cannot alias old geometry.
    std::vector<uint8_t> collision(4096,1);unsigned n=0;
    auto put=[&](size_t at){return geometry.Get(collision.data()+at,1024,[&]{return ++n;});};
    CHECK(put(0)==1);CHECK(put(1024)==2);CHECK(put(0)==3);
    std::puts("PASS exact fog keys/rejections/collisions, mutable geometry, size changes, bounded bypass and upload-generation reset");
}
