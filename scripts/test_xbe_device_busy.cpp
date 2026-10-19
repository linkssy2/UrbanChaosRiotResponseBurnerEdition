// Production polling body with a held render queue: no false completion or repeated submissions.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"failed line %d\n",__LINE__);std::exit(1);}}while(0)
enum class Op {Draw,Flush};
struct Cmd{Op op;};
struct Batch {uint64_t id=1;std::vector<Cmd> cmds;};
struct Probe {
    Batch current;Batch* cur_=&current;
    std::atomic<uint64_t> done_{0};uint64_t busy_probe_=0;
    std::vector<Batch> queued;
    uint64_t Submit(){auto id=current.id;queued.push_back(current);current={id+1,{}};return id;}
    /* METHOD */
};
int main(){
    Probe p;
    CHECK(!p.DeviceBusy() && p.queued.empty());
    p.current.cmds.push_back({Op::Draw});
    CHECK(p.DeviceBusy() && p.queued.size()==1);
    CHECK(p.queued[0].cmds.back().op==Op::Flush);
    for(int i=0;i<10000;++i)CHECK(p.DeviceBusy());
    CHECK(p.queued.size()==1); // The render worker has not completed its GPU wait.
    p.done_.store(1,std::memory_order_release);
    CHECK(!p.DeviceBusy());
    p.current.cmds.push_back({Op::Draw});p.Submit(); // Other submission path.
    CHECK(p.DeviceBusy() && p.queued.size()==3);
    p.done_.store(2);CHECK(p.DeviceBusy());
    p.done_.store(3);CHECK(!p.DeviceBusy());
    p.current.cmds.push_back({Op::Draw});CHECK(p.DeviceBusy());
    p.current.cmds.push_back({Op::Draw});CHECK(p.DeviceBusy());
    p.done_.store(4);CHECK(p.DeviceBusy());
    p.done_.store(5);CHECK(!p.DeviceBusy());
    std::puts("device busy: idle, pending, repeated polls, completed markers and intervening work passed");
}
