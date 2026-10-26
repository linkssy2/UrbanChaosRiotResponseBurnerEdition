// Recorder tests, not evidence that the aim issue is repaired.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
namespace aim_test {std::array<uint8_t,0x800000> memory{};std::array<uint32_t,2> admitted{};}
uint32_t TrialCoopPlayer(unsigned slot){return slot<aim_test::admitted.size()?aim_test::admitted[slot]:0;}
namespace mem {bool IsCommitted(uint32_t page){return page<aim_test::memory.size();}}
#include "../common/xbe_aim_diagnostic.cpp"
namespace aim_test {
constexpr uint32_t P=0x600000,Q=0x602000,C=0x610000,S=0x7f0000,R=0x620000,B=0x630000,H=0x640000,T=0x650000,V=0x660000;
constexpr uint32_t W=0x670000,L=0x672000,M=0x674000,I=0x676000,F=0x678000;
uint8_t* Data(){return memory.data();}
void Put(uint32_t a,uint32_t v){std::memcpy(Data()+a,&v,4);}
void Float(uint32_t a,float v){std::memcpy(Data()+a,&v,4);}
X86Context Context(){X86Context c{};c.esp=S;c.eax=12;c.ebx=13;c.esi=P;c.edi=P;c.edx=14;c.ecx=15;c.cf=c.zf=1;return c;}
void Owner(uint32_t p,unsigned slot){Put(p,0x46fbe8);Put(p+0x54,7+slot);Put(p+0x368,slot);}
template<class F> void Immutable(X86Context& c,F fn){
    const auto old=c;const std::vector<uint8_t> bytes(memory.begin(),memory.end());const auto csr=_mm_getcsr();fn();
    assert(std::memcmp(&old,&c,sizeof c)==0);assert(std::memcmp(memory.data(),bytes.data(),bytes.size())==0);assert(_mm_getcsr()==csr);
}
void OriginalFirePrefix(X86Context& ctx,uint8_t* base){
    X86_LOAD_FLAGS();
// ORIGINAL_FIRE_PREFIX
    X86_SAVE_FLAGS();
}
void EntryWitness(unsigned slot){
    auto c=Context();c.ecx=slot?Q:P;c.esi=slot?P:Q;c.ebp=0x12345678;c.fs_base=F;
    Put(S,0x00be947);Put(S+4,2);Put(F,0x12340000);
    const auto initial=c;const std::vector<uint8_t> bytes(memory.begin(),memory.end());
    OriginalFirePrefix(c,Data());
    assert(c.esi==initial.ecx&&c.edi==2&&c.ecx==initial.ecx+0x498);
    assert(c.ebp==S-4&&c.esp==S-0xaf4&&aim_diagnostic::Word(Data(),c.esp)==2);
    const auto expected=c;const std::vector<uint8_t> expected_bytes(memory.begin(),memory.end());
    c=initial;std::memcpy(Data(),bytes.data(),bytes.size());
    {TrialAimFireScope scope(c,Data());OriginalFirePrefix(c,Data());}
    assert(std::memcmp(&c,&expected,sizeof c)==0);
    assert(std::memcmp(Data(),expected_bytes.data(),expected_bytes.size())==0);
    std::memcpy(Data(),bytes.data(),bytes.size());
}
void Shot(unsigned slot){
    const uint32_t p=slot?Q:P;auto c=Context();c.esi=p;Put(S,R);Put(S+4,7+slot);
    Put(R,2);Put(R+12,7+slot);Float(R+48,10);Float(R+52,20);Float(R+56,30);
    Float(R+16,101);Float(R+20,202);Float(R+24,303);
    Float(S+8+0x54,100);Float(S+8+0x58,200);Float(S+8+0x5c,300);
    auto hit=Context();hit.ecx=B;hit.esp=S-0x100;Put(hit.esp+4,H);Put(B+0xe0,7+slot);Put(B+0x54,22);
    Float(H+0x10,111);Float(H+0x14,222);Float(H+0x18,333);
    auto entry=Context();entry.ecx=p;entry.esp=S-0x200;Put(entry.esp,0x00be947);Put(entry.esp+4,2);
    const unsigned previous_shots=aim_diagnostic::shots;
    Immutable(entry,[&]{TrialAimFireScope firing(entry,Data());
        Immutable(c,[&]{TrialAimShotScope shot(c,Data());
            auto spawned=Context();spawned.esi=B;Immutable(spawned,[&]{TrialAimBullet(spawned,Data());});
            Immutable(hit,[&]{TrialAimImpact(hit,Data());});
        });
        assert(firing.ordinary_==1&&firing.accepted_==(previous_shots<64?1u:0u));
    });
    // Preserve the binding after the firing function returns; no TLS owner.
    Immutable(hit,[&]{TrialAimImpact(hit,Data());});
    const unsigned lines=aim_diagnostic::lines;
    Put(B+0xe0,1000);Immutable(hit,[&]{TrialAimImpact(hit,Data());});assert(aim_diagnostic::lines==lines);
    Put(B+0xe0,7+slot);Put(B+0x54,23);Immutable(hit,[&]{TrialAimImpact(hit,Data());});assert(aim_diagnostic::lines==lines);
    Put(B+0x54,22);
}
}
int main(int argc,char** argv){
    using namespace aim_test;assert(argc==2);
    trial_coop::SetAvailable(true);
    if(std::string(argv[1])=="off"){
        auto c=Context();Immutable(c,[&]{TrialAimFireScope fire(c,nullptr);TrialAimShotScope a(c,nullptr);TrialAimReticleScope r(c,nullptr);
            TrialAimBullet(c,nullptr);TrialAimImpact(c,nullptr);TrialAimRender(c,nullptr);TrialAimQuad(c,nullptr);});
        assert(aim_diagnostic::lines==0&&aim_diagnostic::fire_calls==0&&aim_diagnostic::fire==nullptr);return 0;
    }
    Owner(P,0);Owner(Q,1);Put(C,0x47c780);Put(T,0x453160);Put(T+4,P);
    admitted={P,Q};Put(P+0x308,43);Put(Q+0x308,44);Put(C+0x54,43);Put(C+0x58,7);
    Put(0x51c4dc,W);Put(W+4,L);Put(L+4,L+0x20);Put(L+8,L+0x28);
    for(unsigned i=0;i<2;++i){const auto layer=L+0x40+0x40*i;Put(L+0x20+i*4,layer);
        Put(layer+4,i?Q:P);Put(layer+0x24,i);Put(layer+0x20,V+0x100*i);}
    Put(0x5096e0,M);Put(M+0xc,0x03020100);Put(M+0x10,0x01010101);Put(M+0x14,0x01010101);Put(W+0x20,I);
    Put(I,0x123456a1);Put(I+0xd0,0x123456b2);Put(W+0x948,2);Put(W+0x94c,3);
    Put(W+0x24,M+0x100);Put(M+0x100,M+0x200);Put(M+0x104,M+0x300);
    for(unsigned i=0;i<2;++i){
        const auto device=M+0x200+i*0x100,binding=M+0x400+i*0x200,source=M+0x800+i*0x100;
        const auto state=M+0xa00+i*0x10,values=M+0xc00+i*0x100,flags=M+0xe00+i*0x100;
        Put(device+4,binding);Put(device+8,W);Put(device+12,i);Put(device+0x74,0xaabb00+i);
        Put(binding+4,i+1);Put(binding+0x14,source);Put(binding+0x18,1);Put(binding+0x40,values);Put(binding+0x44,flags);
        Float(values+11*4,float(i));Data()[flags+11]=uint8_t(2+i);
        Put(source,0x481768);Put(source+8,i);Put(source+12,i);Put(source+0x10,0x4fd5e0);
        Float(source+0x48,float(i));Put(source+0x74,state);Put(state,0xabc0+i);Put(state+4,30+i);
    }
    trial_coop::SetAvailable(false);auto disabled=Context();
    Immutable(disabled,[&]{TrialAimFireScope fire(disabled,nullptr);TrialAimShotScope a(disabled,nullptr);TrialAimReticleScope r(disabled,nullptr);
        TrialAimBullet(disabled,nullptr);TrialAimImpact(disabled,nullptr);TrialAimRender(disabled,nullptr);TrialAimQuad(disabled,nullptr);});
    assert(aim_diagnostic::shots==0&&aim_diagnostic::lines==0&&aim_diagnostic::fire_calls==0);trial_coop::SetAvailable(true);
    EntryWitness(0);EntryWitness(1);
    auto resumed=Context();resumed.esi=Q;resumed.ecx=P;resumed.edi=2;
    Immutable(resumed,[&]{TrialAimFireScope outer(resumed,Data(),true);
        assert(outer.owner_==Q&&aim_diagnostic::fire==&outer);
        try{TrialAimFireScope nested(resumed,Data());assert(aim_diagnostic::fire==&nested);throw 7;}
        catch(int){assert(aim_diagnostic::fire==&outer);}
    });assert(aim_diagnostic::fire==nullptr);
    auto render=Context();render.edi=P;render.esi=C;
    Put(S+0x84,V);Put(V,0x456f98);Float(V+8,0);Float(V+12,0);Float(V+16,240);Float(V+20,640);
    for(unsigned i=0;i<64;++i)Float(S+0x80+0x10+i*4,float(i));
    Immutable(render,[&]{TrialAimRender(render,Data());});
    Shot(0);Shot(1);
    render.edi=P;Immutable(render,[&]{TrialAimRender(render,Data());});
    auto reticle=Context();reticle.ecx=T;Put(S,0x7b149);Put(S+4,R);Put(S+8,R+16);
    Float(R,315.5f);Float(R+4,115.5f);Float(R+16,323.5f);Float(R+20,123.5f);
    Immutable(reticle,[&]{TrialAimReticleScope scope(reticle,Data());TrialAimQuad(reticle,Data());});
    // Replacement identity must not inherit a pending shot/camera capture.
    Put(P+0x54,99);Immutable(render,[&]{TrialAimRender(render,Data());});
    assert(aim_diagnostic::players[0].shot==0);
    auto invalid=Context();invalid.esi=0xfffff000;invalid.esp=0xfffffff8;
    Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());TrialAimRender(invalid,Data());});
    invalid.esp=0xfffffffcu;Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());});
    invalid.esp=S;Put(S,0xfffffffcu);Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());});
    invalid.esi=P;Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());});
    Put(S,R);Put(S+4,9);Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());});
    Put(S+4,99);Put(R+12,8);Immutable(invalid,[&]{TrialAimShotScope scope(invalid,Data());});
    // Shot records stop at 64; fire coverage has its own 128-entry budget.
    Put(P+0x54,7);for(unsigned i=2;i<68;++i)Shot(i%2);
    assert(aim_diagnostic::shots==64);
    std::ifstream input(argv[1]);std::string log((std::istreambuf_iterator<char>(input)),{});
    assert(log.find("base_direction=100,200,300")!=std::string::npos);
    assert(log.find("spread_direction=101,202,303")!=std::string::npos);
    assert(log.find("static-hit")!=std::string::npos&&log.find("point=111,222,333")!=std::string::npos);
    assert(log.find("owner=00602000 id=8")!=std::string::npos);
    assert(log.find("previous-render")!=std::string::npos&&log.find("camera_inverse=16,17,18,19")!=std::string::npos);
    assert(log.find("viewport_tlbr=0,0,240,640")!=std::string::npos);
    assert(log.find("reticle-quad")!=std::string::npos&&log.find("xy0=315.5,115.5")!=std::string::npos);
    assert(log.find("entry=CAB20 item=2 return=000BE947 owner=00600000")!=std::string::npos);
    assert(log.find("entry=CAB20 item=2 return=000BE947 owner=00602000")!=std::string::npos);
    assert(log.find("entry=CAF25 item=2 return=00000000 owner=00602000")!=std::string::npos);
    assert(log.find("ordinary_bullets=1 recorded_bullets=1")!=std::string::npos);
    assert(log.find("ordinary_bullets=0 recorded_bullets=0")!=std::string::npos);
    for(const char* reason:{"stack","owner","packet","argument-owner","packet-owner"})
        assert(log.find(std::string("reason=")+reason)!=std::string::npos);
    assert(log.find("admitted0=00600000 admitted1=00602000")!=std::string::npos);
    assert(log.find("layer0_owner=00600000 layer0_tag=0 layer0_viewport=00660000")!=std::string::npos);
    assert(log.find("layer1_owner=00602000 layer1_tag=1 layer1_viewport=00660100")!=std::string::npos);
    assert(log.find("input_map0c18=00010203010101010101010100")!=std::string::npos);
    assert(log.find("preset0=2 preset1=3")!=std::string::npos);
    assert(log.find("input_state0=A1563412")!=std::string::npos&&log.find("input_state1=B2563412")!=std::string::npos);
    assert(log.find("device0=00674200 device0_world=00670000 device0_port=0")!=std::string::npos);
    assert(log.find("device1=00674300 device1_world=00670000 device1_port=1")!=std::string::npos);
    assert(log.find("device0_trigger=00BBAA00")!=std::string::npos&&log.find("device1_trigger=01BBAA00")!=std::string::npos);
    assert(log.find("binding0=00674400 binding0_selector=1 binding0_source=00674800 binding0_count=1 binding0_fire_bits=00000000 binding0_fire_flag=02 source0_pad=1 source0_port=0 source0_index=0 source0_trigger_bits=00000000 source0_handle=0000ABC0 source0_packet=30")!=std::string::npos);
    assert(log.find("binding1=00674600 binding1_selector=2 binding1_source=00674900 binding1_count=1 binding1_fire_bits=3F800000 binding1_fire_flag=03 source1_pad=1 source1_port=1 source1_index=1 source1_trigger_bits=3F800000 source1_handle=0000ABC1 source1_packet=31")!=std::string::npos);
    // Record an actual binding swap and reject unrelated/unreadable source types without guest writes.
    Put(M+0x414,M+0x900);Put(M+0x614,M+0x800);
    Immutable(resumed,[&]{TrialAimFireScope scope(resumed,Data(),true);});
    Put(M+0x414,0xfffffff0u);Put(M+0x800,0);
    Immutable(resumed,[&]{TrialAimFireScope scope(resumed,Data(),true);});
    Put(M+0x204,0xfffffffcu);Put(M+0x644,0xfffffffcu);
    Immutable(resumed,[&]{TrialAimFireScope scope(resumed,Data(),true);});
    input.close();input.open(argv[1]);log.assign(std::istreambuf_iterator<char>(input),{});
    assert(log.find("binding0_source=00674900")!=std::string::npos);
    assert(log.find("source0_pad=1 source0_port=1 source0_index=1")!=std::string::npos);
    assert(log.find("binding1_source=00674800")!=std::string::npos);
    assert(log.find("source1_pad=1 source1_port=0 source1_index=0")!=std::string::npos);
    assert(log.find("source0_pad=0")!=std::string::npos&&log.find("source1_pad=0")!=std::string::npos);
    assert(log.find("binding1_fire_flag=unreadable")!=std::string::npos);
    for(unsigned i=0;i<140;++i)Immutable(resumed,[&]{TrialAimFireScope scope(resumed,Data(),true);});
    assert(aim_diagnostic::fire_calls==128&&aim_diagnostic::fire==nullptr);
    return 0;
}
