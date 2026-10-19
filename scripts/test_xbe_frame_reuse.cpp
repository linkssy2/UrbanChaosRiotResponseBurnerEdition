#include "xbe_frame_reuse.h"
#include "xbe_upload_spill.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <type_traits>
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

    uc_frame_reuse::Geometry<unsigned> geometry;
    static_assert(std::is_empty_v<decltype(geometry)>); // No CPU shadow storage.
    unsigned uploads=0;std::vector<std::vector<uint8_t>> snapshots;
    std::vector<uint8_t> bytes(16384,17);
    auto upload=[&]{snapshots.push_back(bytes);return ++uploads;};
    auto get=[&]{auto id=geometry.Get(bytes.data(),bytes.size(),upload);CHECK(snapshots[id-1]==bytes);return id;};
    CHECK(get()==1);for(unsigned i=0;i<1000;++i)CHECK(get()==i+2);
    const auto original=snapshots.front();
    bytes[7000]^=1;CHECK(get()==1002);CHECK(get()==1003); // Interior writes are included.
    const auto modified=snapshots.back();
    bytes[0]^=1;CHECK(get()==1004);
    geometry.Reset();CHECK(get()==1005); // Every request uses the current upload arena.
    bytes.resize(8192);CHECK(get()==1006);CHECK(get()==1007);
    bytes.resize(64);CHECK(get()==1008);CHECK(get()==1009);
    bytes.resize(262145);CHECK(get()==1010);CHECK(get()==1011);
    CHECK(snapshots.front()==original&&snapshots[1002]==modified);
    // Aligned and colliding addresses have identical fresh-snapshot semantics.
    std::vector<uint8_t> aligned_storage(2*65536);
    auto* aligned=reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(aligned_storage.data())+65535)&~uintptr_t(65535));
    uc_frame_reuse::Geometry<unsigned> aligned_geometry;unsigned n=0;
    auto put=[&](size_t at){return aligned_geometry.Get(aligned+at,1024,[&]{return ++n;});};
    for(unsigned i=0;i<64;++i)CHECK(put(i*1024)==i+1);
    for(unsigned i=0;i<64;++i)CHECK(put(i*1024)==i+65);
    CHECK(n==128);
    std::vector<uint8_t> collision(129*1024);uc_frame_reuse::Geometry<unsigned> crowded;
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<129;++i)
        CHECK(crowded.Get(collision.data()+i*1024,1024,[&]{return i;})==i);
    CHECK(snapshots.front()==original&&snapshots[1002]==modified);
    std::puts("PASS exact fog keys/rejections/collisions, fresh immutable geometry snapshots, size changes, alignment and upload-generation reset");
}
