// Exercise the extracted production palette reader, without a GPU or game.
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1); } } while (0)

struct State { std::array<uint32_t,4> palettes{}; };
struct TextureDesc { uint32_t format=0; };
std::array<uint8_t,0x4000> memory{};
unsigned reads=0,hashes=0;
namespace kernel {
uint32_t R32(uint32_t address) {
    CHECK(address<=memory.size()-4);++reads;
    uint32_t value;std::memcpy(&value,memory.data()+address,4);return value;
}
const void* HostPtr(uint32_t address) { CHECK(address<memory.size());++reads;return memory.data()+address; }
}
namespace mem { uint32_t VirtualFromPhysical(uint32_t address) { return address; } }
uint64_t Hash(const void* bytes,size_t length) {
    // Exact colour-byte comparison below is stronger than this hash witness.
    CHECK(length==1024);++hashes;uint64_t result=1469598103934665603ull;
    for(size_t i=0;i<length;++i)result=(result^static_cast<const uint8_t*>(bytes)[i])*1099511628211ull;
    return result;
}
struct Result {
    uint32_t va=0;uint64_t hash=0;
    unsigned read_count=0,hash_count=0;
    std::array<uint32_t,256> colours{};
    bool operator==(const Result&) const=default;
};
template<class Palette,class Reader>
Result Read(const State& state,int stage,const TextureDesc& desc,Reader reader) {
    reads=hashes=0;Palette palette;reader(state,stage,desc,palette);
    Result result;result.va=palette.va;result.hash=palette.hash;
    result.read_count=reads;result.hash_count=hashes;
    // A skipped palette leaves only va/hash observable; UploadTexture receives
    // nullptr. A read at physical address zero still hashes the full palette.
    if(hashes)std::memcpy(result.colours.data(),palette.colours,sizeof(palette.colours));
    return result;
}
namespace before {
/* ORIGINAL_PALETTE */
Result Run(const State& state,int stage,const TextureDesc& desc) { return Read<StagePalette>(state,stage,desc,ReadStagePalette); }
}
namespace after {
/* PRODUCTION_PALETTE */
Result Run(const State& state,int stage,const TextureDesc& desc) { return Read<StagePalette>(state,stage,desc,ReadStagePalette); }
}
void W32(uint32_t address,uint32_t value) { CHECK(address<=memory.size()-4);std::memcpy(memory.data()+address,&value,4); }
unsigned checks=0;
Result Compare(const State& state,int stage,uint32_t format) {
    const auto old=before::Run(state,stage,{format}),next=after::Run(state,stage,{format});
    CHECK(old==next);++checks;return next;
}
int main() {
    constexpr uint32_t header=0x2000;
    State state;
    for(unsigned size=0;size<4;++size)for(unsigned stage=0;stage<4;++stage)
    for(unsigned pattern=0;pattern<4;++pattern)for(uint32_t address:{0u,0x1000u}) {
        const unsigned count=256u>>size;
        for(unsigned i=0;i<256;++i)W32(address+i*4,pattern==0?0:pattern==1?UINT32_MAX:pattern==2?i*0x01020304u:~(i*0x1234567u));
        W32(header,size<<30);W32(header+4,address);
        state.palettes.fill(0);state.palettes[stage]=header;
        auto result=Compare(state,stage,0x0B);
        CHECK(result.va==address&&result.hash_count==1&&result.read_count==3);
        CHECK(!std::memcmp(result.colours.data(),memory.data()+address,count*4));
        for(unsigned i=count;i<256;++i)CHECK(result.colours[i]==0);
        const auto previous=result;
        memory[address+(count/2)*4]^=0x57;
        result=Compare(state,stage,0x0B);
        CHECK(result.colours[count/2]!=previous.colours[count/2]);
        // Non-P8 ignores even an invalid palette header, without touching memory.
        state.palettes[stage]=UINT32_MAX;
        for(uint32_t format:{0x00u,0x06u,0x12u,0x40u,0xFFFFu}) {
            result=Compare(state,stage,format);
            CHECK(!result.va&&!result.hash&&!result.read_count&&!result.hash_count);
        }
        state.palettes[stage]=0;result=Compare(state,stage,0x0B);
        CHECK(!result.va&&!result.hash&&!result.read_count&&!result.hash_count);
    }
    // Poison retained stack storage: shorter palettes must clear the unused tail.
    for(unsigned size=0;size<4;++size) {
        state.palettes[0]=header;W32(header,size<<30);W32(header+4,0x1000);
        after::StagePalette palette;
        std::memset(palette.colours,0xA7,sizeof(palette.colours));
        CHECK(palette.va==0&&palette.hash==0);
        after::ReadStagePalette(state,0,{0x0B},palette);
        for(unsigned i=256u>>size;i<256;++i)CHECK(palette.colours[i]==0);
        ++checks;
    }
    std::printf("PASS %u palette comparisons: all four sizes/stages, exact colours/hash/reads, zero tails, mutations, absent/non-P8 and address-zero palettes\n",checks);
}
