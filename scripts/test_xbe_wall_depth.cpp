#include "xbe_wall_depth.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
struct Key { int zen=1,zwrite=0,zfunc=0x203,blend=1,src=1,dst=1,zbias=-3; };
#define CHECK(x) do { if (!(x)) std::abort(); } while (0)
int main() {
    Key k;
    const auto bias=[](const Key& x){return uc_wall_depth::ReceiverBias(0xB01013A1,0xB0101860,x);};
    CHECK(bias(k)==-64);
    CHECK(uc_wall_depth::ReceiverBias(0xB01012E1,0xB0101860,k)==-3);
    CHECK(uc_wall_depth::ReceiverBias(0xB01013A1,0xB0101920,k)==-3);
    for (auto member : {&Key::zen,&Key::zwrite,&Key::zfunc,&Key::blend,&Key::src,&Key::dst}) {
        Key other=k; other.*member ^= 1; CHECK(bias(other)==-3);
    }
    for (int original : {0,-1,-2,-4,-64,3}) {Key other=k;other.zbias=original;CHECK(bias(other)==original);}
    std::puts("PASS wall receiver tolerance; other shaders, depth writers, comparisons, blends and authored offsets unchanged");
}
