#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
#include <thread>
#include <unordered_map>
#include <vector>

namespace test {
uint64_t checks=0;
void Require(bool ok,const char* message) {
    ++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::abort();}
}
struct Gate {
    std::mutex mutex;std::condition_variable changed;
    bool armed=false,entered=false,released=false;
    void Arm(){std::lock_guard lock(mutex);armed=true;entered=released=false;}
    void Pause(){
        std::unique_lock lock(mutex);if(!armed)return;
        armed=false;entered=true;changed.notify_all();
        changed.wait(lock,[&]{return released;});
    }
    void Wait(){std::unique_lock lock(mutex);changed.wait(lock,[&]{return entered;});}
    void Release(){std::lock_guard lock(mutex);released=true;changed.notify_all();}
};
Gate map_gate,recheck_gate;
thread_local bool throw_insert=false, pause_map=false, pause_recheck=false;
void Recheck(){if(pause_recheck)recheck_gate.Pause();}
struct Map {
    std::unordered_map<uint32_t,uint64_t> values;
    uint64_t finds=0;
    auto find(uint32_t key){++finds;return values.find(key);}
    auto end(){return values.end();}
    uint64_t& operator[](uint32_t key){
        if(pause_map)map_gate.Pause();
        if(throw_insert){throw_insert=false;throw std::bad_alloc();}
        return values[key];
    }
};
}

// The generated fixture header inserts a scheduling gate into the exact
// production algorithm; the runtime header contains no scheduling hook.
#include "test_dirty_epoch_cache.h"
namespace mem {uint32_t VirtualFromPhysical(uint32_t value){return value;}}
namespace kernel {uint32_t R32(uint32_t address){return address-4;}}

namespace before {
std::mutex g_dirty_lock;test::Map g_dirty;uint64_t g_epoch=1;
// BEFORE_FUNCTIONS
}
namespace after {
std::mutex g_dirty_lock;test::Map g_dirty;uint64_t g_epoch=1;
// AFTER_FUNCTIONS
}

