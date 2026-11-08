#define UC_BURNER_RELEASE
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
static uint64_t checks=0;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(#x);}while(false)
struct Result {
    bool success=true;uint64_t finds=0;std::vector<unsigned> output,effects;
    std::array<unsigned,3> callback_counts{};
    std::map<uint64_t,std::vector<unsigned>> targets;
    std::map<unsigned,uint64_t> used;
};
namespace before {
#include "test_xbe_target_lookup_model.inc"
/* BEFORE */
#define UC_TARGET_LOOKUP_SCENARIOS
#include "test_xbe_target_lookup_model.inc"
#undef UC_TARGET_LOOKUP_SCENARIOS
}
namespace after {
#include "test_xbe_target_lookup_model.inc"
/* AFTER */
#define UC_TARGET_LOOKUP_SCENARIOS
#include "test_xbe_target_lookup_model.inc"
#undef UC_TARGET_LOOKUP_SCENARIOS
}
namespace unchecked {
#include "test_xbe_target_lookup_model.inc"
/* UNCHECKED */
#define UC_TARGET_LOOKUP_SCENARIOS
#include "test_xbe_target_lookup_model.inc"
#undef UC_TARGET_LOOKUP_SCENARIOS
}
int main(){try{
    uint64_t cases=0,saved=0;
    for(unsigned kind=0;kind<12;++kind)for(unsigned extent=0;extent<2;++extent)
    for(unsigned variant=0;variant<16;++variant)for(bool ext:{false,true})for(unsigned event=0;event<12;++event){
        if(event>=9&&!ext)continue;
        auto a=before::Run(kind,extent,variant,ext,event),b=after::Run(kind,extent,variant,ext,event);
        if(a.success!=b.success||a.output!=b.output||a.effects!=b.effects||a.targets!=b.targets||a.used!=b.used||a.callback_counts!=b.callback_counts){
            std::printf("FAIL kind=%u extent=%u variant=%u extension=%u event=%u: success=%u output=%u effects=%u targets=%u used=%u callbacks=%u\n",kind,extent,variant,ext,event,a.success==b.success,a.output==b.output,a.effects==b.effects,a.targets==b.targets,a.used==b.used,a.callback_counts==b.callback_counts);return 2;
        }
        CHECK(a.finds>=b.finds);if(ext)CHECK(a.finds==b.finds);
        saved+=a.finds-b.finds;++cases;
    }
    CHECK(saved>0);
    auto original=before::Run(3,0,0,false,5),unsafe=unchecked::Run(3,0,0,false,5);
    CHECK(original.output!=unsafe.output);
    std::puts("PASS negative witness: unconditional pointer reuse changes wide-filter flags after a current-header size change; narrowed size-checked candidate retains original output");
    std::printf("PASS %llu production-derived stage cases; %llu map lookups eliminated; exact selection/filter flags/resource-operation ledger/target metadata/freshness/extension callbacks; %llu assertions\n",(unsigned long long)cases,(unsigned long long)saved,(unsigned long long)checks);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
