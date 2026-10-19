#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

struct Result { std::array<unsigned char,224> constants{};unsigned reads=0;std::array<int,4> resources{}; };
struct LookupResult {
    std::array<uint32_t,16> output{};
    unsigned reads=0,finds=0;
    std::vector<unsigned> effects;
};

namespace before {
#define NEW_DIMENSIONS 0
#include "test_xbe_texture_constants_model.inc"
/* BEFORE */
#undef NEW_DIMENSIONS
}
namespace after {
#define NEW_DIMENSIONS 1
#include "test_xbe_texture_constants_model.inc"
/* AFTER */
#undef NEW_DIMENSIONS
}
namespace lookup_before {
#define NEW_DIMENSIONS 1
#include "test_xbe_texture_constants_model.inc"
/* LOOKUP_BEFORE */
#undef NEW_DIMENSIONS
}

int main() {
    unsigned cases=0;
    for(unsigned kind=0;kind<9;++kind)for(unsigned extent=0;extent<8;++extent)
    for(unsigned ext=0;ext<2;++ext)for(unsigned mutate=0;mutate<3;++mutate) {
        if(mutate && !ext)continue; // Snapshot headers cannot change during a production Draw.
        auto a=before::Run(kind,extent,ext,mutate),b=after::Run(kind,extent,ext,mutate);
        assert(a.constants==b.constants && a.resources==b.resources);
        const unsigned present=kind==0?0:4;
        assert(a.reads==2*present && b.reads==(ext?2:1)*present);
        ++cases;
    }
    std::printf("PASS %u exact constant/resource cases: null/invalid, guest/target/alias, failed binds, prompt/fog replacements, changed dimensions and extension mutation; Describe reads halved only without extensions\n",cases);
    unsigned lookup_cases=0,saved=0;
    for(unsigned format:{0u,6u,88u,99u,0x40u})for(unsigned cube=0;cube<2;++cube)
    for(unsigned depth:{1u,2u})for(unsigned target=0;target<4;++target)
    for(unsigned extension=0;extension<2;++extension)for(unsigned fail=0;fail<2;++fail)
    for(unsigned presence=0;presence<3;++presence)for(unsigned alt=0;alt<2;++alt) {
        auto a=lookup_before::RunLookup(format,cube,depth,target,extension,fail,presence,alt);
        auto b=after::RunLookup(format,cube,depth,target,extension,fail,presence,alt);
        assert(a.output==b.output && a.reads==b.reads && a.effects==b.effects);
        const unsigned skipped=presence==2 && (cube || depth!=1 || format==88);
        assert(a.finds==b.finds+skipped);
        saved+=skipped;++lookup_cases;
    }
    std::printf("PASS %u exact target-eligibility cases: %u unnecessary searches removed; extension callback timing, rendered targets, depth aliases, failed binds and guest reads preserved\n",lookup_cases,saved);
}