namespace {
using test::Require;
void FreshCache(){uc_dirty_epoch::cache={};after::g_dirty.finds=0;}
void Sequential(){
    FreshCache();
    Require(after::DirtyEpoch(17)==0,"uncached absent key");
    Require(after::DirtyEpoch(17)==0,"cached absent key");
    Require(after::g_dirty.finds==1,"negative result hit avoided map lookup");
    for(uint32_t n=1;n<=3000;++n){
        const uint32_t key=1+(n*17)%53;
        before::MarkDirty(key);after::MarkDirty(key);
        const auto expected=before::DirtyEpoch(key);
        Require(after::DirtyEpoch(key)==expected,"write refresh");
        Require(after::DirtyEpoch(key)==expected,"same-key warm read");
        const uint32_t other=1+(n*29)%53;
        Require(after::DirtyEpoch(other)==before::DirtyEpoch(other),"other key exact value");
        Require(after::DirtyEpoch(0x10000+n)==0,"new absent key after generation change");
    }
    auto expected_epoch=before::g_epoch;
    const auto expected_map=before::g_dirty.values;
    before::MarkDirty(0);after::MarkDirty(0);
    Require(before::g_epoch==expected_epoch&&after::g_epoch==expected_epoch,"zero resource leaves epoch unchanged");
    Require(before::g_dirty.values==expected_map&&after::g_dirty.values==expected_map,"zero resource leaves map unchanged");

}
void InsertionFailure(){
    FreshCache();const auto value=after::DirtyEpoch(0x20000);Require(value==0,"failure key initially absent");
    const auto old_revision=uc_dirty_epoch::revision.load();
    bool caught_before=false,caught_after=false;
    test::throw_insert=true;try{before::MarkDirty(0x20000);}catch(const std::bad_alloc&){caught_before=true;}
    test::throw_insert=true;try{after::MarkDirty(0x20000);}catch(const std::bad_alloc&){caught_after=true;}
    Require(caught_before&&caught_after,"allocation exception preserved");
    Require(before::g_epoch==after::g_epoch,"pre-insertion increment-on-throw preserved");
    Require(before::g_dirty.values==after::g_dirty.values,"failed insertion leaves same map");
    Require(uc_dirty_epoch::revision.load()==old_revision+2,"exception restores new even revision");
    const auto misses=after::g_dirty.finds;
    Require(after::DirtyEpoch(0x20000)==0,"failed insertion remains absent");
    Require(after::g_dirty.finds==misses+1,"failed insertion invalidates negative cache");
    before::MarkDirty(0x20000);after::MarkDirty(0x20000);
    Require(after::DirtyEpoch(0x20000)==before::DirtyEpoch(0x20000),"recovery write after insertion failure");
}
void WriterInProgress(){
    constexpr uint32_t key=0x30000;
    after::MarkDirty(key);const auto old=after::DirtyEpoch(key);
    test::map_gate.Arm();
    std::thread writer([&]{test::pause_map=true;after::MarkDirty(key);});
    test::map_gate.Wait();
    Require((uc_dirty_epoch::revision.load()&1)!=0,"writer owns odd revision before map mutation");
    uint64_t untouched=0xabcdef;
    Require(!uc_dirty_epoch::TryRead(key,untouched)&&untouched==0xabcdef,"odd revision refuses old warmed value");
    std::atomic<bool> reader_started=false,reader_done=false;
    uint64_t result=0;
    std::thread reader([&]{reader_started.store(true);result=after::DirtyEpoch(key);reader_done.store(true);});
    while(!reader_started.load())std::this_thread::yield();
    Require(!reader_done.load(),"reader cannot finish while writer holds map mutex");
    test::map_gate.Release();writer.join();reader.join();
    Require(result>old,"blocked reader sees completed writer");
    Require((uc_dirty_epoch::revision.load()&1)==0,"successful writer publishes even revision");
}
void InterveningWriter(){
    constexpr uint32_t key=0x30001;
    after::MarkDirty(key);
    test::recheck_gate.Arm();
    uint64_t old=0,result=0;
    std::thread reader([&]{
        old=after::DirtyEpoch(key);
        test::pause_recheck=true;
        result=after::DirtyEpoch(key);
    });
    test::recheck_gate.Wait();
    after::MarkDirty(key);const auto expected=after::DirtyEpoch(key);
    test::recheck_gate.Release();reader.join();
    Require(result==expected&&result>old,"writer between acquire loads cannot return stale cached value");
}
void ConcurrentStress(){
    constexpr unsigned keys=31,iterations=18000;
    constexpr uint32_t first=0x40000;
    std::atomic<bool> start=false,done=false,ok=true;
    std::atomic<uint64_t> reads=0;
    std::vector<std::thread> readers;
    for(unsigned thread=0;thread<4;++thread)readers.emplace_back([&,thread]{
        uint64_t seen[keys]{};unsigned n=thread;
        while(!start.load(std::memory_order_acquire))std::this_thread::yield();
        do{
            const unsigned key=(n++/4)%keys;
            const auto result=after::DirtyEpoch(first+key);
            if(result<seen[key])ok.store(false);
            seen[key]=result;reads.fetch_add(1,std::memory_order_relaxed);
        }while(!done.load(std::memory_order_acquire));
    });
    std::thread writer([&]{
        start.store(true,std::memory_order_release);
        for(unsigned i=0;i<iterations;++i)after::MarkDirty(first+i%keys);
        done.store(true,std::memory_order_release);
    });
    writer.join();for(auto& reader:readers)reader.join();
    Require(ok.load()&&reads.load()>0,"concurrent readers return monotonic per-key epochs");
    for(unsigned i=0;i<keys;++i){
        uint64_t expected;
        {std::lock_guard lock(after::g_dirty_lock);expected=after::g_dirty.values.at(first+i);}
        Require(after::DirtyEpoch(first+i)==expected,"post-join reads see final exact values");
    }
    std::printf("stress_reads=%llu ",(unsigned long long)reads.load());
}
void Exhaustion(){
    constexpr auto maximum=std::numeric_limits<uint64_t>::max();
    constexpr uint32_t key=0x50000;
    uc_dirty_epoch::revision.store(maximum-1);
    FreshCache();after::DirtyEpoch(key);after::DirtyEpoch(key);
    Require(after::g_dirty.finds==1,"last even revision may cache before next write");
    after::MarkDirty(key);
    Require(uc_dirty_epoch::revision.load()==maximum,"revision exhaustion permanently fails closed");
    const auto expected=after::DirtyEpoch(key),finds=after::g_dirty.finds;
    for(unsigned i=0;i<10;++i)Require(after::DirtyEpoch(key)==expected,"exhausted revision keeps exact locked result");
    Require(after::g_dirty.finds==finds+10,"exhaustion never reuses cache");
    after::MarkDirty(key);
    Require(uc_dirty_epoch::revision.load()==maximum&&after::DirtyEpoch(key)>expected,"subsequent writes remain valid under permanent fallback");
}
}
int main(){
    Sequential();InsertionFailure();WriterInProgress();InterveningWriter();ConcurrentStress();Exhaustion();
    std::printf("PASS checks=%llu cache_bytes=%zu\n",
        (unsigned long long)test::checks,sizeof(uc_dirty_epoch::Cache));
}
