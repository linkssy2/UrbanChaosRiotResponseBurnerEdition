// Compare the extracted production packing blocks, including every slot mask.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
static void Original(std::vector<float>& packed,const std::vector<float>& g_im_vertices,uint32_t count,uint32_t g_im_mask) {
/* PACK_OLD */
}
static void Candidate(std::vector<float>& packed,const std::vector<float>& g_im_vertices,uint32_t count,uint32_t g_im_mask) {
/* PACK_NEW */
}
static volatile uint64_t sink=0;
using Pack=decltype(&Original);
static double Bench(Pack pack,unsigned count,unsigned mask) {
    std::vector<float> input(size_t(count)*64,1),output;
    pack(output,input,count,mask);
    auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<10000;++i) {
        input[0]=float(i);pack(output,input,count,mask);
        sink=sink+output.size()+(output.empty()?0:uint64_t(output[0]));
    }
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/10000;
}
int main(int argc,char**) {
    std::vector<float> input(257*64),a,b;
    uint32_t rng=91;
    for(auto& value:input){rng=rng*1664525u+1013904223u;std::memcpy(&value,&rng,4);}
    unsigned checks=0;
    for(unsigned mask=0;mask<65536;++mask)for(unsigned count:{0u,1u,4u,7u}) {
        Original(a,input,count,mask);Candidate(b,input,count,mask);
        assert(a.size()==b.size());
        assert(a.empty()||!std::memcmp(a.data(),b.data(),a.size()*4));++checks;
    }
    for(unsigned count:{257u,2u,128u,0u,1u})for(unsigned mask:{0u,1u,0x8000u,0x8421u,0xffffu,0xffffffffu}) {
        Original(a,input,count,mask);Candidate(b,input,count,mask);
        assert(a.size()==b.size());assert(a.empty()||!std::memcmp(a.data(),b.data(),a.size()*4));++checks;
    }
    std::printf("immediate packing: %u exact byte comparisons passed\n",checks);
    if(argc>1)for(unsigned count:{4u,64u,256u})for(unsigned mask:{1u,0x109u,0x8421u,0xffffu}) {
        std::array<double,7>x,y;
        for(unsigned r=0;r<7;++r) {
            if(r&1){y[r]=Bench(Candidate,count,mask);x[r]=Bench(Original,count,mask);}
            else{x[r]=Bench(Original,count,mask);y[r]=Bench(Candidate,count,mask);}
        }
        std::sort(x.begin(),x.end());std::sort(y.begin(),y.end());
        std::printf("vertices=%u mask=%04x original_ns=%.2f candidate_ns=%.2f change_pct=%+.2f\n",count,mask,x[3],y[3],100*(y[3]/x[3]-1));
    }
}
