// Native code-entry/toggle/reset paths plus the production host/guest queue.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
namespace cheat_test {
std::array<uint8_t,0x800000> memory{};
constexpr uint32_t P=0x5152b0,W=0x600000,L=0x601000,A=0x602000,V=0x603000,M=0x604000,S=0x7f0000;
bool committed=true;
std::vector<uint32_t> events;
uint8_t* Data(){return memory.data();}
uint32_t Word(uint32_t p){uint32_t v;std::memcpy(&v,Data()+p,4);return v;}
void Put(uint32_t p,uint32_t v){std::memcpy(Data()+p,&v,4);}
}
namespace mem {bool IsCommitted(uint32_t page){return cheat_test::committed&&page<cheat_test::memory.size();}}
#include "../common/xbe_cheats.cpp"
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_000643E0);
X86_FUNC(sub_00057FC0){
    using namespace cheat_test;assert(Word(ctx.esp+4)==0x450958);events.push_back(0x57fc0);
    // Caller state, including SSE status, must not leak into the menu tick.
    ctx.eax=0xBAD;ctx.ecx=0xBAD;ctx.edx=0xBAD;ctx.xmm[4].u[2]=0xBAD;
    _mm_setcsr(0x1fa0);ctx.esp+=4;
}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target){
    using namespace cheat_test;assert(ctx.ecx==A);
    events.push_back(target);
    if(target==0x20000){assert(Word(ctx.esp+4)==0x450f88&&Word(ctx.esp+8)==0x3f800000);ctx.esp+=12;}
    else {assert(target==0x20004);events.push_back(Word(ctx.esp+4));ctx.esp+=8;}
    ctx.eax=0xBAD;ctx.ecx=0xBAD;ctx.edx=0xBAD;ctx.xmm[5].u[1]=0xBAD;
}
// NATIVE_CHEAT_CONTRACTS
#include "../common/xbe_cheats_native.inc"
namespace cheat_test {
X86Context Context(){
    X86Context c{};c.esp=S;c.eax=11;c.ebx=13;c.ecx=17;c.edx=19;c.esi=23;c.edi=29;c.ebp=31;
    c.xmm[1].u[2]=37;c.cf=1;c.pf=1;c.zf=1;return c;
}
void Seed(uint32_t enabled=0,uint32_t known=0,bool restricted=false){
    memory.fill(0);committed=true;events.clear();TrialCheatsInvalidate();
    std::memcpy(Data()+0x501728,NativeTable,sizeof NativeTable);
    Put(0x51c4dc,W);Put(W+4,L);Put(W+0x6c,A);Put(A,V);
    Put(V+0x50,0x20000);Put(V+0x68,0x20004);
    Put(0x5096e0,M);Data()[M+4]=restricted?0x10:0;
    Put(P,123);std::memcpy(Data()+P+4,"Nick Mason",11);
    for(unsigned i=0;i<trial_cheats::Count;++i){
        Data()[P+0x179+cheats::offsets[i]]=uint8_t((enabled>>i)&1);
        Data()[P+0x188+cheats::offsets[i]]=uint8_t((known>>i)&1);
    }
    for(unsigned i=0;i<1024;++i)Data()[S-1024+i]=uint8_t(i*17+3);
}
void Step(uint64_t epoch=71){
    auto c=Context();const auto before=c;_mm_setcsr(0x5f80);
    TrialCheatsUpdate(c,Data(),epoch);
    assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x5f80);
}
void Parity(){
    unsigned comparisons=0;
    for(unsigned i=0;i<trial_cheats::Count;++i)for(bool enabled:{false,true})for(bool known:{false,true}){
        const uint32_t bit=1u<<i;
        Seed((0x155u&~bit)|(enabled?bit:0),(0x2aau&~bit)|(known?bit:0));
        Step();const auto state=trial_cheats::Snapshot();assert(state.valid&&state.identity&&state.pause_epoch==71);
        const std::vector<uint8_t> initial(memory.begin(),memory.end());
        auto original=Context();original.esp-=64;original.ebp=original.esp+16;
        original.esi=i*(!enabled&&!known?20:5);original.edi=i;
        if(!enabled&&!known)OriginalActivate(original,Data());else OriginalToggle(original,Data());
        // The original UI has its own caller frame; the added action restores
        // its borrowed scratch stack instead of exposing that frame to the UI.
        std::memcpy(Data()+S-1024,initial.data()+S-1024,1024);
        const std::vector<uint8_t> expected(memory.begin(),memory.end());const auto expected_events=events;
        std::memcpy(Data(),initial.data(),initial.size());events.clear();
        assert(trial_cheats::Toggle(i,state)&&trial_cheats::Snapshot().pending);
        assert(!trial_cheats::Toggle(i,state));Step();
        assert(!std::memcmp(Data(),expected.data(),expected.size())&&events==expected_events);
        const auto after=trial_cheats::Snapshot();
        assert(after.valid&&!after.pending&&after.identity==state.identity&&after.enabled==(state.enabled^bit));
        Step();assert(events==expected_events);++comparisons;
    }
    std::printf("Cheats: %u native RAM/action comparisons passed\n",comparisons);
}
void StaleRequests(){
    for(unsigned fault=0;fault<12;++fault){
        Seed();Step();const auto state=trial_cheats::Snapshot();assert(trial_cheats::Toggle(6,state));
        uint64_t epoch=71;
        switch(fault){
        case 0:epoch=0;break;
        case 1:epoch=72;break;
        case 2:Put(P,124);break;
        case 3:Data()[P+4]='J';break;
        case 4:Put(0x51c4dc,W+0x100);break;
        case 5:Put(W+4,L+0x100);break;
        case 6:Data()[P+0x183]=1;break; // Native change after the copied UI state.
        case 7:Put(0x501728+6*20+8,1000);break;
        case 8:Put(0x501728+6*20+12,0xffffffffu);break;
        case 9:Put(W+0x6c,0);break;
        case 10:committed=false;break;
        case 11:Put(V+0x68,0);break;
        }
        const std::vector<uint8_t> bytes(memory.begin(),memory.end());Step(epoch);
        assert(!std::memcmp(Data(),bytes.data(),bytes.size())&&events.empty()&&!trial_cheats::Snapshot().pending);
        assert(!trial_cheats::Toggle(6,state));committed=true;
    }
    Seed();Step();const auto first=trial_cheats::Snapshot();
    assert(!trial_cheats::Toggle(trial_cheats::Count,first));
    auto stale=first;stale.pause_epoch=72;assert(!trial_cheats::Toggle(0,stale));
    stale=first;stale.identity++;assert(!trial_cheats::Toggle(0,stale));
    stale=first;stale.enabled^=1;assert(!trial_cheats::Toggle(0,stale));
    stale=first;stale.valid=false;assert(!trial_cheats::Toggle(0,stale));
    TrialCheatsInvalidate();assert(!trial_cheats::Toggle(0,first));Step();
    assert(trial_cheats::Snapshot().identity!=first.identity);
    // Invalid stack does not dispatch any native action or touch state.
    auto current=trial_cheats::Snapshot();assert(trial_cheats::Toggle(0,current));
    auto c=Context();c.esp=0x10000;TrialCheatsUpdate(c,Data(),71);
    assert(events.empty()&&!trial_cheats::Snapshot().pending&&!(trial_cheats::Snapshot().enabled&1));
}
void PermissionAndReset(){
    // Main Menu can toggle native profile flags without a loaded campaign.
    Seed();Put(W+4,0);Step();assert(trial_cheats::Snapshot().valid);
    assert(trial_cheats::Toggle(0,trial_cheats::Snapshot()));Step();
    assert(trial_cheats::Snapshot().enabled==1&&!trial_cheats::Snapshot().pending);
    Put(W+4,0xfffffff0);Step();assert(!trial_cheats::Snapshot().valid);

    for(unsigned i=0;i<trial_cheats::Count;++i){
        Seed(0,0,true);auto permission=Context();permission.esi=i*20;OriginalPermission(permission,Data());
        const bool allowed=permission.eax!=0;Step();assert(trial_cheats::Toggle(i,trial_cheats::Snapshot()));Step();
        assert(bool(trial_cheats::Snapshot().enabled&(1u<<i))==allowed);
        assert(Data()[P+0x188+cheats::offsets[i]]==uint8_t(allowed));
        if(!allowed)assert(events.empty());
        // Once originally discovered, toggling uses the retail toggle route.
        Seed(0,1u<<i,true);Step();assert(trial_cheats::Toggle(i,trial_cheats::Snapshot()));Step();
        assert(trial_cheats::Snapshot().enabled==(1u<<i));
        assert(trial_cheats::Toggle(i,trial_cheats::Snapshot()));Step();assert(!trial_cheats::Snapshot().enabled);
    }
    Seed(0x3ff,0x3ff);auto c=Context();OriginalReset(c,Data());Step();
    assert(trial_cheats::Snapshot().enabled==((1u<<1)|(1u<<3)));
    for(unsigned i=0;i<trial_cheats::Count;++i)assert(Data()[P+0x188+cheats::offsets[i]]==1);
}
}
int main(){
    cheat_test::Parity();cheat_test::StaleRequests();cheat_test::PermissionAndReset();
    std::puts("Cheats: profile-shared native toggles, discovery, sound refresh, stale contexts, permissions and reset policy passed");
}
