// Format is extracted from the production source; no game or GPU is required.
#include "gpu/texture.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1); } } while (0)
using gpu::FormatInfo;
using gpu::Host;
namespace before {
/* BEFORE */
}
namespace after {
/* AFTER */
}
namespace synthetic {
/* SYNTHETIC */
}

template<class Table>
const FormatInfo& Oracle(uint32_t id,const Table& entries,const FormatInfo& unknown) {
    for(const auto& e:entries)if(e.id==id)return e.info;
    return unknown;
}
void Compare(uint32_t id) {
    const auto& a=before::Format(id);const auto& b=after::Format(id);
    CHECK(!std::strcmp(a.name,b.name)&&a.bits==b.bits&&a.swizzled==b.swizzled&&
        a.compressed==b.compressed&&a.depth==b.depth&&a.host==b.host);
    CHECK(&after::Format(id)==&Oracle(id,after::kFormats,after::kUnknown));
    CHECK(&synthetic::Format(id)==&Oracle(id,synthetic::kFormats,synthetic::kUnknown));
}
int main() {
    for(uint32_t id=0;id<65536;++id)Compare(id);
    for(uint32_t id:{0x10000u,0x7FFFFFFFu,0x80000000u,0xFFFFFFFEu,0xFFFFFFFFu})Compare(id);
    CHECK(!std::strcmp(synthetic::Format(0x06).name,"A8R8G8B8"));
    CHECK(!std::strcmp(synthetic::Format(0x08).name,"first_gap"));
    CHECK(!std::strcmp(synthetic::Format(0x123).name,"first_high"));
    CHECK(!std::strcmp(synthetic::Format(0xFFFFFFFFu).name,"maximum_id"));
    CHECK(&after::Format(0x08)==&after::Format(0xFFFFFFFFu));
    std::puts("PASS 65541 format IDs: exact fields/object addresses, unknown fallback, first duplicate and high IDs");
}
